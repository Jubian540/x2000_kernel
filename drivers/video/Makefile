obj-$(CONFIG_VGASTATE)            += vgastate.o
obj-$(CONFIG_HDMI)                += hdmi.o

obj-$(CONFIG_VT)		  += console/
obj-$(CONFIG_LOGO)		  += logo/
obj-$(CONFIG_TRUE_COLOR_LOGO)	  += logo-ingenic/
obj-y				  += backlight/

obj-y				  += fbdev/
obj-y				  += ingenic_bscaler/
obj-y				  += ingenic-nna/
obj-y				  += ingenic_i2d/
obj-y				  += ingenic_ipu/
obj-y				  += ingenic_avpu/
obj-$(CONFIG_VIDEOMODE_HELPERS) += display_timing.o videomode.o
ifeq ($(CONFIG_OF),y)
obj-$(CONFIG_VIDEOMODE_HELPERS) += of_display_timing.o of_videomode.o
endif

