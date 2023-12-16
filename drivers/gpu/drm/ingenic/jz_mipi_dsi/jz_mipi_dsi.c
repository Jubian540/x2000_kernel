/*
 * Ingenic SoC MIPI-DSI dsim driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/memory.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>
#include <linux/gpio.h>
#include "../ingenic_drv.h"
#include "../jz_dsim.h"
#include "jz_mipi_dsi_lowlevel.h"
#include "jz_mipi_dsih_hal.h"
#include "jz_mipi_dsi_regs.h"

#define DSI_IOBASE	0x10075000
#define DSI_PHY_IOBASE	0x10077000

void dump_dsi_reg(struct dsi_device *dsi);

int jz_dsi_video_cfg(struct dsi_device *dsi)
{
	dsih_error_t err_code = OK;
	unsigned short bytes_per_pixel_x100 = 0;	/* bpp x 100 because it can be 2.25 */
	unsigned short video_size = 0;
	unsigned int ratio_clock_xPF = 0;	/* holds dpi clock/byte clock times precision factor */
	unsigned short null_packet_size = 0;
	unsigned char video_size_step = 1;
	unsigned int total_bytes = 0;
	unsigned int bytes_per_chunk = 0;
	unsigned int no_of_chunks = 0;
	unsigned int bytes_left = 0;
	unsigned int chunk_overhead = 0;

	struct video_config *video_config;
	video_config = dsi->video_config;

	/* check DSI controller dsi */
	if ((dsi == NULL) || (video_config == NULL)) {
		return ERR_DSI_INVALID_INSTANCE;
	}

	if (dsi->state != INITIALIZED) {
		return ERR_DSI_INVALID_INSTANCE;
	}

	ratio_clock_xPF =
	    (video_config->byte_clock * PRECISION_FACTOR) /
	    (video_config->pixel_clock);

	video_size = video_config->h_active_pixels;
	/*  disable set up ACKs and error reporting */
	mipi_dsih_hal_dpi_frame_ack_en(dsi, video_config->receive_ack_packets);
	if (video_config->receive_ack_packets) {	/* if ACK is requested, enable BTA, otherwise leave as is */
		mipi_dsih_hal_bta_en(dsi, 1);
	}
	/*0:switch to high speed transfer, 1 low power mode */
	mipi_dsih_write_word(dsi, R_DSI_HOST_CMD_MODE_CFG, 0);
	/*0:enable video mode, 1:enable cmd mode */
	mipi_dsih_hal_gen_set_mode(dsi, 0);

	err_code =
	    jz_dsi_video_coding(dsi, &bytes_per_pixel_x100, &video_size_step,
				&video_size);
	if (err_code) {
		return err_code;
	}

	jz_dsi_dpi_cfg(dsi, &ratio_clock_xPF, &bytes_per_pixel_x100);

	/* TX_ESC_CLOCK_DIV must be less than 20000KHz */
	jz_dsih_hal_tx_escape_division(dsi, TX_ESC_CLK_DIV);

	/* video packetisation   */
	if (video_config->video_mode == VIDEO_BURST_WITH_SYNC_PULSES) {	/* BURST */
		//mipi_dsih_hal_dpi_null_packet_en(dsi, 0);
		mipi_dsih_hal_dpi_null_packet_size(dsi, 0);
		//mipi_dsih_hal_dpi_multi_packet_en(dsi, 0);
		err_code =
		    err_code ? err_code : mipi_dsih_hal_dpi_chunks_no(dsi, 1);
		err_code =
		    err_code ? err_code :
		    mipi_dsih_hal_dpi_video_packet_size(dsi, video_size);
		if (err_code != OK) {
			return err_code;
		}
		/* BURST by default, returns to LP during ALL empty periods - energy saving */
		mipi_dsih_hal_dpi_lp_during_hfp(dsi, 1);
		mipi_dsih_hal_dpi_lp_during_hbp(dsi, 1);
		mipi_dsih_hal_dpi_lp_during_vactive(dsi, 1);
		mipi_dsih_hal_dpi_lp_during_vfp(dsi, 1);
		mipi_dsih_hal_dpi_lp_during_vbp(dsi, 1);
		mipi_dsih_hal_dpi_lp_during_vsync(dsi, 1);
#ifdef CONFIG_DSI_DPI_DEBUG
		/*      D E B U G               */
		{
			pr_info("burst video");
			pr_info("h line time %d ,",
			       (unsigned
				short)((video_config->h_total_pixels *
					ratio_clock_xPF) / PRECISION_FACTOR));
			pr_info("video_size %d ,", video_size);
		}
#endif
	} else {		/* non burst transmission */
		null_packet_size = 0;
		/* bytes to be sent - first as one chunk */
		bytes_per_chunk =
		    (bytes_per_pixel_x100 * video_config->h_active_pixels) /
		    100 + VIDEO_PACKET_OVERHEAD;
		/* bytes being received through the DPI interface per byte clock cycle */
		total_bytes =
		    (ratio_clock_xPF * video_config->no_of_lanes *
		     (video_config->h_total_pixels -
		      video_config->h_back_porch_pixels -
		      video_config->h_sync_pixels)) / PRECISION_FACTOR;
		pr_debug("---->total_bytes:%d, bytes_per_chunk:%d\n", total_bytes,
		       bytes_per_chunk);
		/* check if the in pixels actually fit on the DSI link */
		if (total_bytes >= bytes_per_chunk) {
			chunk_overhead = total_bytes - bytes_per_chunk;
			/* overhead higher than 1 -> enable multi packets */
			if (chunk_overhead > 1) {
				for (video_size = video_size_step; video_size < video_config->h_active_pixels; video_size += video_size_step) {	/* determine no of chunks */
					if ((((video_config->h_active_pixels *
					       PRECISION_FACTOR) / video_size) %
					     PRECISION_FACTOR) == 0) {
						no_of_chunks =
						    video_config->
						    h_active_pixels /
						    video_size;
						bytes_per_chunk =
						    (bytes_per_pixel_x100 *
						     video_size) / 100 +
						    VIDEO_PACKET_OVERHEAD;
						if (total_bytes >=
						    (bytes_per_chunk *
						     no_of_chunks)) {
							bytes_left =
							    total_bytes -
							    (bytes_per_chunk *
							     no_of_chunks);
							break;
						}
					}
				}
				/* prevent overflow (unsigned - unsigned) */
				if (bytes_left >
				    (NULL_PACKET_OVERHEAD * no_of_chunks)) {
					null_packet_size =
					    (bytes_left -
					     (NULL_PACKET_OVERHEAD *
					      no_of_chunks)) / no_of_chunks;
					if (null_packet_size > MAX_NULL_SIZE) {	/* avoid register overflow */
						null_packet_size =
						    MAX_NULL_SIZE;
					}
				}
			} else {	/* no multi packets */
				no_of_chunks = 1;
#ifdef CONFIG_DSI_DPI_DEBUG
				/*      D E B U G               */
				{
					pr_info("no multi no null video");
					pr_info("h line time %d",
					       (unsigned
						short)((video_config->
							h_total_pixels *
							ratio_clock_xPF) /
						       PRECISION_FACTOR));
					pr_info("video_size %d", video_size);
				}
#endif
				/* video size must be a multiple of 4 when not 18 loosely */
				for (video_size = video_config->h_active_pixels;
				     (video_size % video_size_step) != 0;
				     video_size++) {
					;
				}
			}
		} else {
			pr_err
			    ("resolution cannot be sent to display through current settings");
			err_code = ERR_DSI_OVERFLOW;

		}
	}
	err_code =
	    err_code ? err_code : mipi_dsih_hal_dpi_chunks_no(dsi,
							      no_of_chunks);
	err_code =
	    err_code ? err_code : mipi_dsih_hal_dpi_video_packet_size(dsi,
								      video_size);
	err_code =
	    err_code ? err_code : mipi_dsih_hal_dpi_null_packet_size(dsi,
								     null_packet_size);

	// mipi_dsih_hal_dpi_null_packet_en(dsi, null_packet_size > 0? 1: 0);
	// mipi_dsih_hal_dpi_multi_packet_en(dsi, (no_of_chunks > 1)? 1: 0);
