#ifndef _DT_INGENIC_PINCTL_H__
#define _DT_INGENIC_PINCTL_H__
#include <dt-bindings/gpio/gpio.h>

#define PINCTL_FUNCTION0	0
#define PINCTL_FUNCTION1	1
#define PINCTL_FUNCTION2	2
#define PINCTL_FUNCTION3	3
#define PINCTL_FUNCHILVL	4
#define PINCTL_FUNCLOLVL	5
#define PINCTL_FUNCINPUT	6
#define PINCTL_FUNCINPUT_LO	7
#define PINCTL_FUNCINPUT_HI	8
#define PINCTL_FUNCINPUT_FE	9
#define PINCTL_FUNCINPUT_RE	10
#define PINCTL_FUNCINPUT_RE_FE	11
#define PINCTL_FUNCTIONS	12


#define PINCTL_CFG_TYPES	1

/* Used in device tree for gpio function settings. ingenic,pincfg*/
#define PINCTL_CFG_BIAS_DISABLE			1
#define PINCTL_CFG_BIAS_HIGH_IMPEDANCE		2
#define PINCTL_CFG_BIAS_PULL_DOWN		3
#define PINCTL_CFG_BIAS_PULL_PIN_DEFAULT	4
#define PINCTL_CFG_BIAS_PULL_UP			5
#define PINCTL_CFG_DRIVE_STRENGTH		9
#define PINCTL_CFG_FILTER			10
#define PINCTL_CFG_INPUT_SCHMITT_ENABLE		11
#define PINCTL_CFG_SLEW_RATE			17

#define PINCTL_CFG_TYPE_MSK	0xFFFF
#define PINCTL_CFGVAL_SFT	16
#define PINCTL_CFGVAL_MSK	(0xFFFF << PINCTL_CFGVAL_SFT)

#define PINCFG_PACK(type, value)	(((value) << PINCTL_CFGVAL_SFT) | type)
#define PINCFG_UNPACK_TYPE(cfg)		((cfg) & PINCTL_CFG_TYPE_MSK)
#define PINCFG_UNPACK_VALUE(cfg)	(((cfg) & PINCTL_CFGVAL_MSK) >> \
						PINCTL_CFGVAL_SFT)

#define PINCTL_NOBIAS		((0 << PINCTL_CFGVAL_SFT) | PINCTL_CFG_PULL)
#define PINCTL_PULLEN		((1 << PINCTL_CFGVAL_SFT) | PINCTL_CFG_PULL)


/* Used in device tree for gpio settings. eg, <&gpb 0, GPIO_ACTIVE_LOW INGENIC_GPIO_PULL_UP | INGENIC_GPIO_DS_0> */
#define INGENIC_GPIO_BIAS_MASK		0x7
#define INGENIC_GPIO_BIAS_SFT		0
#define INGENIC_GPIO_NOBIAS		(0 << INGENIC_GPIO_BIAS_SFT)
#define INGENIC_GPIO_PULLEN		(1 << INGENIC_GPIO_BIAS_SFT)
#define INGENIC_GPIO_PULLUP		(2 << INGENIC_GPIO_BIAS_SFT)
#define INGENIC_GPIO_PULLDOWN		(3 << INGENIC_GPIO_BIAS_SFT)
#define INGENIC_GPIO_HIZ		(4 << INGENIC_GPIO_BIAS_SFT)

#define INGENIC_GPIO_DS_SFT		3
#define INGENIC_GPIO_DS_MSK		0x7
#define INGENIC_GPIO_DS_0		(0 << INGENIC_GPIO_DS_SFT)
#define INGENIC_GPIO_DS_1		(1 << INGENIC_GPIO_DS_SFT)
#define INGENIC_GPIO_DS_2		(2 << INGENIC_GPIO_DS_SFT)
#define INGENIC_GPIO_DS_3		(3 << INGENIC_GPIO_DS_SFT)
#define INGENIC_GPIO_DS_4		(4 << INGENIC_GPIO_DS_SFT)
#define INGENIC_GPIO_DS_5		(5 << INGENIC_GPIO_DS_SFT)
#define INGENIC_GPIO_DS_6		(6 << INGENIC_GPIO_DS_SFT)
#define INGENIC_GPIO_DS_7		(7 << INGENIC_GPIO_DS_SFT)

#define INGENIC_GPIO_SLEW_RATE_SFT	6
#define INGENIC_GPIO_SLEW_RATE		(1 << INGENIC_GPIO_SLEW_RATE_SFT)

#define INGENIC_GPIO_SCHMITT_SFT	7
#define INGENIC_GPIO_SCHMITT		(1 << INGENIC_GPIO_SCHMITT_SFT)

/* Use in pinctrl drivers.*/
#define INGENIC_GPIO_PULL(x)		(((x) >> INGENIC_GPIO_BIAS_SFT) & INGENIC_GPIO_BIAS_MASK)
#define INGENIC_GPIO_DRIVE_STRENGTH(x)  (((x) >> INGENIC_GPIO_DS_SFT) & INGENIC_GPIO_DS_MSK)
#define INGENIC_GPIO_SCHMITT_ENABLE(x)	((x) >> INGENIC_GPIO_SCHMITT_SFT)
#define INGENIC_GPIO_SLEW_RATE_ENABLE(x) ((x) >> INGENIC_GPIO_SLEW_RATE_SFT)


#endif /*_DT_INGENIC_PINCTL_H__*/