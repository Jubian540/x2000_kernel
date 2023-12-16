/*
 * Copyright (c) 2015 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * Input file for Ingenic I2D driver
 *
 * This  program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

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
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/suspend.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/time.h>
#include <linux/dma-buf.h>
#include <soc/base.h>
#include "ingenic_i2d.h"

//#define TIMEOUT_TEST
//#define DEBUG
#ifdef	DEBUG
static int debug_i2d = 1;

#define I2D_DEBUG(format, ...) { if (debug_i2d) printk(format, ## __VA_ARGS__);}
#else
#define I2D_DEBUG(format, ...) do{ } while(0)
#endif

#define I2D_BUF_SIZE (1024 * 1024 * 4)

struct i2d_reg_struct jz_i2d_regs_name[] = {
	{"I2D_VERSION", I2D_RTL_VERSION},
	{"I2D_SHD_CTRL", I2D_SHD_CTRL},
	{"I2D_CTRL", I2D_CTRL},
	{"I2D_IMG_SIZE", I2D_IMG_SIZE},
	{"I2D_IMG_MODE", I2D_IMG_MODE},
	{"I2D_SRC_ADDR_Y", I2D_SRC_ADDR_Y},
	{"I2D_SRC_ADDR_UV", I2D_SRC_ADDR_UV},
	{"I2D_SRC_Y_STRID", I2D_SRC_Y_STRID},
	{"I2D_SRC_UV_STRID", I2D_SRC_UV_STRID},
	{"I2D_DST_ADDR_Y", I2D_DST_ADDR_Y},
	{"I2D_DST_ADDR_UV", I2D_DST_ADDR_UV},
	{"I2D_DST_Y_STRID", I2D_DST_Y_STRID},
	{"I2D_DST_UV_STRID", I2D_DST_UV_STRID},
	{"I2D_IRQ_STATE", I2D_IRQ_STATE},
	{"I2D_IRQ_CLEAR", I2D_IRQ_CLEAR},
	{"I2D_IRQ_MASK", I2D_IRQ_MASK},

	{"I2D_TIMEOUT_VALUE", I2D_TIMEOUT_VALUE},
	{"I2D_TIMEOUT_MODE", I2D_TIMEOUT_MODE},
	{"I2D_CLK_GATE", I2D_CLK_GATE},
	{"I2D_DBG_0", I2D_DBG_0},
};


static void reg_bit_set(struct jz_i2d *i2d, int offset, unsigned int bit)
{
	unsigned int reg = 0;
	reg = reg_read(i2d, offset);
	reg |= bit;
	reg_write(i2d, offset, reg);
}

static void reg_bit_clr(struct jz_i2d *i2d, int offset, unsigned int bit)
{
	unsigned int reg = 0;
	reg= reg_read(i2d, offset);
	reg &= ~(bit);
	reg_write(i2d, offset, reg);
}


static int _i2d_dump_regs(struct jz_i2d *i2d)
{
	int i = 0;
	int num = 0;

	if (i2d == NULL) {
		dev_err(i2d->dev, "i2d is NULL!\n");
		return -1;
	}
	printk("----- dump regs -----\n");

	num = sizeof(jz_i2d_regs_name) / sizeof(struct i2d_reg_struct);
	for (i = 0; i < num; i++) {
		printk("i2d_reg: %s: \t0x%08x\r\n", jz_i2d_regs_name[i].name, reg_read(i2d, jz_i2d_regs_name[i].addr));
	}

	return 0;
}

static void _i2d_dump_param(struct jz_i2d *i2d)
{
	return;
}

static int i2d_dump_info(struct jz_i2d *i2d)
{
	int ret = 0;
	if (i2d == NULL) {
		dev_err(i2d->dev, "i2d is NULL\n");
		return -1;
	}
	printk("i2d: i2d->base: %p\n", i2d->iomem);
	_i2d_dump_param(i2d);
	ret = _i2d_dump_regs(i2d);

	return ret;
}

static int i2d_reg_set(struct jz_i2d *i2d, struct i2d_param *i2d_param)
{
    unsigned int fmt = 0;
    unsigned int srcw = 0;
    unsigned int srch = 0;
    unsigned int flip_enable = 0, mirr_enable = 0, rotate_angle = 0, rotate_enable = 0;
    unsigned int flip_mode = 0;
    unsigned int value = 0;

	unsigned int src_y_pbuf = 0;
	unsigned int src_uv_pbuf = 0;
    unsigned int dst_y_pbuf = 0;
    unsigned int dst_uv_pbuf = 0;

    unsigned int src_y_strid = 0;
	unsigned int src_uv_strid = 0;
    unsigned int dst_y_strid = 0;
    unsigned int dst_uv_strid = 0;

    struct i2d_param *ip = i2d_param;
    if (i2d == NULL) {
		dev_err(i2d->dev, "i2d: i2d is NULL or i2d_param is NULL\n");
		return -1;
	}

    fmt = ip->data_type;
    srcw = ip->src_w;
    srch = ip->src_h;
    flip_enable = ip->flip_enable;
    mirr_enable = ip->mirr_enable;
    rotate_enable = ip->rotate_enable;
    rotate_angle = ip->rotate_angle;

    switch(fmt) {
        case PIX_FMT_NV12:
            src_y_strid  = srcw;
            src_uv_strid = srcw;
            if ((rotate_angle == 0) || (rotate_angle == 180) || (flip_enable == 1) || (mirr_enable == 1)) {
                dst_y_strid = srcw;
                dst_uv_strid = srcw;
            } else if ((rotate_angle == 90) || (rotate_angle == 270)) {
                dst_y_strid = srch;
                dst_uv_strid = srch;
            }
            reg_write(i2d, I2D_IMG_MODE, I2D_DATA_TYPE_NV12);
            break;
        case HAL_PIXEL_FORMAT_RAW8:
            src_y_strid = srcw;
            src_uv_strid = srcw;
            if ((rotate_angle == 0) || (rotate_angle == 180) || (flip_enable == 1) || (mirr_enable == 1)) {
                dst_y_strid = srcw;
                dst_uv_strid = srcw;
            } else if ((rotate_angle == 90) || (rotate_angle == 270)) {
                dst_y_strid = srch;
                dst_uv_strid = srch;
            }
            reg_write(i2d, I2D_IMG_MODE, I2D_DATA_TYPE_RAW8);
            break;
        case HAL_PIXEL_FORMAT_RGB_565:
            src_y_strid = srcw << 1;
            src_uv_strid = srcw;
            if ((rotate_angle == 0) || (rotate_angle == 180) || (flip_enable == 1) || (mirr_enable == 1)) {
                dst_y_strid = srcw << 1;
                dst_uv_strid = srcw;
            } else if ((rotate_angle == 90) || (rotate_angle == 270)) {
                dst_y_strid = srch << 1;
                dst_uv_strid = srch;
            }
            reg_write(i2d, I2D_IMG_MODE, I2D_DATA_TYPE_RGB565);
            break;
        case HAL_PIXEL_FORMAT_ARGB_8888:
            src_y_strid = srcw << 2;
            src_uv_strid = srcw;
            if ((flip_mode == FLIP_MODE_0_DEGREE) || (flip_mode == FLIP_MODE_180_DEGREE) || (flip_mode == FLIP_MODE_FLIP) || (flip_mode == FLIP_MODE_MIRR)) {
                dst_y_strid = srcw << 2;
                dst_uv_strid = srcw;
            } else if ((flip_mode == FLIP_MODE_90_DEGREE) || (flip_mode == FLIP_MODE_270_DEGREE)) {
                dst_y_strid = srch << 2;
                dst_uv_strid = srch;
            }
            reg_write(i2d, I2D_IMG_MODE, I2D_DATA_TYPE_ARGB8888);
            break;
        default:
            src_y_strid  = srcw;
            src_uv_strid = srcw;
            if ((flip_mode == FLIP_MODE_0_DEGREE) || (flip_mode == FLIP_MODE_180_DEGREE) || (flip_mode == FLIP_MODE_FLIP) || (flip_mode == FLIP_MODE_MIRR)) {
                dst_y_strid = srcw;
                dst_uv_strid = srcw;
            } else if ((flip_mode == FLIP_MODE_90_DEGREE) || (flip_mode == FLIP_MODE_270_DEGREE)) {
                dst_y_strid = srch;
                dst_uv_strid = srch;
            }
            reg_write(i2d, I2D_IMG_MODE, I2D_DATA_TYPE_NV12);
            break;
    }


#ifdef TIMEOUT_TEST
    reg_write(i2d, I2D_TIMEOUT_MODE, (0 << 0)); //timeout mode 0:not restart  1:auto restart
    reg_write(i2d, I2D_TIMEOUT_VALUE, (0x64 << 0)); //timeout value
#endif
    reg_write(i2d, I2D_IMG_SIZE, (srcw << I2D_WIDTH | srch << I2D_HEIGHT));
   // reg_write(i2d, I2D_IMG_MODE, I2D_DATA_TYPE_NV12);
    reg_write(i2d, I2D_IMG_SIZE, (srcw << I2D_WIDTH | srch << I2D_HEIGHT));
    //reg_write(i2d, I2D_IMG_MODE, I2D_FLIP_MODE_90);

    if(rotate_enable == 1) {
        unsigned int value = 0, tmp1 = 0;

        if(rotate_angle == 0) {
            flip_mode = 0x00;
        } else if(rotate_angle == 90) {
            flip_mode = 0x04;
        } else if(rotate_angle == 180) {
            flip_mode = 0x08;
        } else if(rotate_angle == 270) {
            flip_mode = 0x0c;
        } else {
            printk("error: the rotate_angle not suppot!!!\n");
            return -1;
        }

        value = flip_mode;
        flip_mode = flip_mode & 0x00000004;
        switch(flip_mode) {
            case 0:
                dst_uv_pbuf = ip->dst_addr_y + dst_y_strid*srch;
                tmp1 = reg_read(i2d, I2D_IMG_MODE);
                value = (((~tmp1) & value) | value);
                reg_write(i2d, I2D_IMG_MODE, value);
                break;
            case 4:
                dst_uv_pbuf = ip->dst_addr_y + dst_y_strid*srcw;
                tmp1 = reg_read(i2d, I2D_IMG_MODE);
                value = (((~tmp1) & value) | value);
                reg_write(i2d, I2D_IMG_MODE, value);
                break;
            default:
                dst_uv_pbuf = ip->dst_addr_y + dst_y_strid*srch;
                tmp1 = reg_read(i2d, I2D_IMG_MODE);
                value = (((~tmp1) & value) | value);
                reg_write(i2d, I2D_IMG_MODE, value);
                break;
        }
    }


     if(mirr_enable == 1) {
        dst_uv_pbuf = ip->dst_addr_y + dst_y_strid*srch;
        value = reg_read(i2d, I2D_IMG_MODE);
        reg_write(i2d, I2D_IMG_MODE, (value | I2D_MIRR));
    }

    if(flip_enable == 1) {
        dst_uv_pbuf = ip->dst_addr_y + dst_y_strid*srch;
        value = reg_read(i2d, I2D_IMG_MODE);
        reg_write(i2d, I2D_IMG_MODE, (value | I2D_FLIP));
    }



    src_y_pbuf = ip->src_addr_y;
    src_uv_pbuf = ip->src_addr_y + src_y_strid*srch;
    dst_y_pbuf = ip->dst_addr_y;

    reg_write(i2d, I2D_SRC_ADDR_Y, src_y_pbuf);
    reg_write(i2d, I2D_SRC_ADDR_UV, src_uv_pbuf);
    reg_write(i2d, I2D_DST_ADDR_Y, dst_y_pbuf);
    reg_write(i2d, I2D_DST_ADDR_UV, dst_uv_pbuf);


    reg_write(i2d, I2D_SRC_Y_STRID, src_y_strid);
    reg_write(i2d, I2D_SRC_UV_STRID, src_uv_strid);
    reg_write(i2d, I2D_DST_Y_STRID, dst_y_strid);
    reg_write(i2d, I2D_DST_UV_STRID, dst_uv_strid);


    return 0;
}

static int i2d_start(struct jz_i2d *i2d, struct i2d_param *i2d_param)
{
	int ret = 0;
	struct i2d_param *ip = i2d_param;
	if ((i2d == NULL) || (i2d_param == NULL)) {
		dev_err(i2d->dev, "i2d: i2d is NULL or i2d_param is NULL\n");
		return -1;
	}
	I2D_DEBUG("i2d: enter i2d_start %d\n", current->pid);
#ifndef CONFIG_FPGA_TEST
	clk_prepare_enable(i2d->clk);
#endif
	__reset_safe_i2d();

    ret = i2d_reg_set(i2d, ip);
    __i2d_mask_irq();//mask 0


	reg_write(i2d, I2D_SHD_CTRL, 0xffffffff);


	/* start i2d */
	__start_i2d();

