#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/suspend.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/gpio.h>
#include <asm/cacheflush.h>
#include <linux/of_address.h>

#include "hw_composer.h"

struct hw_composer_master *hw_comp_master;

struct hw_composer_master * hw_composer_init(struct dpu_ctrl *dctrl)
{
	int ret = 0;

	hw_comp_master = kzalloc(sizeof(struct hw_composer_master), GFP_KERNEL);
	if(IS_ERR_OR_NULL(hw_comp_master)) {
		return -ENOMEM;
	}

	hw_comp_master->dctrl = dctrl;

	/*comp mutex??*/
	mutex_init(&hw_comp_master->lock);

	return hw_comp_master;
}


void hw_composer_exit(struct hw_composer_master *comp_master)
{
	mutex_destroy(&comp_master->lock);

	if(comp_master) {
		kfree(comp_master);
	}
}

struct hw_composer_ctx *hw_composer_create(void *data)
{
	struct hw_composer_ctx *ctx = kzalloc(sizeof(struct hw_composer_ctx), GFP_KERNEL);

	if(IS_ERR_OR_NULL(ctx)) {
		return -ENOMEM;
	}
	/*设置ctx的master.*/
	ctx->master = hw_comp_master;

	ctx->block = 1;

	INIT_LIST_HEAD(&ctx->list);

	/*添加到ctx链表.*/

	return ctx;
}

int hw_composer_destroy(struct hw_composer_ctx *ctx)
{

	list_del(&ctx->list);
	kfree(ctx);

	return 0;
}

#if 0
static int hw_composer_check_frm_cfg(struct hw_composer_ctx *ctx, struct ingenicfb_frm_cfg *frm_cfg)
{
	struct fb_var_screeninfo *var = &info->var;
	struct ingenicfb_lay_cfg *lay_cfg;
	struct fb_videomode *mode;
	int scale_num = 0;
	int i;

	lay_cfg = frm_cfg->lay_cfg;

	if((!(lay_cfg[0].lay_en || lay_cfg[1].lay_en || lay_cfg[2].lay_en || lay_cfg[3].lay_en)) ||
	   (lay_cfg[0].lay_z_order == lay_cfg[1].lay_z_order) ||
	   (lay_cfg[1].lay_z_order == lay_cfg[2].lay_z_order) ||
	   (lay_cfg[2].lay_z_order == lay_cfg[3].lay_z_order)) {
		dev_err(info->dev,"%s %d frame[0] cfg value is err!\n",__func__,__LINE__);
		return -EINVAL;
	}

	switch (lay_cfg[0].format) {
	case LAYER_CFG_FORMAT_RGB555:
	case LAYER_CFG_FORMAT_ARGB1555:
	case LAYER_CFG_FORMAT_RGB565:
	case LAYER_CFG_FORMAT_RGB888:
	case LAYER_CFG_FORMAT_ARGB8888:
	case LAYER_CFG_FORMAT_YUV422:
	case LAYER_CFG_FORMAT_NV12:
	case LAYER_CFG_FORMAT_NV21:
		break;
	default:
		dev_err(info->dev,"%s %d frame[0] cfg value is err!\n",__func__,__LINE__);
		return -EINVAL;
	}

	for(i = 0; i < DPU_SUPPORT_MAX_LAYERS; i++) {
		if(lay_cfg[i].lay_en) {
			if((lay_cfg[i].source_w > DPU_MAX_SIZE) ||
			   (lay_cfg[i].source_w < DPU_MIN_SIZE) ||
			   (lay_cfg[i].source_h > DPU_MAX_SIZE) ||
			   (lay_cfg[i].source_h < DPU_MIN_SIZE) ||
			   (lay_cfg[i].disp_pos_x > mode->xres) ||
			   (lay_cfg[i].disp_pos_y > mode->yres) ||
			   (lay_cfg[i].color > LAYER_CFG_COLOR_BGR) ||
			   (lay_cfg[i].stride > DPU_STRIDE_SIZE)) {
				dev_err(info->dev,"%s %d frame[0] cfg value is err!\n",__func__,__LINE__);
				return -EINVAL;
			}
			switch (lay_cfg[i].format) {
			case LAYER_CFG_FORMAT_RGB555:
			case LAYER_CFG_FORMAT_ARGB1555:
			case LAYER_CFG_FORMAT_RGB565:
			case LAYER_CFG_FORMAT_RGB888:
			case LAYER_CFG_FORMAT_ARGB8888:
			case LAYER_CFG_FORMAT_YUV422:
			case LAYER_CFG_FORMAT_NV12:
			case LAYER_CFG_FORMAT_NV21:
				break;
			default:
				dev_err(info->dev,"%s %d frame[%d] cfg value is err!\n",__func__,__LINE__, i);
				return -EINVAL;
			}
			if(lay_cfg[i].lay_scale_en) {
				scale_num++;
				if((lay_cfg[i].scale_w > mode->xres) ||
					(lay_cfg[i].scale_w < DPU_SCALE_MIN_SIZE) ||
					(lay_cfg[i].scale_h > mode->yres) ||
					(lay_cfg[i].scale_h < DPU_SCALE_MIN_SIZE) ||
					(lay_cfg[i].scale_w + lay_cfg[i].disp_pos_x > mode->xres) ||
					(lay_cfg[i].scale_h + lay_cfg[i].disp_pos_y > mode->yres) ||
					(scale_num > 2)) {
					dev_err(info->dev,"%s %d frame[%d] cfg value is err!\n",
							__func__, __LINE__,i);
					return -EINVAL;
				}
			} else {
				if((lay_cfg[i].disp_pos_x + lay_cfg[i].source_w > mode->xres) ||
				(lay_cfg[i].disp_pos_y + lay_cfg[i].source_h > mode->yres)) {
					dev_err(info->dev,"%s %d frame[%d] cfg value is err!\n",
							__func__, __LINE__,i);
					return -EINVAL;
				}
			}
		}
	}

	return 0;
}
#endif

void hw_composer_lock(struct hw_composer_master *master)
{
	mutex_lock(&master->lock);
}

void hw_composer_unlock(struct hw_composer_master *master)
{
	mutex_unlock(&master->lock);
}

int hw_composer_setup(struct hw_composer_ctx *ctx, struct comp_setup_info *comp_info)
{
	/*TODO: check comp_info.*/

	memcpy(&ctx->comp_info, comp_info, sizeof(struct comp_setup_info));
	return 0;
}


int hw_composer_start(struct hw_composer_ctx *ctx)
{
	struct hw_composer_master *master = ctx->master;
	struct dpu_ctrl *dctrl = master->dctrl;
	int ret = 0;



	ret = dpu_ctrl_comp_setup(dctrl, &ctx->comp_info);


	ret = dpu_ctrl_comp_start(dctrl, ctx->block);

	return ret;
}

int hw_composer_stop(struct hw_composer_ctx *ctx)
{
	struct hw_composer_master *master = ctx->master;
	struct dpu_ctrl *dctrl = master->dctrl;
	int ret = 0;

	// TODO:
	dpu_ctrl_comp_stop(dctrl, QCK_STOP);

	return ret;
}
