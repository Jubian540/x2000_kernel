#include <linux/videodev2.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "isp-drv.h"

static inline int apical_isp_hflip_s_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	int ret = 0;
	unsigned int value = 0;

	value = control->value;

	tisp_mirror_enable(&core->core_tuning, value);
	isp->ctrls.hflip = value;

	return ret;
}

static inline int apical_isp_hflip_g_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	int ret = 0;
	control->value = isp->ctrls.hflip;
	return ret;
}

static inline int apical_isp_vflip_s_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	int ret = 0;
	unsigned int value = 0;

	value = control->value;

	tisp_flip_enable(&core->core_tuning, value);
	isp->ctrls.vflip = value;

	return ret;
}

static inline int apical_isp_vflip_g_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	int ret = 0;
	control->value = isp->ctrls.vflip;
	return ret;
}

static inline int apical_isp_sat_s_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	struct ispcam_device *ispcam = isp->ispcam;
	struct vic_device *vic = ispcam->vic;
	struct v4l2_subdev_format *input_fmt = &vic->formats[VIC_PAD_SINK];
	unsigned int value = 128;
	int ret = 0;

	if(input_fmt->format.code == MEDIA_BUS_FMT_YUYV8_1X16){
		return ret;
	}
	/* the original value */
	value = control->value & 0xff;
	tisp_set_saturation(&core->core_tuning, value);

	return ret;
}

static inline int apical_isp_sat_g_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	struct ispcam_device *ispcam = isp->ispcam;
	struct vic_device *vic = ispcam->vic;
	struct v4l2_subdev_format *input_fmt = &vic->formats[VIC_PAD_SINK];
	unsigned int value = 128;
	int ret = 0;

	if(input_fmt->format.code == MEDIA_BUS_FMT_YUYV8_1X16){
		return ret;
	}
	/* the original value */
	value = tisp_get_saturation(&core->core_tuning);
	control->value = value;

	return ret;
}

static inline int apical_isp_bright_s_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	struct ispcam_device *ispcam = isp->ispcam;
	struct vic_device *vic = ispcam->vic;
	struct v4l2_subdev_format *input_fmt = &vic->formats[VIC_PAD_SINK];
	unsigned int value = 128;
	int ret = 0;

	if(input_fmt->format.code == MEDIA_BUS_FMT_YUYV8_1X16){
		return ret;
	}
	/* the original value */
	value = control->value & 0xff;
	tisp_set_brightness(&core->core_tuning, value);

	return ret;
}

static inline int apical_isp_bright_g_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	struct ispcam_device *ispcam = isp->ispcam;
	struct vic_device *vic = ispcam->vic;
	struct v4l2_subdev_format *input_fmt = &vic->formats[VIC_PAD_SINK];
	unsigned int value = 128;
	int ret = 0;

	if(input_fmt->format.code == MEDIA_BUS_FMT_YUYV8_1X16){
		return ret;
	}
	/* the original value */
	value = tisp_get_brightness(&core->core_tuning);
	control->value = value;

	return ret;
}

static inline int apical_isp_contrast_s_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	struct ispcam_device *ispcam = isp->ispcam;
	struct vic_device *vic = ispcam->vic;
	struct v4l2_subdev_format *input_fmt = &vic->formats[VIC_PAD_SINK];
	unsigned int value = 128;
	int ret = 0;

	if(input_fmt->format.code == MEDIA_BUS_FMT_YUYV8_1X16){
		return ret;
	}
	/* the original value */
	value = control->value & 0xff;
	tisp_set_contrast(&core->core_tuning, value);

	return ret;
}

static inline int apical_isp_contrast_g_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	struct ispcam_device *ispcam = isp->ispcam;
	struct vic_device *vic = ispcam->vic;
	struct v4l2_subdev_format *input_fmt = &vic->formats[VIC_PAD_SINK];
	unsigned int value = 128;
	int ret = 0;

	if(input_fmt->format.code == MEDIA_BUS_FMT_YUYV8_1X16){
		return ret;
	}
	/* the original value */
	value = tisp_get_contrast(&core->core_tuning);
	control->value = value;

	return ret;
}

