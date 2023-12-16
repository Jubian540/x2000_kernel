/*
 * ov2735b Camera Driver
 *
 * Copyright (C) 2010 Alberto Panizzo <maramaopercheseimorto@gmail.com>
 *
 * Based on ov772x, ov9640 drivers and previous non merged implementations.
 *
 * Copyright 2005-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006, OmniVision
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/of_gpio.h>
#include <linux/v4l2-mediabus.h>
#include <linux/videodev2.h>

#include <media/soc_camera.h>
#include <media/v4l2-clk.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-image-sizes.h>

#include <linux/clk.h>
#include <linux/regulator/consumer.h>

#define OV2735B_CHIP_ID_H       (0x27)
#define OV2735B_CHIP_ID_L       (0x35)
#define OV2735B_REG_END         0xff
#define OV2735B_REG_DELAY       0x00
#define OV2735B_PAGE_REG            0xfd

#define  ov2735b_DEFAULT_WIDTH    720
#define  ov2735b_DEFAULT_HEIGHT   480

/* Private v4l2 controls */
#define V4L2_CID_PRIVATE_BALANCE  (V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_PRIVATE_EFFECT  (V4L2_CID_PRIVATE_BASE + 1)

#define ov2735b_FLIP_VAL			((unsigned char)0x04)
#define ov2735b_FLIP_MASK		((unsigned char)0x04)

/* whether sensor support high resolution (> vga) preview or not */
#define SUPPORT_HIGH_RESOLUTION_PRE		1

/*
 * Struct
 */
struct regval_list {
	uint16_t reg_num;
	unsigned char value;
};

struct mode_list {
	u16 index;
	const struct regval_list *mode_regs;
};

struct ov2735b_win_size {
	char *name;
	int width;
	int height;
	const struct regval_list *regs;
};


struct ov2735b_gpio {
	int pin;
	int active_level;
};

struct ov2735b_priv {
	struct v4l2_subdev		subdev;
	struct v4l2_ctrl_handler	hdl;
	u32	cfmt_code;
	struct v4l2_clk			*clk;
	const struct ov2735b_win_size	*win;

	int				model;
	u16				balance_value;
	u16				effect_value;
	u16				flag_vflip:1;
	u16				flag_hflip:1;

	struct soc_camera_subdev_desc	ssdd_dt;
	struct gpio_desc *resetb_gpio;
	struct gpio_desc *pwdn_gpio;
	struct gpio_desc *vcc_en_gpio;

	struct regulator *reg;
	struct regulator *reg1;
};

static int ov2735b_s_power(struct v4l2_subdev *sd, int on);

unsigned short ov2735b_read_reg(struct i2c_client *client, unsigned char reg, unsigned char *value)
{
/*
	struct i2c_msg msg[2] = {
		[0] = {
			.addr   = client->addr,
			.flags  = 0,
			.len    = 1,
			.buf    = &reg,
		},
		[1] = {
			.addr   = client->addr,
			.flags  = I2C_M_RD,
			.len    = 1,
			.buf    = value,
		}
	};
	int ret;
	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret > 0)
		ret = 0;
*/
	return 0;
}

int ov2735b_write_reg(struct i2c_client *client, unsigned char reg, unsigned short value)
{
	return 0;
}

static struct regval_list ov2735b_1080p_regs[] = {

	{OV2735B_REG_END, 0x00},        /* END MARKER */
};

