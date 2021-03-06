// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Xilinx, Inc.
 *
 * Authors: sonal.santan@xilinx.com
 */

#ifndef	_XOCL_ICAP_DEFS_H_
#define	_XOCL_ICAP_DEFS_H_

#include <linux/list.h>
#include <linux/pid.h>

#include "xocl-lib.h"
#include "xocl-features.h"
#include "xocl-mailbox-proto.h"
#include "xclbin.h"

#define	ICAP_PRIVILEGED(icap)	((icap)->icap_regs != NULL)
#define DMA_HWICAP_BITFILE_BUFFER_SIZE 1024

#define	ICAP_MAX_NUM_CLOCKS		4
#define OCL_CLKWIZ_STATUS_OFFSET	0x4
#define OCL_CLKWIZ_STATUS_MASK		0xffff
#define OCL_CLKWIZ_STATUS_MEASURE_START	0x1
#define OCL_CLKWIZ_STATUS_MEASURE_DONE	0x2
#define OCL_CLKWIZ_CONFIG_OFFSET(n)	(0x200 + 4 * (n))
#define OCL_CLK_FREQ_COUNTER_OFFSET	0x8
#define OCL_CLK_FREQ_V5_COUNTER_OFFSET	0x10
#define OCL_CLK_FREQ_V5_CLK0_ENABLED	0x10000
#define ICAP_DEFAULT_EXPIRE_SECS	1

#define INVALID_MEM_IDX			0xFFFF
/*
 * Bitstream header information.
 */
typedef struct {
	unsigned int HeaderLength;     /* Length of header in 32 bit words */
	unsigned int BitstreamLength;  /* Length of bitstream to read in bytes*/
	unsigned char *DesignName;     /* Design name read from bitstream header */
	unsigned char *PartName;       /* Part name read from bitstream header */
	unsigned char *Date;           /* Date read from bitstream header */
	unsigned char *Time;           /* Bitstream creation time read from header */
	unsigned int MagicLength;      /* Length of the magic numbers in header */
} XHwIcap_Bit_Header;
#define XHI_BIT_HEADER_FAILURE	-1
/* Used for parsing bitstream header */
#define XHI_EVEN_MAGIC_BYTE	0x0f
#define XHI_ODD_MAGIC_BYTE	0xf0
/* Extra mode for IDLE */
#define XHI_OP_IDLE		-1
/* The imaginary module length register */
#define XHI_MLR			15

#define	GATE_FREEZE_USER	0x0c

enum icap_sec_level {
	ICAP_SEC_NONE = 0,
	ICAP_SEC_DEDICATE,
	ICAP_SEC_SYSTEM,
	ICAP_SEC_MAX = ICAP_SEC_SYSTEM,
};

/*
 * AXI-HWICAP IP register layout
 */
struct icap_reg {
	u32			ir_rsvd1[7];
	u32			ir_gier;
	u32			ir_isr;
	u32			ir_rsvd2;
	u32			ir_ier;
	u32			ir_rsvd3[53];
	u32			ir_wf;
	u32			ir_rf;
	u32			ir_sz;
	u32			ir_cr;
	u32			ir_sr;
	u32			ir_wfv;
	u32			ir_rfo;
	u32			ir_asr;
} __attribute__((packed));

struct icap_generic_state {
	u32			igs_state;
} __attribute__((packed));

struct icap_axi_gate {
	u32			iag_wr;
	u32			iag_rvsd;
	u32			iag_rd;
} __attribute__((packed));

struct icap_bitstream_user {
	struct list_head	ibu_list;
	pid_t			ibu_pid;
};

struct xocl_icap {
	struct xocl_subdev_base    core;
	struct mutex		   icap_lock;
	struct icap_reg		  *icap_regs;
	struct icap_generic_state *icap_state;
	unsigned int		   idcode;
	bool			   icap_axi_gate_frozen;
	struct icap_axi_gate	  *icap_axi_gate;

	uuid_t			   icap_bitstream_uuid;
	int			   icap_bitstream_ref;

	char			  *icap_clock_bases[ICAP_MAX_NUM_CLOCKS];
	unsigned short		   icap_ocl_frequency[ICAP_MAX_NUM_CLOCKS];

	struct clock_freq_topology *icap_clock_freq_topology;
	unsigned long		    icap_clock_freq_topology_length;
	char			   *icap_clock_freq_counter;
	struct mem_topology	   *mem_topo;
	struct ip_layout	   *ip_layout;
	struct debug_ip_layout	   *debug_layout;
	struct connectivity	   *connectivity;
	void			   *partition_metadata;