#ifdef DEBUG
	i2d_dump_info(i2d);
#endif
	I2D_DEBUG("i2d_start\n");
	mutex_lock(&i2d->irq_mutex);
	ret = wait_for_completion_interruptible_timeout(&i2d->done_i2d, msecs_to_jiffies(2000));
	if (ret < 0) {
		printk("i2d: done_i2d wait_for_completion_interruptible_timeout err %d\n", ret);
	    mutex_unlock(&i2d->irq_mutex);
		goto err_i2d_wait_for_done;
	} else if (ret == 0) {
		ret = -1;
		printk("i2d: done_i2d wait_for_completion_interruptible_timeout timeout %d\n", ret);
		i2d_dump_info(i2d);
	    mutex_unlock(&i2d->irq_mutex);
		goto err_i2d_wait_for_done;
	} else {
		;
	}
	mutex_unlock(&i2d->irq_mutex);
    I2D_DEBUG("i2d: exit i2d_start %d\n", current->pid);

    return 0;

err_i2d_wait_for_done:

	return ret;

}

static int i2d_listen_buffer(struct jz_i2d *i2d, unsigned long arg)
{
    int ret = 0;
    int buffers = 0;

	ret = wait_for_completion_interruptible_timeout(&i2d->done_i2d, msecs_to_jiffies(10*1000));
	if (ret < 0) {
		printk("i2d: done_i2d wait_for_completion_interruptible_timeout err %d\n", ret);
		return -1;
	} else if (ret == 0) {
		ret = -1;
		printk("i2d: done_i2d wait_for_completion_interruptible_timeout timeout %d\n", ret);
		i2d_dump_info(i2d);
		buffers = ret;
		return -1;
	} else {
		buffers = 1;
	}

    ret = copy_to_user((void __user *)arg, &buffers,sizeof(buffers));
    if (ret){
        printk("copy_to_user error!!\n");
        return -1;
    }
    return ret;
}