static struct ov2735b_win_size ov2735b_supported_win_sizes[] = {
	{
		.name = "576",
		.width = ov2735b_DEFAULT_WIDTH,
		.height = ov2735b_DEFAULT_HEIGHT,
		.regs = ov2735b_1080p_regs,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(ov2735b_supported_win_sizes))

static u32 ov2735b_codes[] = {
//	MEDIA_BUS_FMT_YUYV8_2X8,
	MEDIA_BUS_FMT_UYVY8_2X8,
};

/*
 * General functions
 */
static struct ov2735b_priv *to_ar0144(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct ov2735b_priv,
			    subdev);
}
#if 0
static int ov2735b_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != OV2735B_REG_END) {
		if (vals->reg_num == OV2735B_REG_DELAY) {
			msleep(vals->value);
		} else {
			ret = ov2735b_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

#else
static int ov2735b_write_array(struct i2c_client *client,
			      const struct regval_list *vals)
{
	int ret;

	while (vals->reg_num != OV2735B_REG_END) {
		if (vals->reg_num == OV2735B_REG_DELAY) {
			msleep(vals->value);
		} else {
			ret = ov2735b_write_reg(client, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}
#endif
#if 0
static int ov2735b_mask_set(struct i2c_client *client,
			   u16  reg, u16  mask, u16  set)
{
	int ret = 0;
	unsigned char value[2] = {0};
	ret = ov2735b_read_reg(client, reg, value);
	if (ret < 0)
		return ret;

	val &= ~mask;
	val |= set & mask;

	dev_vdbg(&client->dev, "masks: 0x%02x, 0x%02x", reg, val);

	return ov2735b_write_reg(client, reg, val);
}
#endif
static int ov2735b_reset(struct i2c_client *client)
{
	/*
	struct ov2735b_priv *priv = to_ar0144(client);

	gpio_direction_output(priv->reset.pin, priv->reset.active_level);
	msleep(10);
	gpio_direction_output(priv->reset.pin, !priv->reset.active_level);
	msleep(10);
	return 0;
	*/
}

/*
 * soc_camera_ops functions
 */

static struct regval_list ov2735b_stream_on[] = {
	{0xfd, 0x00},
	{0x36, 0x00},
	{0x37, 0x00},//fake stream on

	{OV2735B_REG_END, 0x00},
};

static struct regval_list ov2735b_stream_off[] = {
	{0xfd, 0x00},
	{0x36, 0x01},
	{0x37, 0x01},//fake stream off

	{OV2735B_REG_END, 0x00},
};


static int ov2735b_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);

	if (!enable ) {
		dev_info(&client->dev, "stream down\n");
		ov2735b_write_array(client, ov2735b_stream_off);
		return 0;
	}

	dev_info(&client->dev, "stream on\n");
	ov2735b_write_array(client, ov2735b_stream_on);

	return 0;
}

static int ov2735b_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd =
		&container_of(ctrl->handler, struct ov2735b_priv, hdl)->subdev;
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov2735b_priv *priv = to_ar0144(client);

	switch (ctrl->id) {

	case V4L2_CID_VFLIP:
		ctrl->val = priv->flag_vflip;
		break;
	case V4L2_CID_HFLIP:
		ctrl->val = priv->flag_hflip;
		break;
	default:
		break;
	}
	return 0;
}


static int ov2735b_s_ctrl(struct v4l2_ctrl *ctrl)
{
#if 0
	struct v4l2_subdev *sd =
		&container_of(ctrl->handler, struct ov2735b_priv, hdl)->subdev;
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov2735b_priv *priv = to_ar0144(client);
	int ret = 0;
	int i = 0;
	u16 value;

	int balance_count = ARRAY_SIZE(ov2735b_balance);
	int effect_count = ARRAY_SIZE(ov2735b_effect);

	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
		value = ctrl->val ? ov2735b_FLIP_VAL : 0x00;
		priv->flag_vflip = ctrl->val ? 1 : 0;
//		ret = ov2735b_mask_set(client, 0x301D, ar0144_FLIP_MASK, value);
		break;

	case V4L2_CID_HFLIP:
		value = ctrl->val ? ov2735b_FLIP_VAL : 0x00;
		priv->flag_hflip = ctrl->val ? 1 : 0;
//		ret = ov2735b_mask_set(client, 0x301D, ar0144_FLIP_MASK, value);
		break;

	default:
		dev_err(&client->dev, "no V4L2 CID: 0x%x ", ctrl->id);
		return -EINVAL;
	}

	return ret;
#endif
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov2735b_g_register(struct v4l2_subdev *sd,
			     struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	reg->size = 1;
	if (reg->reg > 0xff)
		return -EINVAL;

	ret = ov2735b_read_reg(client, reg->reg, reg->val);

	return 0;
}

static int ov2735b_s_register(struct v4l2_subdev *sd,
			     const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (reg->reg > 0xff ||
	    reg->val > 0xff)
		return -EINVAL;

	return ov2735b_write_reg(client, reg->reg, reg->val);
}
#endif

/* Select the nearest higher resolution for capture */
static const struct ov2735b_win_size *ov2735b_select_win(u32 *width, u32 *height)
{
	int i, default_size = ARRAY_SIZE(ov2735b_supported_win_sizes) - 1;

	for (i = 0; i < ARRAY_SIZE(ov2735b_supported_win_sizes); i++) {
		if ((*width >= ov2735b_supported_win_sizes[i].width) &&
		    (*height >= ov2735b_supported_win_sizes[i].height)) {
			*width = ov2735b_supported_win_sizes[i].width;
			*height = ov2735b_supported_win_sizes[i].height;
			return &ov2735b_supported_win_sizes[i];
		}
	}

	*width = ov2735b_supported_win_sizes[default_size].width;
	*height = ov2735b_supported_win_sizes[default_size].height;
	return &ov2735b_supported_win_sizes[default_size];
}

static int ov2735b_get_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov2735b_priv *priv = to_ar0144(client);

	if (format->pad)
		return -EINVAL;

	mf->width = ov2735b_DEFAULT_WIDTH;//priv->win->width;
	mf->height = ov2735b_DEFAULT_HEIGHT;//priv->win->height;
	mf->code = priv->cfmt_code;

	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field	= V4L2_FIELD_INTERLACED;

	return 0;
}

static int ov2735b_set_params(struct i2c_client *client, u32 *width, u32 *height, u32 code)
{
	struct ov2735b_priv       *priv = to_ar0144(client);
	int ret;

	int bala_index = priv->balance_value;
	int effe_index = priv->effect_value;

	/* select win */
	priv->win = ov2735b_select_win(width, height);

	/* select format */
	priv->cfmt_code = 0;

	/* reset hardware */
	ov2735b_reset(client);

	/* initialize the sensor with default data */
	dev_dbg(&client->dev, "%s: Init default", __func__);
//	ret = ov2735b_write_array(client, ar0144_init_regs);
//	if (ret < 0)
//		goto err;

#if 0
	/* set balance */
	ret = ov2735b_write_array(client, ar0144_balance[bala_index].mode_regs);
	if (ret < 0)
		goto err;

	/* set effect */
	ret = ov2735b_write_array(client, ar0144_effect[effe_index].mode_regs);
	if (ret < 0)
		goto err;
#endif
	/* set size win */
	printk("priv->win.width = %d\n", priv->win->width);
	printk("priv->win.height = %d\n", priv->win->height);

	ret = ov2735b_write_array(client, priv->win->regs);
	if (ret < 0)
		goto err;

	priv->cfmt_code = code;
	*width = priv->win->width;
	*height = priv->win->height;

	return 0;

err:
	dev_err(&client->dev, "%s: Error %d", __func__, ret);
	ov2735b_reset(client);
	priv->win = NULL;

	return ret;
}

static int ov2735b_set_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct i2c_client *client = v4l2_get_subdevdata(sd);


	if (format->pad)
		return -EINVAL;

	/*
	 * select suitable win, but don't store it
	 */
	ov2735b_select_win(&mf->width, &mf->height);

	mf->field = V4L2_FIELD_INTERLACED;
	mf->colorspace = V4L2_COLORSPACE_JPEG;

	if (format->which == V4L2_SUBDEV_FORMAT_ACTIVE)
		return ov2735b_set_params(client, &mf->width,
					 &mf->height, mf->code);
	cfg->try_fmt = *mf;
	return 0;
}