static inline int apical_isp_sharp_s_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	struct ispcam_device *ispcam = isp->ispcam;
	struct vic_device *vic = ispcam->vic;
	struct v4l2_subdev_format *input_fmt = &vic->formats[VIC_PAD_SINK];
	unsigned int value = 128;
	int ret = 0;

	if(input_fmt->format.code == MEDIA_BUS_FMT_YUYV8_1X16){
		return ret;
	}
	/* the original value */
	value = control->value & 0xff;
	tisp_set_sharpness(&core->core_tuning, value);

	return ret;
}

static inline int apical_isp_sharp_g_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	struct ispcam_device *ispcam = isp->ispcam;
	struct vic_device *vic = ispcam->vic;
	struct v4l2_subdev_format *input_fmt = &vic->formats[VIC_PAD_SINK];
	unsigned int value = 128;
	int ret = 0;

	if(input_fmt->format.code == MEDIA_BUS_FMT_YUYV8_1X16){
		return ret;
	}
	/* the original value */
	value = tisp_get_sharpness(&core->core_tuning);
	control->value = value;

	return ret;
}

static inline int flicker_value_v4l2_to_tuning(int val)
{
	int ret = 0;
	switch(val){
	case V4L2_CID_POWER_LINE_FREQUENCY_DISABLED:
		ret = 0;
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY_50HZ:
		ret = 50;
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY_60HZ:
		ret = 60;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int ispcore_exp_auto_s_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	tisp_ev_attr_t ae_attr;
	int ret = 0;
	/*
	 * control->value.
	 *	0: 自动曝光.
	 *	1: 手动曝光.
	 *	2. 快门优先.
	 *	3. 光圈优先.
	 * */

	ret = tisp_g_ev_attr(&core->core_tuning, &ae_attr);
	if(control->value == V4L2_EXPOSURE_AUTO) {
		ae_attr.manual_it = 0;
	} else if(control->value == V4L2_EXPOSURE_MANUAL) {
		ae_attr.manual_it = 1;
	}

	ret = tisp_s_ae_attr(&core->core_tuning, ae_attr);

	return ret;
}

/* 获取当前曝光方式.*/
static int ispcore_exp_auto_g_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	tisp_ev_attr_t ae_attr;
	int ret = 0;

	ret = tisp_g_ev_attr(&core->core_tuning, &ae_attr);

	control->value = ae_attr.manual_it == 0 ? V4L2_EXPOSURE_AUTO : V4L2_EXPOSURE_MANUAL;

	return ret;
}

/* 设置曝光等级. eg: [-4, 4]
 *
 *  相对曝光时间. 从 [-4, 4] -> [inte_min, inte_max]
 * */
static int ispcore_exp_s_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	tisp_ev_attr_t ae_attr;

}

static int ispcore_exp_g_control(tisp_core_t *core, struct v4l2_control *control)
{

}

/* 设置曝光时间. */
static int ispcore_exp_abs_s_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	tisp_ev_attr_t ae_attr;
	int ret = 0;

	ret = tisp_g_ev_attr(&core->core_tuning, &ae_attr);
	if(ae_attr.manual_it) {
		ae_attr.integration_time = control->value;
		ret = tisp_s_ae_attr(&core->core_tuning, ae_attr);
	} else {
		dev_warn(isp->dev, "set exp_abs while in exposure [auto] mode.\n");
	}

	return ret;

}

static int ispcore_exp_abs_g_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	tisp_ev_attr_t ae_attr;
	int ret = 0;

	ret = tisp_g_ev_attr(&core->core_tuning, &ae_attr);

	control->value = ae_attr.integration_time;

	return ret;
}

/*设置自动增益控制方式
 * 0: 自动增益控制
 * 1: 手动增益控制.
 * */