void i2dgettimee(int64_t *ptime)
{
    struct timeval sttime;
    do_gettimeofday(&sttime);
    *ptime = sttime.tv_sec  * 1000000 + (sttime.tv_usec);
    return ;
}


#ifdef CONFIG_VIDEO_V4L2
static int i2d_dmabuf_get_phy(struct device *dev,int fd,unsigned int *phyaddr)
{
	struct dma_buf_attachment *attach;
	struct dma_buf *dbuf;
	struct sg_table *sgt;
	int err = 0;
	dbuf = dma_buf_get(fd);
	if (IS_ERR(dbuf))
		return -EINVAL;
	attach = dma_buf_attach(dbuf, dev);
	if (IS_ERR(attach)) {
		err = -EINVAL;
		goto fail_attach;
	}
	sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		err = -EINVAL;
		goto fail_map;
	}

	*phyaddr = sg_dma_address(sgt->sgl);

	dma_buf_unmap_attachment(attach, sgt, DMA_BIDIRECTIONAL);
fail_map:
	dma_buf_detach(dbuf, attach);
fail_attach:
	dma_buf_put(dbuf);
	return err;
}
#endif

static long i2d_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct i2d_param iparam;
	struct miscdevice *dev = filp->private_data;
	struct jz_i2d *i2d = container_of(dev, struct jz_i2d, misc_dev);
    int64_t time0 = 0,time1 = 0,time2 = 0;
	I2D_DEBUG("i2d: %s pid: %d, tgid: %d file: %p, cmd: 0x%08x\n",
			__func__, current->pid, current->tgid, filp, cmd);

	if (_IOC_TYPE(cmd) != JZI2D_IOC_MAGIC) {
		dev_err(i2d->dev, "invalid cmd!\n");
		return -EFAULT;
	}

	mutex_lock(&i2d->mutex);
	switch (cmd) {
		case IOCTL_I2D_START:
            i2dgettimee(&time0);
			if (copy_from_user(&iparam, (void *)arg, sizeof(struct i2d_param))) {
				dev_err(i2d->dev, "copy_from_user error!!!\n");
				ret = -EFAULT;
				break;
			}
            i2dgettimee(&time1);
			ret = i2d_start(i2d, &iparam);
			if (ret < 0) {
				printk("i2d: error i2d start ret = %d\n", ret);
			}
            i2dgettimee(&time2);
#ifdef I2DTIME
            printk("*****time1-time0=(copyfrom:%lld),time2-time1=(i2dstart:%lld),total:%lld\n",time1-time0,time2-time1,time2-time0);
#endif
			break;
		case IOCTL_I2D_LISTEN:
			//return i2d_listen_buffer(i2d, arg);
            break;
		case IOCTL_I2D_GET_PBUFF:
			if (i2d->pbuf.vaddr_alloc == 0) {
				unsigned int size = I2D_BUF_SIZE;
				i2d->pbuf.vaddr_alloc = (unsigned int)kmalloc(size, GFP_KERNEL);
				if (!i2d->pbuf.vaddr_alloc) {
					printk("i2d kmalloc is error\n");
					ret = -ENOMEM;
				}
				memset((void*)(i2d->pbuf.vaddr_alloc), 0x00, size);
				i2d->pbuf.size = size;
				i2d->pbuf.paddr = virt_to_phys((void *)(i2d->pbuf.vaddr_alloc));
				i2d->pbuf.paddr_align = ((unsigned long)(i2d->pbuf.paddr));
			}
			I2D_DEBUG("i2d: %s i2d->pbuf.vaddr_alloc = 0x%08x\ni2d->pbuf.vaddr_align = 0x%08x\ni2d->pbuf.size = 0x%x\ni2d->pbuf.paddr = 0x%08x\n"
					, __func__, i2d->pbuf.vaddr_alloc, i2d->pbuf.paddr_align, i2d->pbuf.size, i2d->pbuf.paddr);
			if (copy_to_user((void *)arg, &i2d->pbuf, sizeof(struct i2d_buf_info))) {
				dev_err(i2d->dev, "copy_to_user error!!!\n");
				ret = -EFAULT;
			}
			break;
		case IOCTL_I2D_RES_PBUFF:
			if (i2d->pbuf.vaddr_alloc != 0) {
				kfree((void *)i2d->pbuf.vaddr_alloc);
				i2d->pbuf.vaddr_alloc = 0;
				i2d->pbuf.size = 0;
				i2d->pbuf.paddr = 0;
				i2d->pbuf.paddr_align = 0;
			} else {
				dev_warn(i2d->dev, "buffer wanted to free is null\n");
			}
			break;
		case IOCTL_I2D_BUF_FLUSH_CACHE:
			{
				struct i2d_flush_cache_para fc;
				if (copy_from_user(&fc, (void *)arg, sizeof(fc))) {
					dev_err(i2d->dev, "copy_from_user error!!!\n");
					ret = -EFAULT;
					break;
				}
				//dma_sync_single_for_device(NULL, fc.addr, fc.size, DMA_TO_DEVICE);
				//dma_sync_single_for_device(NULL, fc.addr, fc.size, DMA_FROM_DEVICE);
				dma_cache_sync(NULL, fc.addr, fc.size, DMA_BIDIRECTIONAL);
			}
			break;
#ifdef CONFIG_VIDEO_V4L2
		case IOCTL_I2D_GET_DMA_PHY:
			{
				unsigned int phyaddr = 0;
				int fd = 0;
				if (copy_from_user(&fd, (void *)arg, sizeof(fd))) {
					dev_err(i2d->dev, "copy_from_user error!!!\n");
					ret = -EFAULT;
					break;
				}
				ret = i2d_dmabuf_get_phy(i2d->dev,fd,&phyaddr);
				if (copy_to_user((void *)arg, &phyaddr, sizeof(phyaddr)))
					return -EFAULT;
			}
			break;
#endif
	    default:
			dev_err(i2d->dev, "invalid command: 0x%08x\n", cmd);
			ret = -EINVAL;
	}

	mutex_unlock(&i2d->mutex);
	return ret;
}