#ifdef  CONFIG_DSI_DPI_DEBUG
	/*      D E B U G               */
	{
		pr_info("total_bytes %d ,", total_bytes);
		pr_info("bytes_per_chunk %d ,", bytes_per_chunk);
		pr_info("bytes left %d ,", bytes_left);
		pr_info("null packets %d ,", null_packet_size);
		pr_info("chunks %d ,", no_of_chunks);
		pr_info("video_size %d ", video_size);
	}
#endif
	mipi_dsih_hal_dpi_video_vc(dsi, video_config->virtual_channel);
	jz_dsih_dphy_no_of_lanes(dsi, video_config->no_of_lanes);
	/* enable high speed clock */
	mipi_dsih_dphy_enable_hs_clk(dsi, 1);
	pr_debug("video configure is ok!\n");
	return err_code;

}

/* set all register settings to MIPI DSI controller. */
dsih_error_t jz_dsi_phy_cfg(struct dsi_device * dsi)
{
	dsih_error_t err = OK;
	err = jz_dsi_set_clock(dsi);
	if (err) {
		return err;
	}
	err = jz_init_dsi(dsi);
	if (err) {
		return err;
	}
	return OK;
}

int jz_dsi_phy_open(struct dsi_device *dsi)
{
	struct video_config *video_config;
	video_config = dsi->video_config;

	pr_debug("entry %s()\n", __func__);

	jz_dsih_dphy_reset(dsi, 0);
	jz_dsih_dphy_stop_wait_time(dsi, 0x1c);	/* 0x1c: */

	if (video_config->no_of_lanes == 2 || video_config->no_of_lanes == 1) {
		jz_dsih_dphy_no_of_lanes(dsi, video_config->no_of_lanes);
	} else {
		return ERR_DSI_OUT_OF_BOUND;
	}

	jz_dsih_dphy_clock_en(dsi, 1);
	jz_dsih_dphy_shutdown(dsi, 1);
	jz_dsih_dphy_reset(dsi, 1);

	dsi->dsi_phy->status = INITIALIZED;
	return OK;
}