static inline int ispcore_auto_gain_s_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	tisp_ev_attr_t ae_attr;
	int ret = 0;
	//ret = tisp_g_ev_attr(&core->core_tuning, &ae_attr);

	if(control->value == 0 ) {
		ae_attr.manual_ag = 0;
		ae_attr.again=2048;
	} else if (control->value == 1 ){
		ae_attr.manual_ag = 1;
	}

	ret = tisp_s_ae_attr_ag(&core->core_tuning, ae_attr);

	return ret;
}

/*获取自动增益控制方式.*/
static inline int ispcore_auto_gain_g_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	tisp_ev_attr_t ae_attr;
	int ret = 0;
	ret = tisp_g_ev_attr(&core->core_tuning, &ae_attr);

	control->value = ae_attr.manual_ag;

	return ret;
}

/*手动增益控制模式下，设置当前增益.*/
static inline int ispcore_gain_s_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	tisp_ev_attr_t ae_attr;
	int ret = 0;
	//ret = tisp_g_ev_attr(&core->core_tuning, &ae_attr);

	if(ae_attr.manual_ag) {
		ae_attr.again = control->value;
		ret = tisp_s_ae_attr_ag(&core->core_tuning, ae_attr);
	}else{
		dev_warn(isp->dev, "set gain_s warning while in gain mode.\n");
	}

	return ret;
}
static inline int ispcore_gain_g_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	tisp_ev_attr_t ae_attr;
	int ret = 0;
	ret = tisp_g_ev_attr(&core->core_tuning, &ae_attr);

	control->value = ae_attr.again;

	return ret;
}

static inline int ispcore_wb_s_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	tisp_wb_attr_t wb_attr;
	int ret = 0;
	ret = tisp_g_wb_attr(&core->core_tuning, &wb_attr);
	switch(control->value){
		case V4L2_WHITE_BALANCE_MANUAL:
			wb_attr.tisp_wb_manual=0;
			break;
		case V4L2_WHITE_BALANCE_AUTO:
			wb_attr.tisp_wb_manual=1;
			break;
		case V4L2_WHITE_BALANCE_INCANDESCENT:
			wb_attr.tisp_wb_manual=2;
			break;
		case V4L2_WHITE_BALANCE_FLUORESCENT:
			wb_attr.tisp_wb_manual=3;
			break;
		case V4L2_WHITE_BALANCE_FLUORESCENT_H:
			wb_attr.tisp_wb_manual=4;
			break;
		case V4L2_WHITE_BALANCE_HORIZON:
			wb_attr.tisp_wb_manual=5;
			break;
		case V4L2_WHITE_BALANCE_DAYLIGHT:
			wb_attr.tisp_wb_manual=6;
			break;
		case V4L2_WHITE_BALANCE_FLASH:
			wb_attr.tisp_wb_manual=7;
			break;
		case V4L2_WHITE_BALANCE_CLOUDY:
			wb_attr.tisp_wb_manual=8;
			break;
		case V4L2_WHITE_BALANCE_SHADE:
			wb_attr.tisp_wb_manual=9;
			break;
		default:
			break;
	}
	ret = tisp_s_wb_attr(&core->core_tuning, wb_attr);

	return ret;
}
static inline int ispcore_wb_g_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	tisp_wb_attr_t wb_attr;
	int ret = 0;
	ret = tisp_g_wb_attr(&core->core_tuning, &wb_attr);
	control->value=wb_attr.tisp_wb_manual;

	return ret;
}