static int ov2735b_enum_mbus_code(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->pad || code->index >= ARRAY_SIZE(ov2735b_codes))
		return -EINVAL;

	code->code = ov2735b_codes[code->index];
	return 0;
}

static int ov2735b_enum_frame_size(struct v4l2_subdev *sd,
		       struct v4l2_subdev_pad_config *cfg,
		       struct v4l2_subdev_frame_size_enum *fse)
{
	int i, j;
	int num_valid = -1;
	__u32 index = fse->index;

	if(index >= N_WIN_SIZES)
		return -EINVAL;

	j = ARRAY_SIZE(ov2735b_codes);
	while(--j)
		if(fse->code == ov2735b_codes[j])
			break;

	for (i = 0; i < N_WIN_SIZES; i++) {
		if (index == ++num_valid) {
			fse->code = ov2735b_codes[j];
			fse->min_width = ov2735b_supported_win_sizes[index].width;
			fse->max_width = fse->min_width;
			fse->min_height = ov2735b_supported_win_sizes[index].height;
			fse->max_height = fse->min_height;
			return 0;
		}
	}

	return -EINVAL;
}

static int ov2735b_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	a->c.left	= 0;
	a->c.top	= 0;
	a->c.width	= ov2735b_DEFAULT_WIDTH;
	a->c.height	= ov2735b_DEFAULT_HEIGHT;
	a->type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;

	return 0;
}