static int i2d_open(struct inode *inode, struct file *filp)
{
	int ret = 0;

	struct miscdevice *dev = filp->private_data;
	struct jz_i2d *i2d = container_of(dev, struct jz_i2d, misc_dev);

	I2D_DEBUG("i2d: %s pid: %d, tgid: %d filp: %p\n",
			__func__, current->pid, current->tgid, filp);
	mutex_lock(&i2d->mutex);

	mutex_unlock(&i2d->mutex);
	return ret;
}

static int i2d_release(struct inode *inode, struct file *filp)
{
	int ret = 0;

	struct miscdevice *dev = filp->private_data;
	struct jz_i2d *i2d = container_of(dev, struct jz_i2d, misc_dev);

	I2D_DEBUG("i2d: %s  pid: %d, tgid: %d filp: %p\n",
			__func__, current->pid, current->tgid, filp);
	mutex_lock(&i2d->mutex);

	mutex_unlock(&i2d->mutex);
	return ret;
}

static struct file_operations i2d_ops = {
	.owner = THIS_MODULE,
	.open = i2d_open,
	.release = i2d_release,
	.unlocked_ioctl = i2d_ioctl,
};

int ttt = 0;

static irqreturn_t i2d_irq_handler(int irq, void *data)
{
	struct jz_i2d *i2d;
	unsigned int status;

	I2D_DEBUG("i2d: %s\n", __func__);
	i2d = (struct jz_i2d *)data;

    status = reg_read(i2d, I2D_IRQ_STATE);

    __i2d_irq_clear(status);

    I2D_DEBUG("----- %s, status= 0x%08x\n", __func__, status);
	 /* this status doesn't do anything including trigger interrupt,
	 * just give a hint */
    if (status & 0x1){
        complete(&i2d->done_i2d);
    }

    /*timeout 10 times,let timeout value 0xffffffff,this can normal run handler*/
   // if (status & 0x2){
        //if(ttt > 10){
        //    reg_write(i2d, I2D_TIMEOUT_VALUE, 0xfffffff); //timeout value
        //}else{
        //    ttt++;
        //}
    //}
    return IRQ_HANDLED;
}