static inline int apical_isp_hilightdepress_s_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	struct ispcam_device *ispcam = isp->ispcam;
	struct vic_device *vic = ispcam->vic;
	struct v4l2_subdev_format *input_fmt = &vic->formats[VIC_PAD_SINK];
	unsigned int value = 0;
	int ret = 0;

	if(input_fmt->format.code == MEDIA_BUS_FMT_YUYV8_1X16){
		return ret;
	}

	value = control->value;

	ret=tisp_s_Hilightdepress(&core->core_tuning,value);

	return ret;
}
static inline int apical_isp_hilightdepress_g_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	struct ispcam_device *ispcam = isp->ispcam;
	struct vic_device *vic = ispcam->vic;
	struct v4l2_subdev_format *input_fmt = &vic->formats[VIC_PAD_SINK];
	int ret=0;

	if(input_fmt->format.code == MEDIA_BUS_FMT_YUYV8_1X16){
		return -1;
	}

	ret=tisp_g_Hilightdepress(&core->core_tuning,&control->value);

	return 0;
}

static inline int apical_isp_flicker_s_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	int ret = 0;

	ret = tisp_s_antiflick(&core->core_tuning, flicker_value_v4l2_to_tuning(control->value));
	if(ret != 0)
		dev_err(isp->dev, "set flicker failed!!!\n");
	isp->ctrls.flicker = control->value;

	return ret;
}

static inline int apical_isp_flicker_g_control(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	int ret = 0;
	control->value = isp->ctrls.flicker;
	return ret;
}

static int apical_isp_module_s_attr(tisp_core_t *core, struct v4l2_control *control)
{
	tisp_module_control_t module;

	module.key = control->value;
	tisp_s_module_control(&core->core_tuning, module);

	return 0;
}

static int apical_isp_module_g_attr(tisp_core_t *core, struct v4l2_control *control)
{
	tisp_module_control_t module;
	int ret = 0;

	tisp_g_module_control(&core->core_tuning, &module);
	control->value = module.key;

	return ret;
}

static inline int apical_isp_day_or_night_g_ctrl(tisp_core_t *core, struct v4l2_control *control)
{

	struct isp_device *isp = core->priv_data;
	struct image_tuning_ctrls *ctrls = &(isp->ctrls);
	TISP_MODE_DN_E dn = isp->ctrls.daynight;
	control->value = dn;

	return 0;
}

static inline int apical_isp_day_or_night_s_ctrl(tisp_core_t *core, struct v4l2_control *control)
{
	struct isp_device *isp = core->priv_data;
	struct image_tuning_ctrls *ctrls = &(isp->ctrls);
	int ret = 0;

	TISP_MODE_DN_E dn = control->value;

	if(dn != isp->ctrls.daynight){
		isp->ctrls.daynight = dn;
		tisp_day_or_night_s_ctrl(&core->core_tuning, dn);
	}

	return ret;
}

static int apical_isp_ae_luma_g_ctrl(tisp_core_t *core, struct v4l2_control *control)
{
	unsigned char luma;
	struct isp_device *isp = core->priv_data;
	struct v4l2_subdev_format *input_fmt = &isp->formats[ISP_PAD_SINK];
	int width = input_fmt->format.width;
	int height = input_fmt->format.height;

	tisp_g_ae_luma(&core->core_tuning, &luma, width, height);
	control->value = luma;

	return 0;
}