	void			   *rp_bit;
	unsigned long		    rp_bit_len;
	void			   *rp_fdt;
	unsigned long		    rp_fdt_len;
	void			   *rp_mgmt_bin;
	unsigned long		    rp_mgmt_bin_len;
	void			   *rp_sche_bin;
	unsigned long		    rp_sche_bin_len;
	void			   *rp_sc_bin;
	unsigned long		   *rp_sc_bin_len;

	struct bmc		    bmc_header;

	char			   *icap_clock_freq_counters[ICAP_MAX_NUM_CLOCKS];
	char			   *icap_ucs_control_status;

	uint64_t		    cache_expire_secs;
	struct xcl_pr_region	    cache;
	ktime_t			    cache_expires;

	enum icap_sec_level	    sec_level;


	/* Use reader_ref as xclbin metadata reader counter
	 * Ther reference count increases by 1
	 * if icap_xclbin_rd_lock get called.
	 */
	u64			    busy;
	int			    reader_ref;
	wait_queue_head_t	    reader_wq;
};

/*
 * Precomputed table with config0 and config2 register values together with
 * target frequency. The steps are approximately 5 MHz apart. Table is
 * generated by wiz.pl.
 */
const static struct xclmgmt_ocl_clockwiz {
	/* target frequency */
	unsigned short ocl;
	/* config0 register */
	unsigned long config0;
	/* config2 register */
	unsigned config2;
} frequency_table[] = {
	{/*1275.000*/   10.000, 	0x02EE0C01,     0x0001F47F},
	{/*1575.000*/   15.000, 	0x02EE0F01,     0x00000069},
	{/*1600.000*/   20.000, 	0x00001001,     0x00000050},
	{/*1600.000*/   25.000, 	0x00001001,     0x00000040},
	{/*1575.000*/   30.000, 	0x02EE0F01,     0x0001F434},
	{/*1575.000*/   35.000, 	0x02EE0F01,     0x0000002D},
	{/*1600.000*/   40.000, 	0x00001001,     0x00000028},
	{/*1575.000*/   45.000, 	0x02EE0F01,     0x00000023},
	{/*1600.000*/   50.000, 	0x00001001,     0x00000020},
	{/*1512.500*/   55.000, 	0x007D0F01,     0x0001F41B},
	{/*1575.000*/   60.000, 	0x02EE0F01,     0x0000FA1A},
	{/*1462.500*/   65.000, 	0x02710E01,     0x0001F416},
	{/*1575.000*/   70.000, 	0x02EE0F01,     0x0001F416},
	{/*1575.000*/   75.000, 	0x02EE0F01,     0x00000015},
	{/*1600.000*/   80.000, 	0x00001001,     0x00000014},
	{/*1487.500*/   85.000, 	0x036B0E01,     0x0001F411},
	{/*1575.000*/   90.000, 	0x02EE0F01,     0x0001F411},
	{/*1425.000*/   95.000, 	0x00FA0E01,     0x0000000F},
	{/*1600.000*/   100.000,        0x00001001,     0x00000010},
	{/*1575.000*/   105.000,        0x02EE0F01,     0x0000000F},
	{/*1512.500*/   110.000,        0x007D0F01,     0x0002EE0D},
	{/*1437.500*/   115.000,        0x01770E01,     0x0001F40C},
	{/*1575.000*/   120.000,        0x02EE0F01,     0x00007D0D},
	{/*1562.500*/   125.000,        0x02710F01,     0x0001F40C},
	{/*1462.500*/   130.000,        0x02710E01,     0x0000FA0B},
	{/*1350.000*/   135.000,        0x01F40D01,     0x0000000A},
	{/*1575.000*/   140.000,        0x02EE0F01,     0x0000FA0B},
	{/*1450.000*/   145.000,        0x01F40E01,     0x0000000A},
	{/*1575.000*/   150.000,        0x02EE0F01,     0x0001F40A},
	{/*1550.000*/   155.000,        0x01F40F01,     0x0000000A},
	{/*1600.000*/   160.000,        0x00001001,     0x0000000A},
	{/*1237.500*/   165.000,        0x01770C01,     0x0001F407},
	{/*1487.500*/   170.000,        0x036B0E01,     0x0002EE08},
	{/*1575.000*/   175.000,        0x02EE0F01,     0x00000009},
	{/*1575.000*/   180.000,        0x02EE0F01,     0x0002EE08},
	{/*1387.500*/   185.000,        0x036B0D01,     0x0001F407},
	{/*1425.000*/   190.000,        0x00FA0E01,     0x0001F407},
	{/*1462.500*/   195.000,        0x02710E01,     0x0001F407},
	{/*1600.000*/   200.000,        0x00001001,     0x00000008},
	{/*1537.500*/   205.000,        0x01770F01,     0x0001F407},
	{/*1575.000*/   210.000,        0x02EE0F01,     0x0001F407},
	{/*1075.000*/   215.000,        0x02EE0A01,     0x00000005},
	{/*1512.500*/   220.000,        0x007D0F01,     0x00036B06},
	{/*1575.000*/   225.000,        0x02EE0F01,     0x00000007},
	{/*1437.500*/   230.000,        0x01770E01,     0x0000FA06},
	{/*1175.000*/   235.000,        0x02EE0B01,     0x00000005},
	{/*1500.000*/   240.000,        0x00000F01,     0x0000FA06},
	{/*1225.000*/   245.000,        0x00FA0C01,     0x00000005},
	{/*1562.500*/   250.000,        0x02710F01,     0x0000FA06},
	{/*1275.000*/   255.000,        0x02EE0C01,     0x00000005},
	{/*1462.500*/   260.000,        0x02710E01,     0x00027105},
	{/*1325.000*/   265.000,        0x00FA0D01,     0x00000005},
	{/*1350.000*/   270.000,        0x01F40D01,     0x00000005},
	{/*1512.500*/   275.000,        0x007D0F01,     0x0001F405},
	{/*1575.000*/   280.000,        0x02EE0F01,     0x00027105},
	{/*1425.000*/   285.000,        0x00FA0E01,     0x00000005},
	{/*1450.000*/   290.000,        0x01F40E01,     0x00000005},
	{/*1475.000*/   295.000,        0x02EE0E01,     0x00000005},
	{/*1575.000*/   300.000,        0x02EE0F01,     0x0000FA05},
	{/*1525.000*/   305.000,        0x00FA0F01,     0x00000005},
	{/*1550.000*/   310.000,        0x01F40F01,     0x00000005},
	{/*1575.000*/   315.000,        0x02EE0F01,     0x00000005},
	{/*1600.000*/   320.000,        0x00001001,     0x00000005},
	{/*1462.500*/   325.000,        0x02710E01,     0x0001F404},
	{/*1237.500*/   330.000,        0x01770C01,     0x0002EE03},
	{/*837.500*/    335.000,        0x01770801,     0x0001F402},
	{/*1487.500*/   340.000,        0x036B0E01,     0x00017704},
	{/*862.500*/    345.000,        0x02710801,     0x0001F402},
	{/*1575.000*/   350.000,        0x02EE0F01,     0x0001F404},
	{/*887.500*/    355.000,        0x036B0801,     0x0001F402},
	{/*1575.000*/   360.000,        0x02EE0F01,     0x00017704},
	{/*912.500*/    365.000,        0x007D0901,     0x0001F402},
	{/*1387.500*/   370.000,        0x036B0D01,     0x0002EE03},
	{/*1500.000*/   375.000,        0x00000F01,     0x00000004},
	{/*1425.000*/   380.000,        0x00FA0E01,     0x0002EE03},
	{/*962.500*/    385.000,        0x02710901,     0x0001F402},
	{/*1462.500*/   390.000,        0x02710E01,     0x0002EE03},
	{/*987.500*/    395.000,        0x036B0901,     0x0001F402},
	{/*1600.000*/   400.000,        0x00001001,     0x00000004},
	{/*1012.500*/   405.000,        0x007D0A01,     0x0001F402},
	{/*1537.500*/   410.000,        0x01770F01,     0x0002EE03},
	{/*1037.500*/   415.000,        0x01770A01,     0x0001F402},
	{/*1575.000*/   420.000,        0x02EE0F01,     0x0002EE03},
	{/*1487.500*/   425.000,        0x036B0E01,     0x0001F403},
	{/*1075.000*/   430.000,        0x02EE0A01,     0x0001F402},
	{/*1087.500*/   435.000,        0x036B0A01,     0x0001F402},
	{/*1375.000*/   440.000,        0x02EE0D01,     0x00007D03},
	{/*1112.500*/   445.000,        0x007D0B01,     0x0001F402},
	{/*1575.000*/   450.000,        0x02EE0F01,     0x0001F403},
	{/*1137.500*/   455.000,        0x01770B01,     0x0001F402},
	{/*1437.500*/   460.000,        0x01770E01,     0x00007D03},
	{/*1162.500*/   465.000,        0x02710B01,     0x0001F402},
	{/*1175.000*/   470.000,        0x02EE0B01,     0x0001F402},
	{/*1425.000*/   475.000,        0x00FA0E01,     0x00000003},
	{/*1500.000*/   480.000,        0x00000F01,     0x00007D03},
	{/*1212.500*/   485.000,        0x007D0C01,     0x0001F402},
	{/*1225.000*/   490.000,        0x00FA0C01,     0x0001F402},
	{/*1237.500*/   495.000,        0x01770C01,     0x0001F402},
	{/*1562.500*/   500.000,        0x02710F01,     0x00007D03},
	{/*1262.500*/   505.000,        0x02710C01,     0x0001F402},
	{/*1275.000*/   510.000,        0x02EE0C01,     0x0001F402},
	{/*1287.500*/   515.000,        0x036B0C01,     0x0001F402},
	{/*1300.000*/   520.000,        0x00000D01,     0x0001F402},
	{/*1575.000*/   525.000,        0x02EE0F01,     0x00000003},
	{/*1325.000*/   530.000,        0x00FA0D01,     0x0001F402},
	{/*1337.500*/   535.000,        0x01770D01,     0x0001F402},
	{/*1350.000*/   540.000,        0x01F40D01,     0x0001F402},
	{/*1362.500*/   545.000,        0x02710D01,     0x0001F402},
	{/*1512.500*/   550.000,        0x007D0F01,     0x0002EE02},
	{/*1387.500*/   555.000,        0x036B0D01,     0x0001F402},
	{/*1400.000*/   560.000,        0x00000E01,     0x0001F402},
	{/*1412.500*/   565.000,        0x007D0E01,     0x0001F402},
	{/*1425.000*/   570.000,        0x00FA0E01,     0x0001F402},
	{/*1437.500*/   575.000,        0x01770E01,     0x0001F402},
	{/*1450.000*/   580.000,        0x01F40E01,     0x0001F402},
	{/*1462.500*/   585.000,        0x02710E01,     0x0001F402},
	{/*1475.000*/   590.000,        0x02EE0E01,     0x0001F402},
	{/*1487.500*/   595.000,        0x036B0E01,     0x0001F402},
	{/*1575.000*/   600.000,        0x02EE0F01,     0x00027102},
	{/*1512.500*/   605.000,        0x007D0F01,     0x0001F402},
	{/*1525.000*/   610.000,        0x00FA0F01,     0x0001F402},
	{/*1537.500*/   615.000,        0x01770F01,     0x0001F402},
	{/*1550.000*/   620.000,        0x01F40F01,     0x0001F402},
	{/*1562.500*/   625.000,        0x02710F01,     0x0001F402},
	{/*1575.000*/   630.000,        0x02EE0F01,     0x0001F402},
	{/*1587.500*/   635.000,        0x036B0F01,     0x0001F402},
	{/*1600.000*/   640.000,        0x00001001,     0x0001F402},
	{/*1290.000*/   645.000,        0x01F44005,     0x00000002},
	{/*1462.500*/   650.000,        0x02710E01,     0x0000FA02}
};

static inline u32 reg_rd(void __iomem *reg)
{
	if (!reg)
		return -1;

	return XOCL_READ_REG32(reg);
}

static inline void reg_wr(void __iomem *reg, u32 val)
{
	if (!reg)
		return;

	iowrite32(val, reg);
}

#define	ICAP_ERR(icap, fmt, arg...)	\
	xocl_err(&(icap)->core.pdev->dev, fmt "\n", ##arg)
#define	ICAP_WARN(icap, fmt, arg...)	\
	xocl_warn(&(icap)->core.pdev->dev, fmt "\n", ##arg)
#define	ICAP_INFO(icap, fmt, arg...)	\
	xocl_info(&(icap)->core.pdev->dev, fmt "\n", ##arg)
#define	ICAP_DBG(icap, fmt, arg...)	\
	xocl_dbg(&(icap)->core.pdev->dev, fmt "\n", ##arg)

long icap_ioctl(struct platform_device *pdev, unsigned int cmd, unsigned long arg);

unsigned short icap_get_ocl_frequency(const struct xocl_icap *icap, int idx);

#endif