static int ov2735b_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
{
	a->bounds.left			= 0;
	a->bounds.top			= 0;
	a->bounds.width			= ov2735b_DEFAULT_WIDTH;
	a->bounds.height		= ov2735b_DEFAULT_HEIGHT;
	a->defrect			= a->bounds;
	a->type				= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	a->pixelaspect.numerator	= 1;
	a->pixelaspect.denominator	= 1;

	return 0;
}

static int ov2735b_detect(struct i2c_client *client, unsigned int *ident)
{
	unsigned char v;
	int ret;

	printk("[%d]:%s\n", __LINE__, __func__);
	ret = ov2735b_read_reg(client, 0x02, &v);
	printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != OV2735B_CHIP_ID_H)
		return -ENODEV;

	printk("[%d]:%s\n", __LINE__, __func__);
	ret = ov2735b_read_reg(client, 0x03, &v);
	printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != OV2735B_CHIP_ID_L)
		return -ENODEV;

	printk("[%d]:%s\n", __LINE__, __func__);
	ret = ov2735b_read_reg(client, 0x04, &v);
	printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != 0x05 && v != 0x06 && v != 0x07)
		return -ENODEV;

	printk("[%d]:%s\n", __LINE__, __func__);

	return 0;

}

static int ov2735b_video_probe(struct i2c_client *client)
{
	struct ov2735b_priv *priv = to_ar0144(client);
	int ret = 0;

	printk("[%d]:%s\n", __LINE__, __func__);
	ret = ov2735b_s_power(&priv->subdev, 1);
	if (ret < 0)
		return ret;
	printk("[%d]:%s\n", __LINE__, __func__);
	/*
	 * check and show product ID and manufacturer ID
	 */

#if 0
	printk("[%d]:%s\n", __LINE__, __func__);
	ret = ov2735b_detect(client, NULL);
	if(ret < 0)
	{
		v4l_err(client,
				"chip found @ 0x%x (%s) is not an ov2735b chip.\n",
				client->addr << 1, client->adapter->name);
		goto done;
	}
	printk("[%d]:%s\n", __LINE__, __func__);
#endif

	v4l_info(client, "chip found @ 0x%02x (%s)\n",
			client->addr << 1, client->adapter->name);

//	ret = v4l2_ctrl_handler_setup(&priv->hdl);

done:
	ov2735b_s_power(&priv->subdev, 0);
	return ret;
}

static int ov2735b_s_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);
	struct ov2735b_priv *priv = to_ar0144(client);

	return soc_camera_set_power(&client->dev, ssdd, priv->clk, on);
}

static const struct v4l2_ctrl_ops ov2735b_ctrl_ops = {
	.s_ctrl = ov2735b_s_ctrl,
	.g_volatile_ctrl = ov2735b_g_ctrl,
};

static struct v4l2_subdev_core_ops ov2735b_subdev_core_ops = {
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register	= ov2735b_g_register,
	.s_register	= ov2735b_s_register,
#endif
	.s_power	= ov2735b_s_power,
};

static int ov2735b_g_mbus_config(struct v4l2_subdev *sd,
		struct v4l2_mbus_config *cfg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);

	cfg->flags = V4L2_MBUS_PCLK_SAMPLE_RISING | V4L2_MBUS_MASTER |
		V4L2_MBUS_VSYNC_ACTIVE_LOW | V4L2_MBUS_HSYNC_ACTIVE_HIGH |
		V4L2_MBUS_DATA_ACTIVE_HIGH;
	cfg->type = V4L2_MBUS_BT656;


	cfg->flags = soc_camera_apply_board_flags(ssdd, cfg);
	return 0;
}

