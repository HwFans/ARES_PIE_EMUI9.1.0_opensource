/*
 *  linux/include/linux/mmc/core.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef LINUX_MMC_CORE_H
#define LINUX_MMC_CORE_H

#include <uapi/linux/mmc/core.h>
#include <linux/interrupt.h>
#include <linux/completion.h>

struct request;
struct mmc_data;
struct mmc_request;

struct mmc_command {
	u32			opcode;
	u32			arg;
#define MMC_CMD23_ARG_REL_WR	(1 << 31)
#define MMC_CMD23_ARG_PACKED	((0 << 31) | (1 << 30))
#define MMC_CMD23_ARG_TAG_REQ	(1 << 29)
	u32			resp[4];
	unsigned int		flags;		/* expected response type */
#define mmc_resp_type(cmd)	((cmd)->flags & (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC|MMC_RSP_BUSY|MMC_RSP_OPCODE))

/*
 * These are the SPI response types for MMC, SD, and SDIO cards.
 * Commands return R1, with maybe more info.  Zero is an error type;
 * callers must always provide the appropriate MMC_RSP_SPI_Rx flags.
 */
#define MMC_RSP_SPI_R1	(MMC_RSP_SPI_S1)
#define MMC_RSP_SPI_R1B	(MMC_RSP_SPI_S1|MMC_RSP_SPI_BUSY)
#define MMC_RSP_SPI_R2	(MMC_RSP_SPI_S1|MMC_RSP_SPI_S2)
#define MMC_RSP_SPI_R3	(MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)
#define MMC_RSP_SPI_R4	(MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)
#define MMC_RSP_SPI_R5	(MMC_RSP_SPI_S1|MMC_RSP_SPI_S2)
#define MMC_RSP_SPI_R7	(MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)

#define mmc_spi_resp_type(cmd)	((cmd)->flags & \
		(MMC_RSP_SPI_S1|MMC_RSP_SPI_BUSY|MMC_RSP_SPI_S2|MMC_RSP_SPI_B4))

/*
 * These are the command types.
 */
#define mmc_cmd_type(cmd)	((cmd)->flags & MMC_CMD_MASK)

	unsigned int		retries;	/* max number of retries */
	int			error;		/* command error */

/*
 * Standard errno values are used for errors, but some have specific
 * meaning in the MMC layer:
 *
 * ETIMEDOUT    Card took too long to respond
 * EILSEQ       Basic format problem with the received or sent data
 *              (e.g. CRC check failed, incorrect opcode in response
 *              or bad end bit)
 * EINVAL       Request cannot be performed because of restrictions
 *              in hardware and/or the driver
 * ENOMEDIUM    Host can determine that the slot is empty and is
 *              actively failing requests
 */

	unsigned int		busy_timeout;	/* busy detect timeout in ms */
	/* Set this flag only for blocking sanitize request */
	bool			sanitize_busy;
	/* Set this flag only for blocking bkops request */
	bool			bkops_busy;

	struct mmc_data		*data;		/* data segment associated with cmd */
	struct mmc_request	*mrq;		/* associated request */
};

struct mmc_data {
	unsigned int		timeout_ns;	/* data timeout (in ns, max 80ms) */
	unsigned int		timeout_clks;	/* data timeout (in clocks) */
	unsigned int		blksz;		/* data block size */
	unsigned int		blocks;		/* number of blocks */
	int			error;		/* data error */
	unsigned int		flags;

#define MMC_DATA_WRITE	(1 << 8)
#define MMC_DATA_READ	(1 << 9)
#define MMC_DATA_STREAM	(1 << 10)

	unsigned int		bytes_xfered;

	struct mmc_command	*stop;		/* stop command */
	struct mmc_request	*mrq;		/* associated request */

	unsigned int		sg_len;		/* size of scatter list */
	int			sg_count;	/* mapped sg entries */
	struct scatterlist	*sg;		/* I/O scatter list */
	s32			host_cookie;	/* host private data */
	bool			fault_injected; /* fault injected */
};

struct mmc_host;
struct mmc_request {
	struct mmc_command	*sbc;		/* SET_BLOCK_COUNT for multiblock */
	struct mmc_command	*cmd;
	struct mmc_data		*data;
	struct mmc_command	*stop;

	struct completion	completion;
	void			(*done)(struct mmc_request *);/* completion function */
	struct mmc_host		*host;
	struct mmc_cmdq_req	*cmdq_req;
	struct request *req;
	ktime_t io_start;
#ifdef CONFIG_BLOCK
	int					lat_hist_enabled;
#endif
};

struct mmc_card;

struct mmc_bus_ops {
	void (*remove)(struct mmc_host *);
	void (*detect)(struct mmc_host *);
	int (*pre_suspend)(struct mmc_host *);
	int (*suspend)(struct mmc_host *);
	int (*resume)(struct mmc_host *);
	int (*runtime_suspend)(struct mmc_host *);
	int (*runtime_resume)(struct mmc_host *);
	int (*runtime_idle)(struct mmc_host *);
	int (*power_save)(struct mmc_host *);
	int (*power_restore)(struct mmc_host *);
	int (*alive)(struct mmc_host *);
	int (*shutdown)(struct mmc_host *);
	int (*reset)(struct mmc_host *);
	int (*change_bus_speed)(struct mmc_host *, unsigned long *);
	int (*pre_hibernate)(struct mmc_host *);
	int (*post_hibernate)(struct mmc_host *);
#ifdef CONFIG_MMC_PASSWORDS
	int (*sysfs_add)(struct mmc_host *, struct mmc_card *card);
	void (*sysfs_remove)(struct mmc_host *, struct mmc_card *card);
#endif
};

struct mmc_card;
struct mmc_async_req;
struct mmc_cmdq_req;

