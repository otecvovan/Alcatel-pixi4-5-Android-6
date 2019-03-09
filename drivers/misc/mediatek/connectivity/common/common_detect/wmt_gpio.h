
#ifndef _WMT_GPIO_H_
#define _WMT_GPIO_H_

#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include "osal.h"


#define DEFAULT_PIN_ID (0xffffffff)

typedef enum _ENUM_GPIO_PIN_ID {
	GPIO_COMBO_LDO_EN_PIN = 0,
	GPIO_COMBO_PMUV28_EN_PIN,
	GPIO_COMBO_PMU_EN_PIN,
	GPIO_COMBO_RST_PIN,
	GPIO_COMBO_BGF_EINT_PIN,
	GPIO_WIFI_EINT_PIN,
	GPIO_COMBO_ALL_EINT_PIN,
	GPIO_COMBO_URXD_PIN,
	GPIO_COMBO_UTXD_PIN,
	GPIO_PCM_DAICLK_PIN,
	GPIO_PCM_DAIPCMIN_PIN,
	GPIO_PCM_DAIPCMOUT_PIN,
	GPIO_PCM_DAISYNC_PIN,
	GPIO_COMBO_I2S_CK_PIN,
	GPIO_COMBO_I2S_WS_PIN,
	GPIO_COMBO_I2S_DAT_PIN,
	GPIO_GPS_SYNC_PIN,
	GPIO_GPS_LNA_PIN,
	GPIO_PIN_ID_MAX
} ENUM_GPIO_PIN_ID, *P_ENUM_GPIO_PIN_ID;

typedef enum _ENUM_GPIO_STATE_ID {
	GPIO_PULL_DIS = 0,
	GPIO_PULL_DOWN,
	GPIO_PULL_UP,
	GPIO_OUT_LOW,
	GPIO_OUT_HIGH,
	GPIO_IN_DIS,
	GPIO_IN_EN,
	GPIO_IN_PULL_DIS,
	GPIO_IN_PULLDOWN,
	GPIO_IN_PULLUP,
	GPIO_STATE_MAX,
} ENUM_GPIO_STATE_ID, *P_ENUM_GPIO_STATE_ID;

typedef struct _GPIO_CTRL_STATE {
	INT32 gpio_num;
	struct pinctrl_state *gpio_state[GPIO_STATE_MAX];
} GPIO_CTRL_STATE, *P_GPIO_CTRL_STATE;

typedef struct _GPIO_CTRL_INFO {
	struct pinctrl *pinctrl_info;
	GPIO_CTRL_STATE gpio_ctrl_state[GPIO_PIN_ID_MAX];
} GPIO_CTRL_INFO, *P_GPIO_CTRL_INFO;

extern const PUINT8 gpio_state_name[GPIO_PIN_ID_MAX][GPIO_STATE_MAX];
extern const PUINT8 gpio_pin_name[GPIO_PIN_ID_MAX];
extern GPIO_CTRL_INFO gpio_ctrl_info;

INT32 wmt_gpio_init(struct platform_device *pdev);

INT32 wmt_gpio_deinit(VOID);

#endif