static struct v4l2_subdev_video_ops ov2735b_subdev_video_ops = {
	.s_stream	= ov2735b_s_stream,
	.cropcap	= ov2735b_cropcap,
	.g_crop		= ov2735b_g_crop,
	.g_mbus_config	= ov2735b_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops ov2735b_subdev_pad_ops = {
	.enum_mbus_code = ov2735b_enum_mbus_code,
	.enum_frame_size = ov2735b_enum_frame_size,
	.get_fmt	= ov2735b_get_fmt,
	.set_fmt	= ov2735b_set_fmt,
};

static struct v4l2_subdev_ops ov2735b_subdev_ops = {
	.core	= &ov2735b_subdev_core_ops,
	.video	= &ov2735b_subdev_video_ops,
	.pad	= &ov2735b_subdev_pad_ops,
};

/* OF probe functions */
static int ov2735b_hw_power(struct device *dev, int on)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ov2735b_priv *priv = to_ar0144(client);

	dev_dbg(&client->dev, "%s: %s the camera\n",
			__func__, on ? "ENABLE" : "DISABLE");

	/* thses gpio should be set according to the active level in dt defines */
	if(priv->vcc_en_gpio) {
		gpiod_direction_output(priv->vcc_en_gpio, on);
	}

	if (priv->pwdn_gpio) {
		gpiod_direction_output(priv->pwdn_gpio, on);
	}

	msleep(10);
	return 0;
}

static int ov2735b_hw_reset(struct device *dev)
{
	/*
	struct i2c_client *client = to_i2c_client(dev);
	struct ov2735b_priv *priv = to_ar0144(client);

	gpio_direction_output(priv->reset.pin, priv->reset.active_level);
	msleep(10);
	gpio_direction_output(priv->reset.pin, !priv->reset.active_level);
	msleep(10);
	return 0;
	*/
}


static int ov2735b_probe_dt(struct i2c_client *client,
		struct ov2735b_priv *priv)
{

	struct soc_camera_subdev_desc	*ssdd_dt = &priv->ssdd_dt;
	struct v4l2_subdev_platform_data *sd_pdata = &ssdd_dt->sd_pdata;
	struct device_node *np = client->dev.of_node;
	int supplies = 0, index = 0;

	supplies = of_property_count_strings(np, "supplies-name");
	if(supplies <= 0) {
		goto no_supply;
	}

	sd_pdata->num_regulators = supplies;
	sd_pdata->regulators = devm_kzalloc(&client->dev, supplies * sizeof(struct regulator_bulk_data), GFP_KERNEL);
	if(!sd_pdata->regulators) {
		dev_err(&client->dev, "Failed to allocate regulators.!\n");
		goto no_supply;
	}

	for(index = 0; index < sd_pdata->num_regulators; index ++) {
		of_property_read_string_index(np, "supplies-name", index,
				&(sd_pdata->regulators[index].supply));

		dev_dbg(&client->dev, "sd_pdata->regulators[%d].supply: %s\n",
				index, sd_pdata->regulators[index].supply);
	}

	soc_camera_power_init(&client->dev, ssdd_dt);

no_supply:

	/* Request the power down GPIO asserted */
	priv->pwdn_gpio = devm_gpiod_get_optional(&client->dev, "pwdn",
			GPIOD_OUT_HIGH);
	if (!priv->pwdn_gpio)
		dev_dbg(&client->dev, "pwdn gpio is not assigned!\n");
	else if (IS_ERR(priv->pwdn_gpio))
		return PTR_ERR(priv->pwdn_gpio);

	/* Request the power vcc-en GPIO asserted */
	priv->vcc_en_gpio = devm_gpiod_get_optional(&client->dev, "vcc-en",
			GPIOD_OUT_HIGH);
	if (!priv->vcc_en_gpio)
		dev_dbg(&client->dev, "vcc_en gpio is not assigned!\n");
	else if (IS_ERR(priv->vcc_en_gpio))
		return PTR_ERR(priv->vcc_en_gpio);