void dump_dsi_reg(struct dsi_device *dsi)
{
	pr_info("dsi->dev: ===========>dump dsi reg\n");
	pr_info("dsi->dev: VERSION------------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VERSION));
	pr_info("dsi->dev: PWR_UP:------------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_PWR_UP));
	pr_info("dsi->dev: CLKMGR_CFG---------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_CLKMGR_CFG));
	pr_info("dsi->dev: DPI_VCID-----------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_DPI_VCID));
	pr_info("dsi->dev: DPI_COLOR_CODING---:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_DPI_COLOR_CODING));
	pr_info("dsi->dev: DPI_CFG_POL--------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_DPI_CFG_POL));
	pr_info("dsi->dev: DPI_LP_CMD_TIM-----:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_DPI_LP_CMD_TIM));
	pr_info("dsi->dev: DBI_VCID-----------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_DBI_VCID));
	pr_info("dsi->dev: DBI_CFG------------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_DBI_CFG));
	pr_info("dsi->dev: DBI_PARTITIONING_EN:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_DBI_PARTITIONING_EN));
	pr_info("dsi->dev: DBI_CMDSIZE--------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_DBI_CMDSIZE));
	pr_info("dsi->dev: PCKHDL_CFG---------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_PCKHDL_CFG));
	pr_info("dsi->dev: GEN_VCID-----------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_GEN_VCID));
	pr_info("dsi->dev: MODE_CFG-----------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_MODE_CFG));
	pr_info("dsi->dev: VID_MODE_CFG-------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_MODE_CFG));
	pr_info("dsi->dev: VID_PKT_SIZE-------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_PKT_SIZE));
	pr_info("dsi->dev: VID_NUM_CHUNKS-----:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_NUM_CHUNKS));
	pr_info("dsi->dev: VID_NULL_SIZE------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_NULL_SIZE));
	pr_info("dsi->dev: VID_HSA_TIME-------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_HSA_TIME));
	pr_info("dsi->dev: VID_HBP_TIME-------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_HBP_TIME));
	pr_info("dsi->dev: VID_HLINE_TIME-----:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_HLINE_TIME));
	pr_info("dsi->dev: VID_VSA_LINES------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_VSA_LINES));
	pr_info("dsi->dev: VID_VBP_LINES------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_VBP_LINES));
	pr_info("dsi->dev: VID_VFP_LINES------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_VFP_LINES));
	pr_info("dsi->dev: VID_VACTIVE_LINES--:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_VACTIVE_LINES));
	pr_info("dsi->dev: EDPI_CMD_SIZE------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_EDPI_CMD_SIZE));
	pr_info("dsi->dev: CMD_MODE_CFG-------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_CMD_MODE_CFG));
	pr_info("dsi->dev: GEN_HDR------------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_GEN_HDR));
	pr_info("dsi->dev: GEN_PLD_DATA-------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_GEN_PLD_DATA));
	pr_info("dsi->dev: CMD_PKT_STATUS-----:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_CMD_PKT_STATUS));
	pr_info("dsi->dev: TO_CNT_CFG---------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_TO_CNT_CFG));
	pr_info("dsi->dev: HS_RD_TO_CNT-------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_HS_RD_TO_CNT));
	pr_info("dsi->dev: LP_RD_TO_CNT-------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_LP_RD_TO_CNT));
	pr_info("dsi->dev: HS_WR_TO_CNT-------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_HS_WR_TO_CNT));
	pr_info("dsi->dev: LP_WR_TO_CNT_CFG---:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_LP_WR_TO_CNT));
	pr_info("dsi->dev: BTA_TO_CNT---------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_BTA_TO_CNT));
	pr_info("dsi->dev: SDF_3D-------------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_SDF_3D));
	pr_info("dsi->dev: LPCLK_CTRL---------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_LPCLK_CTRL));
	pr_info("dsi->dev: PHY_TMR_LPCLK_CFG--:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_PHY_TMR_LPCLK_CFG));
	pr_info("dsi->dev: PHY_TMR_CFG--------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_PHY_TMR_CFG));
	pr_info("dsi->dev: PHY_RSTZ-----------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_PHY_RSTZ));
	pr_info("dsi->dev: PHY_IF_CFG---------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_PHY_IF_CFG));
	pr_info("dsi->dev: PHY_ULPS_CTRL------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_PHY_ULPS_CTRL));
	pr_info("dsi->dev: PHY_TX_TRIGGERS----:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_PHY_TX_TRIGGERS));
	pr_info("dsi->dev: PHY_STATUS---------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_PHY_STATUS));
	pr_info("dsi->dev: PHY_TST_CTRL0------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_PHY_TST_CTRL0));
	pr_info("dsi->dev: PHY_TST_CTRL1------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_PHY_TST_CTRL1));
	pr_info("dsi->dev: INT_ST0------------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_INT_ST0));
	pr_info("dsi->dev: INT_ST1------------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_INT_ST1));
	pr_info("dsi->dev: INT_MSK0-----------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_INT_MSK0));
	pr_info("dsi->dev: INT_MSK1-----------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_INT_MSK1));
	pr_info("dsi->dev: INT_FORCE0---------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_INT_FORCE0));
	pr_info("dsi->dev: INT_FORCE1---------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_INT_FORCE1));
	pr_info("dsi->dev: VID_SHADOW_CTRL----:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_SHADOW_CTRL));
	pr_info("dsi->dev: DPI_VCID_ACT-------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_DPI_VCID_ACT));
	pr_info("dsi->dev: DPI_COLOR_CODING_AC:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_DPI_COLOR_CODING_ACT));
	pr_info("dsi->dev: DPI_LP_CMD_TIM_ACT-:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_DPI_LP_CMD_TIM_ACT));
	pr_info("dsi->dev: VID_MODE_CFG_ACT---:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_MODE_CFG_ACT));
	pr_info("dsi->dev: VID_PKT_SIZE_ACT---:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_PKT_SIZE_ACT));
	pr_info("dsi->dev: VID_NUM_CHUNKS_ACT-:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_NUM_CHUNKS_ACT));
	pr_info("dsi->dev: VID_HSA_TIME_ACT---:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_HSA_TIME_ACT));
	pr_info("dsi->dev: VID_HBP_TIME_ACT---:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_HBP_TIME_ACT));
	pr_info("dsi->dev: VID_HLINE_TIME_ACT-:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_HLINE_TIME_ACT));
	pr_info("dsi->dev: VID_VSA_LINES_ACT--:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_VSA_LINES_ACT));
	pr_info("dsi->dev: VID_VBP_LINES_ACT--:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_VBP_LINES_ACT));
	pr_info("dsi->dev: VID_VFP_LINES_ACT--:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_VFP_LINES_ACT));
	pr_info("dsi->dev: VID_VACTIVE_LINES_ACT:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_VID_VACTIVE_LINES_ACT));
	pr_info("dsi->dev: SDF_3D_ACT---------:%08x\n",
		 mipi_dsih_read_word(dsi, R_DSI_HOST_SDF_3D_ACT));

}

void set_base_dir_tx(struct dsi_device *dsi, void *param)
{
	int i = 0;
	register_config_t phy_direction[] = {
		{0xb4, 0x02},
		{0xb8, 0xb0},
		{0xb8, 0x100b0},
		{0xb4, 0x00},
		{0xb8, 0x000b0},
		{0xb8, 0x0000},
		{0xb4, 0x02},
		{0xb4, 0x00}
	};
	i = mipi_dsih_write_register_configuration(dsi, phy_direction,
						   (sizeof(phy_direction) /
						    sizeof(register_config_t)));
	if (i != (sizeof(phy_direction) / sizeof(register_config_t))) {
		pr_err("ERROR setting up testchip %d", i);
	}
}

static int jz_mipi_update_cfg(struct dsi_device *dsi)
{
	int ret;
	int st_mask = 0;
	int retry = 5;
	ret = jz_dsi_phy_open(dsi);
	if (ret) {
		pr_err("open the phy failed!\n");
		return ret;
	}

	mipi_dsih_write_word(dsi, R_DSI_HOST_CMD_MODE_CFG,
				     0xffffff0);

	/*set command mode */
	mipi_dsih_write_word(dsi, R_DSI_HOST_MODE_CFG, 0x1);
	/*set this register for cmd size, default 0x6 */
	mipi_dsih_write_word(dsi, R_DSI_HOST_EDPI_CMD_SIZE, 0x6);

	/*
	 * jz_dsi_phy_cfg:
	 * PLL programming, config the output freq to DEFAULT_DATALANE_BPS.
	 * */
	ret = jz_dsi_phy_cfg(dsi);
	if (ret) {
		pr_err("phy configigure failed!\n");
		return ret;
	}

	pr_debug("wait for phy config ready\n");
	if (dsi->video_config->no_of_lanes == 2)
		st_mask = 0x95;
	else
		st_mask = 0x15;

	/*checkout phy clk lock and  clklane, datalane stopstate  */
	udelay(10);
	while ((mipi_dsih_read_word(dsi, R_DSI_HOST_PHY_STATUS) & st_mask) !=
			st_mask && retry--) {
		pr_info("phy status = %08x\n", mipi_dsih_read_word(dsi, R_DSI_HOST_PHY_STATUS));
	}

	if (!retry){
		pr_err("wait for phy config failed!\n");
		return ret;
	}

	dsi->state = INITIALIZED;
	return 0;
}

static int jz_mipi_dsi_blank_mode(struct dsi_device *dsi, int power)
{
    switch (power) {
	case FB_BLANK_POWERDOWN:
        if (dsi->suspended)
            return 0;

        jz_dsih_dphy_clock_en(dsi, 0);
        jz_dsih_dphy_shutdown(dsi, 0);
        clk_disable_unprepare(dsi->clk);
        mipi_dsih_hal_power(dsi, 0);

        dsi->state = NOT_INITIALIZED;
        dsi->suspended = true;

        break;
    case FB_BLANK_UNBLANK:
        if (!dsi->suspended)
            return 0;

        clk_prepare_enable(dsi->clk);
        jz_mipi_update_cfg(dsi);

        dsi->suspended = false;
        break;
    case FB_BLANK_NORMAL:
        /* TODO. */
        break;


    default:
        break;
    }

    return 0;
}

int jz_dsi_mode_cfg(struct dsi_device *dsi, int mode)
{
	struct video_config *video_config;
	struct dsi_config *dsi_config;
	video_config = dsi->video_config;
	dsi_config = dsi->dsi_config;

	if(mode == 1) {
		/* video mode */
		jz_dsih_hal_power(dsi, 0);
		jz_dsi_video_cfg(dsi);
		jz_dsih_hal_power(dsi, 1);
	} else {
		mipi_dsih_write_word(dsi, R_DSI_HOST_EDPI_CMD_SIZE, 1024); /* 当设置太小时, 对于M31传输数据会有卡死的现象??, 这里设置成1024。*/
		mipi_dsih_dphy_enable_hs_clk(dsi, 1);
		mipi_dsih_dphy_auto_clklane_ctrl(dsi, 1);
		mipi_dsih_write_word(dsi, R_DSI_HOST_CMD_MODE_CFG, 1);

		/* cmd mode */
		/* color coding fix to 24bit ???? */
		mipi_dsih_hal_dpi_frame_ack_en(dsi, video_config->receive_ack_packets);
		if (video_config->receive_ack_packets) {	/* if ACK is requested, enable BTA, otherwise leave as is */
			mipi_dsih_hal_bta_en(dsi, 1);
		}
		if(dsi_config->te_mipi_en) {
			mipi_dsih_hal_tear_effect_ack_en(dsi, 1);
		} else {
			mipi_dsih_hal_tear_effect_ack_en(dsi, 0);
		}
		mipi_dsih_hal_gen_set_mode(dsi, 1);
		mipi_dsih_hal_dpi_color_coding(dsi, dsi->video_config->color_coding);
	}

	return 0;
}

static int jz_dsi_query_te(struct dsi_device *dsi)
{
	struct dsi_cmd_packet set_tear_on = {0x15, 0x35, 0x00};
	/* unsigned int cmd_mode_cfg; */

	if(!dsi->dsi_config->te_mipi_en)
		return 0;

	//return 0;

	/* According to DSI host spec. */
	/*
	Tearing effect request must always be triggered by a set_tear_on
	command in the DWC_mipi_dsi_host implementation
	*/
	write_command(dsi, &set_tear_on);

	return 0;

}

static int jz_dsi_mode_init(struct dsi_device *dsi,
		struct jzdsi_data *pdata)
{
	struct dsi_phy *dsi_phy = dsi->dsi_phy;
	struct drm_display_mode *mode = pdata->mode;

	dsi->dsi_config = &(pdata->dsi_config);
	dsi_phy->status = NOT_INITIALIZED;
	dsi_phy->reference_freq = REFERENCE_FREQ;
	dsi_phy->bsp_pre_config = set_base_dir_tx;
	dsi->video_config = &(pdata->video_config);

	dsi->video_config->pixel_clock = mode->clock;
	dsi->video_config->h_polarity = !!(mode->flags & DRM_MODE_FLAG_PHSYNC);
	dsi->video_config->v_polarity =  !!(mode->flags & DRM_MODE_FLAG_PVSYNC);

	dsi->video_config->h_active_pixels = mode->hdisplay;
	dsi->video_config->h_sync_pixels = mode->hsync_end - mode->hsync_start;
	dsi->video_config->h_back_porch_pixels = mode->htotal - mode->hsync_end;
	dsi->video_config->h_total_pixels = mode->htotal;

	dsi->video_config->v_active_lines = mode->vdisplay;
	dsi->video_config->v_sync_lines = mode->vsync_end - mode->vsync_start;
	dsi->video_config->v_back_porch_lines = mode->vtotal - mode->vsync_end;
	dsi->video_config->v_total_lines = mode->vtotal;

	pr_debug("dsi->video_config->h_total_pixels: %d\n", dsi->video_config->h_total_pixels);
	pr_debug("dsi->video_config->v_total_lines: %d\n", dsi->video_config->v_total_lines);
	pr_debug("jzfb_pdata.modes->refresh: %d\n", mode->vrefresh);
	pr_debug("jzfb_pdata.bpp: %d\n", pdata->bpp_info);
	pr_debug("dsi->video_config->no_of_lanes: %d\n", dsi->video_config->no_of_lanes);
	pr_debug("---dsi->video_config->byte_clock: %d\n", dsi->video_config->byte_clock);

	if(!dsi->video_config->byte_clock) {
		dsi->video_config->byte_clock = dsi->video_config->h_total_pixels * dsi->video_config->v_total_lines * mode->vrefresh * pdata->bpp_info / dsi->video_config->no_of_lanes / 8 / 1000 ;
		switch(dsi->video_config->byte_clock_coef) {
		case MIPI_PHY_BYTE_CLK_COEF_MUL1:
			break;
		case MIPI_PHY_BYTE_CLK_COEF_MUL3_DIV2:
			dsi->video_config->byte_clock = dsi->video_config->byte_clock * 3 / 2;
			break;
		case MIPI_PHY_BYTE_CLK_COEF_MUL4_DIV3:
			dsi->video_config->byte_clock = dsi->video_config->byte_clock * 4 / 3;
			break;
		case MIPI_PHY_BYTE_CLK_COEF_MUL5_DIV4:
			dsi->video_config->byte_clock = dsi->video_config->byte_clock * 5 / 4;
			break;
		case MIPI_PHY_BYTE_CLK_COEF_MUL6_DIV5:
			dsi->video_config->byte_clock = dsi->video_config->byte_clock * 6 / 5;
			break;
		default:
			break;
		}
	}
	pr_debug("---dsi->video_config->byte_clock: %d pixclock = %d\n", dsi->video_config->byte_clock, mode->clock);

	if (dsi->video_config->byte_clock * 8 > dsi->dsi_config->max_bps * 1000) {
		dsi->video_config->byte_clock = dsi->dsi_config->max_bps * 1000 / 8;
		pr_err("+++++++++++++warning: DATALANE_BPS is over lcd max_bps allowed ,auto set it lcd max_bps\n");
	}

	return 0;
}

/* define MIPI-DSI Master operations. */
static struct dsi_master_ops jz_master_ops = {
    .mode_init	    = jz_dsi_mode_init,
    .mode_cfg	    = jz_dsi_mode_cfg,
    .cmd_write      = write_command,	/*jz_dsi_wr_data, */
    .query_te	    = jz_dsi_query_te,
    .set_blank_mode = jz_mipi_dsi_blank_mode,
};

struct dsi_device * jzdsi_init(void)
{
	struct dsi_device *dsi;
	struct dsi_phy *dsi_phy;
	int ret = -EINVAL;
	pr_debug("entry %s()\n", __func__);

	dsi = (struct dsi_device *)kzalloc(sizeof(struct dsi_device), GFP_KERNEL);
	if (!dsi) {
		pr_err("dsi->dev: failed to allocate dsi object.\n");
		ret = -ENOMEM;
		goto err_dsi;
	}

	dsi_phy = (struct dsi_phy *)kzalloc(sizeof(struct dsi_phy), GFP_KERNEL);
	if (!dsi_phy) {
		pr_err("dsi->dev: failed to allocate dsi phy  object.\n");
		ret = -ENOMEM;
		goto err_dsi_phy;
	}

	dsi->clk = clk_get(NULL, "gate_dsi");
	if (IS_ERR(dsi->clk)) {
		pr_err("failed to get dsi clock source\n");
		goto err_put_clk;
	}

	dsi->address = (unsigned int)ioremap(DSI_IOBASE, 0x190);
	if (!dsi->address) {
		pr_err("Failed to ioremap register dsi memory region\n");
		goto err_iounmap;
	}
	dsi->phy_address = (unsigned int)ioremap(DSI_PHY_IOBASE, 0x200);
	if(!dsi->phy_address) {
		pr_err("Failed to ioremap dsi phy memory region!\n");
		goto err_iounmap;
	}

	mutex_init(&dsi->lock);
	dsi->master_ops = &jz_master_ops;
	dsi->dsi_phy = dsi_phy;
        dsi->suspended = true;

	return dsi;

err_iounmap:
	clk_put(dsi->clk);
err_put_clk:
	kfree(dsi_phy);
err_dsi_phy:
	kfree(dsi);
err_dsi:
	return NULL;
}

void jzdsi_remove(struct dsi_device *dsi)
{
	struct dsi_phy *dsi_phy;
	dsi_phy = dsi->dsi_phy;

	iounmap((void __iomem *)dsi->address);
	clk_disable(dsi->clk);
	clk_put(dsi->clk);
	kfree(dsi_phy);
	kfree(dsi);
}