static int i2d_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct jz_i2d *i2d;
    int rc = 0;

	I2D_DEBUG("%s\n", __func__);
	i2d = (struct jz_i2d *)devm_kzalloc(&pdev->dev, sizeof(struct jz_i2d), GFP_KERNEL);
	if (!i2d) {
		dev_err(&pdev->dev, "alloc jz_i2d failed!\n");
		return -ENOMEM;
	}

	sprintf(i2d->name, "i2d");

	i2d->misc_dev.minor = MISC_DYNAMIC_MINOR;
	i2d->misc_dev.name = i2d->name;
	i2d->misc_dev.fops = &i2d_ops;
	i2d->dev = &pdev->dev;

	mutex_init(&i2d->mutex);
	mutex_init(&i2d->irq_mutex);
	init_completion(&i2d->done_i2d);
	init_completion(&i2d->done_buf);
	complete(&i2d->done_buf);

	i2d->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!i2d->res) {
		dev_err(&pdev->dev, "failed to get dev resources: %d\n", ret);
		ret = -EINVAL;
	}

    pdev->id = of_alias_get_id(pdev->dev.of_node, "i2d");

    i2d->iomem = devm_ioremap_resource(&pdev->dev, i2d->res);
    if (IS_ERR(i2d->iomem)) {
        dev_err(&pdev->dev, "failed to map io baseaddress. \n");
        ret = -ENODEV;
        goto err_ioremap;
    }

	i2d->irq = platform_get_irq(pdev, 0);
	if (devm_request_irq(&pdev->dev, i2d->irq, i2d_irq_handler, IRQF_SHARED, i2d->name, i2d)) {
		dev_err(&pdev->dev, "request irq failed\n");
		ret = -EINVAL;
		goto err_req_irq;
	}