extern int mmc_cmdq_discard_queue(struct mmc_host *host, u32 tasks);
extern int mmc_cmdq_halt(struct mmc_host *host, bool enable);
extern int mmc_cmdq_halt_on_empty_queue(struct mmc_host *host);
extern void mmc_cmdq_post_req(struct mmc_host *host, int tag, int err);
extern int mmc_cmdq_start_req(struct mmc_host *host,
			      struct mmc_cmdq_req *cmdq_req);
extern int mmc_cmdq_prepare_flush(struct mmc_command *cmd);
extern int mmc_cmdq_wait_for_dcmd(struct mmc_host *host,
			struct mmc_cmdq_req *cmdq_req);
extern int mmc_cmdq_erase(struct mmc_cmdq_req *cmdq_req,
	      struct mmc_card *card, unsigned int from, unsigned int nr,
	      unsigned int arg);

extern int mmc_stop_bkops(struct mmc_card *);
extern int mmc_read_bkops_status(struct mmc_card *);
extern struct mmc_async_req *mmc_start_req(struct mmc_host *,
					   struct mmc_async_req *, int *);
extern int mmc_interrupt_hpi(struct mmc_card *);
extern void mmc_wait_for_req(struct mmc_host *, struct mmc_request *);
extern int mmc_wait_for_cmd(struct mmc_host *, struct mmc_command *, int);
extern int mmc_app_cmd(struct mmc_host *, struct mmc_card *);
extern int mmc_wait_for_app_cmd(struct mmc_host *, struct mmc_card *,
	struct mmc_command *, int);
extern void mmc_check_bkops(struct mmc_card *card);
extern void mmc_start_manual_bkops(struct mmc_card *card);
extern int mmc_switch(struct mmc_card *, u8, u8, u8, unsigned int);
extern int __mmc_switch_cmdq_mode(struct mmc_command *cmd, u8 set, u8 index,
				  u8 value, unsigned int timeout_ms,
				  bool use_busy_signal, bool ignore_timeout);
extern int mmc_send_tuning(struct mmc_host *host, u32 opcode, int *cmd_error);
extern int mmc_get_ext_csd(struct mmc_card *card, u8 **new_ext_csd);
extern int mmc_set_auto_bkops(struct mmc_card *card, bool enable);
extern int mmc_suspend_clk_scaling(struct mmc_host *host);
extern void mmc_flush_detect_work(struct mmc_host *);

#define MMC_ERASE_ARG		0x00000000
#define MMC_SECURE_ERASE_ARG	0x80000000
#define MMC_TRIM_ARG		0x00000001
#define MMC_DISCARD_ARG		0x00000003
#define MMC_SECURE_TRIM1_ARG	0x80000001
#define MMC_SECURE_TRIM2_ARG	0x80008000

#define MMC_SECURE_ARGS		0x80000000
#define MMC_TRIM_ARGS		0x00008001

extern int mmc_erase(struct mmc_card *card, unsigned int from, unsigned int nr,
		     unsigned int arg);
extern int mmc_can_erase(struct mmc_card *card);
extern int mmc_can_trim(struct mmc_card *card);
extern int mmc_can_discard(struct mmc_card *card);
extern int mmc_can_sanitize(struct mmc_card *card);
extern int mmc_can_secure_erase_trim(struct mmc_card *card);
extern int mmc_erase_group_aligned(struct mmc_card *card, unsigned int from,
				   unsigned int nr);
extern unsigned int mmc_calc_max_discard(struct mmc_card *card);

extern int mmc_set_blocklen(struct mmc_card *card, unsigned int blocklen);
extern int mmc_set_blockcount(struct mmc_card *card, unsigned int blockcount,
			      bool is_rel_write);
extern int mmc_hw_reset(struct mmc_host *host);
extern int mmc_cmdq_hw_reset(struct mmc_host *host);
extern int mmc_can_reset(struct mmc_card *card);

extern void mmc_set_data_timeout(struct mmc_data *, const struct mmc_card *);
extern unsigned int mmc_align_data_size(struct mmc_card *, unsigned int);

extern int __mmc_claim_host(struct mmc_host *host, atomic_t *abort);
extern void mmc_release_host(struct mmc_host *host);
extern int mmc_try_claim_host(struct mmc_host *host, unsigned int delay);

extern void mmc_get_card(struct mmc_card *card);
extern void mmc_put_card(struct mmc_card *card);
extern void __mmc_put_card(struct mmc_card *card);

extern void mmc_set_ios(struct mmc_host *host);
extern int mmc_flush_cache(struct mmc_card *);
extern int mmc_cache_barrier(struct mmc_card *);

extern int mmc_detect_card_removed(struct mmc_host *host);

extern void mmc_blk_init_bkops_statistics(struct mmc_card *card);

extern void mmc_deferred_scaling(struct mmc_host *host);
extern void mmc_cmdq_clk_scaling_start_busy(struct mmc_host *host,
	bool lock_needed);
extern void mmc_cmdq_clk_scaling_stop_busy(struct mmc_host *host,
	bool lock_needed, bool is_cmdq_dcmd);
extern int mmc_recovery_fallback_lower_speed(struct mmc_host *host);

/**
 *	mmc_claim_host - exclusively claim a host
 *	@host: mmc host to claim
 *
 *	Claim a host for a set of operations.
 */
static inline void mmc_claim_host(struct mmc_host *host)
{
	__mmc_claim_host(host, NULL);
}

struct device_node;
extern u32 mmc_vddrange_to_ocrmask(int vdd_min, int vdd_max);
extern int mmc_of_parse_voltage(struct device_node *np, u32 *mask);

#endif /* LINUX_MMC_CORE_H */