	/* Request the power resetb GPIO asserted */
	priv->resetb_gpio = devm_gpiod_get_optional(&client->dev, "resetb",
			GPIOD_OUT_LOW);
	if (!priv->resetb_gpio)
		dev_dbg(&client->dev, "vcc_en gpio is not assigned!\n");
	else if (IS_ERR(priv->resetb_gpio))
		return PTR_ERR(priv->resetb_gpio);

	/* Initialize the soc_camera_subdev_desc */
	priv->ssdd_dt.power = ov2735b_hw_power;
	priv->ssdd_dt.reset = ov2735b_hw_reset;
	client->dev.platform_data = &priv->ssdd_dt;
	return 0;
}

static int ov2735b_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct ov2735b_priv	*priv;
	struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);
	struct i2c_adapter	*adapter = to_i2c_adapter(client->dev.parent);
	int	ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE | I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&adapter->dev,
			"ov2735b: I2C-Adapter doesn't support SMBUS\n");
		return -EIO;
	}

	priv = devm_kzalloc(&client->dev, sizeof(struct ov2735b_priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&adapter->dev,
			"Failed to allocate memory for private data!\n");
		return -ENOMEM;
	}

	priv->clk = v4l2_clk_get(&client->dev, "div_cim");
	if (IS_ERR(priv->clk))
		return -EPROBE_DEFER;

	v4l2_clk_set_rate(priv->clk, 24000000);

	/*mclk gate close*/
	*(volatile unsigned int *)0xb0000024 &= ~(1 << 5);

	if (!ssdd && !client->dev.of_node) {
		dev_err(&client->dev, "Missing platform_data for driver\n");
		ret = -EINVAL;
		goto err_videoprobe;
	}

	if (!ssdd) {
		ret = ov2735b_probe_dt(client, priv);
		if (ret)
			goto err_clk;
	}


	v4l2_i2c_subdev_init(&priv->subdev, client, &ov2735b_subdev_ops);

	/* add handler */
	v4l2_ctrl_handler_init(&priv->hdl, 2);

	v4l2_ctrl_new_std(&priv->hdl, &ov2735b_ctrl_ops,
			V4L2_CID_VFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&priv->hdl, &ov2735b_ctrl_ops,
			V4L2_CID_HFLIP, 0, 1, 1, 0);

	priv->subdev.ctrl_handler = &priv->hdl;
	if (priv->hdl.error) {
		ret = priv->hdl.error;
		goto err_clk;
	}

	ret = ov2735b_video_probe(client);
	if (ret < 0)
		goto err_videoprobe;

	ret = v4l2_async_register_subdev(&priv->subdev);
	if (ret < 0)
		goto err_videoprobe;

	dev_info(&adapter->dev, "ov2735b Probed\n");

	return 0;

err_videoprobe:
	v4l2_ctrl_handler_free(&priv->hdl);
err_clk:
	v4l2_clk_put(priv->clk);
	return ret;
}

static int ov2735b_remove(struct i2c_client *client)
{
	struct ov2735b_priv       *priv = to_ar0144(client);

	v4l2_async_unregister_subdev(&priv->subdev);
	v4l2_clk_put(priv->clk);
	v4l2_device_unregister_subdev(&priv->subdev);
	v4l2_ctrl_handler_free(&priv->hdl);
	return 0;
}

static const struct i2c_device_id ov2735b_id[] = {
	{ "ov2735b",  0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov2735b_id);
static const struct of_device_id ov2735b_of_match[] = {
	{.compatible = "ovit,ov2735b", },
	{},
};
MODULE_DEVICE_TABLE(of, ov2735b_of_match);
static struct i2c_driver ov2735b_i2c_driver = {
	.driver = {
		.name = "ov2735b",
		.of_match_table = of_match_ptr(ov2735b_of_match),
	},
	.probe    = ov2735b_probe,
	.remove   = ov2735b_remove,
	.id_table = ov2735b_id,
};
module_i2c_driver(ov2735b_i2c_driver);

MODULE_DESCRIPTION("SoC Camera driver for onsemi ov2735b sensor");
MODULE_LICENSE("GPL v2");