#ifndef CONFIG_FPGA_TEST
	i2d->clk = devm_clk_get(i2d->dev, "gate_i2d");
	if (IS_ERR(i2d->clk)) {
		dev_err(&pdev->dev, "i2d clk get failed!\n");
		goto err_get_i2d_clk;
	}

#endif

    if ((rc = clk_prepare_enable(i2d->clk)) < 0) {
		dev_err(&pdev->dev, "Enable i2d gate clk failed\n");
		goto err_prepare_i2d_clk;
	}

    dev_set_drvdata(&pdev->dev, i2d);

	__reset_safe_i2d();

    ret = misc_register(&i2d->misc_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "register misc device failed!\n");
		goto err_set_drvdata;
	}

	return 0;

err_set_drvdata:
	devm_free_irq(&pdev->dev, i2d->irq, i2d);
err_get_i2d_clk:
	devm_clk_put(&pdev->dev, i2d->clk);
err_prepare_i2d_clk:
	clk_disable_unprepare(i2d->clk);
err_req_irq:
    devm_free_irq(&pdev->dev, i2d->irq, i2d);
err_ioremap:
	devm_iounmap(&pdev->dev, i2d->iomem);
	kfree(i2d);

	return ret;
}

static int i2d_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct jz_i2d *i2d;
	I2D_DEBUG("%s\n", __func__);

	i2d = dev_get_drvdata(&pdev->dev);

    free_irq(i2d->irq, i2d);

    devm_iounmap(&pdev->dev, i2d->iomem);

	misc_deregister(&i2d->misc_dev);
	if (ret < 0) {
		dev_err(i2d->dev, "misc_deregister error %d\n", ret);
		return ret;
	}

    if (i2d->pbuf.vaddr_alloc) {
		kfree((void *)(i2d->pbuf.vaddr_alloc));
		i2d->pbuf.vaddr_alloc = 0;
	}

    clk_disable_unprepare(i2d->clk);

	if (i2d) {
		kfree(i2d);
	}

	return 0;
}

static const struct of_device_id ingenic_i2d_dt_match[] = {
	{ .compatible = "ingenic,x2500-i2d", .data = NULL },
	{},
};
MODULE_DEVICE_TABLE(of, ingenic_i2d_dt_match);

static struct platform_driver jz_i2d_driver = {
	.probe	= i2d_probe,
	.remove = i2d_remove,
	.driver = {
		.name = "jz-i2d",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(ingenic_i2d_dt_match),
	},
};

module_platform_driver(jz_i2d_driver)

MODULE_DESCRIPTION("JZ I2D driver");
MODULE_AUTHOR("jiansheng.zhang <jiansheng.zhang@ingenic.cn>");
MODULE_LICENSE("GPL");