int isp_video_g_ctrl(struct file *file, void *fh, struct v4l2_control *a)
{
	struct isp_video_device *ispvideo = video_drvdata(file);
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	struct isp_device 	*isp	= ispcam->isp;
	tisp_core_t *core = &isp->core;
	int ret = 0;

	switch(a->id){
		case V4L2_CID_HFLIP:
			ret = apical_isp_hflip_g_control(core, a);
			break;
		case V4L2_CID_VFLIP:
			ret = apical_isp_vflip_g_control(core, a);
			break;
		case V4L2_CID_SATURATION:
			ret = apical_isp_sat_g_control(core, a);
			break;
		case V4L2_CID_BRIGHTNESS:
			ret = apical_isp_bright_g_control(core, a);
			break;
		case V4L2_CID_CONTRAST:
			ret = apical_isp_contrast_g_control(core, a);
			break;
		case V4L2_CID_SHARPNESS:
			ret = apical_isp_sharp_g_control(core, a);
			break;
		case V4L2_CID_EXPOSURE_AUTO:
			ret = ispcore_exp_auto_g_control(core, a);
			break;
		case V4L2_CID_EXPOSURE_ABSOLUTE:
			ret = ispcore_exp_abs_g_control(core, a);
			break;
		case V4L2_CID_EXPOSURE:
			ret = ispcore_exp_g_control(core, a);
			break;
		case V4L2_CID_AUTOGAIN:
			ret = ispcore_auto_gain_g_control(core, a);
			break;
		case V4L2_CID_GAIN:
			ret = ispcore_gain_g_control(core, a);
			break;
		case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
			ret = ispcore_wb_g_control(core, a);
			break;
		case IMAGE_TUNING_CID_HILIGHTDEPRESS:
			ret = apical_isp_hilightdepress_g_control(core, a);
			break;
		case V4L2_CID_POWER_LINE_FREQUENCY:
			ret = apical_isp_flicker_g_control(core, a);
			break;
		case IMAGE_TUNING_CID_MODULE_CONTROL:
			ret = apical_isp_module_g_attr(core, a);
			break;
		case IMAGE_TUNING_CID_DAY_OR_NIGHT:
			ret = apical_isp_day_or_night_g_ctrl(core, a);
			break;
		case IMAGE_TUNING_CID_AE_LUMA:
			ret = apical_isp_ae_luma_g_ctrl(core, a);
			break;
		default:
			ret = -EPERM;
			break;
	}

	return ret;
}


int isp_video_s_ctrl(struct file *file, void *fh, struct v4l2_control *a)
{
	struct isp_video_device *ispvideo = video_drvdata(file);
	struct ispcam_device 	*ispcam = ispvideo->ispcam;
	struct isp_device 	*isp	= ispcam->isp;
	tisp_core_t *core = &isp->core;
	int ret = 0;

	switch (a->id) {
		case V4L2_CID_HFLIP:
			ret = apical_isp_hflip_s_control(core, a);
			break;
		case V4L2_CID_VFLIP:
			ret = apical_isp_vflip_s_control(core, a);
			break;
		case V4L2_CID_SATURATION:
			ret = apical_isp_sat_s_control(core, a);
			break;
		case V4L2_CID_BRIGHTNESS:
			ret = apical_isp_bright_s_control(core, a);
			break;
		case V4L2_CID_CONTRAST:
			ret = apical_isp_contrast_s_control(core, a);
			break;
		case V4L2_CID_SHARPNESS:
			ret = apical_isp_sharp_s_control(core, a);
			break;
		case V4L2_CID_EXPOSURE_AUTO:
			ret = ispcore_exp_auto_s_control(core, a);
			break;
		case V4L2_CID_EXPOSURE_ABSOLUTE:
			ret = ispcore_exp_abs_s_control(core, a);
			break;
		case V4L2_CID_EXPOSURE:
			ret = ispcore_exp_s_control(core, a);
			break;
		case V4L2_CID_AUTOGAIN:
			ret = ispcore_auto_gain_s_control(core, a);
			break;
		case V4L2_CID_GAIN:
			ret = ispcore_gain_s_control(core, a);
			break;
		case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
			ret = ispcore_wb_s_control(core, a);
			break;
		case IMAGE_TUNING_CID_HILIGHTDEPRESS:
			ret = apical_isp_hilightdepress_s_control(core, a);
			break;
			break;
		case V4L2_CID_POWER_LINE_FREQUENCY:
			ret = apical_isp_flicker_s_control(core, a);
			break;
		case IMAGE_TUNING_CID_MODULE_CONTROL:
			ret = apical_isp_module_s_attr(core, a);
			break;
		case IMAGE_TUNING_CID_DAY_OR_NIGHT:
			ret = apical_isp_day_or_night_s_ctrl(core, a);
			break;
		default:
			ret = -EPERM;
			break;
	}
	return ret;
}

