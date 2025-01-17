

#ifndef _WMT_EXP_H_
#define _WMT_EXP_H_

#include <mtk_wcn_cmb_stub.h>
#include "osal.h"
#include "wmt_plat.h"
#include "wmt_stp_exp.h"
/* not to reference to internal wmt */
/* #include "wmt_core.h" */

#if 1				/* moved from wmt_lib.h */
#ifndef DFT_TAG
#define DFT_TAG         "[WMT-DFT]"
#endif

#define WMT_LOUD_FUNC(fmt, arg...) \
do { \
	if (gWmtDbgLvl >= WMT_LOG_LOUD) \
		osal_dbg_print(DFT_TAG "[L]%s:"  fmt, __func__ , ##arg); \
} while (0)
#define WMT_INFO_FUNC(fmt, arg...)  \
do { \
	if (gWmtDbgLvl >= WMT_LOG_INFO) \
		osal_dbg_print(DFT_TAG "[I]%s:"  fmt, __func__ , ##arg); \
} while (0)
#define WMT_WARN_FUNC(fmt, arg...) \
do { \
	if (gWmtDbgLvl >= WMT_LOG_WARN) \
		osal_warn_print(DFT_TAG "[W]%s:"  fmt, __func__ , ##arg); \
} while (0)
#define WMT_ERR_FUNC(fmt, arg...) \
do { \
	if (gWmtDbgLvl >= WMT_LOG_ERR) \
		osal_err_print(DFT_TAG "[E]%s(%d):"  fmt, __func__ , __LINE__, ##arg); \
} while (0)
#define WMT_DBG_FUNC(fmt, arg...) \
do { \
	if (gWmtDbgLvl >= WMT_LOG_DBG) \
		osal_dbg_print(DFT_TAG "[D]%s:"  fmt, __func__ , ##arg); \
} while (0)
#define WMT_TRC_FUNC(f) \
do { \
	if (gWmtDbgLvl >= WMT_LOG_DBG) \
		osal_dbg_print(DFT_TAG "<%s> <%d>\n", __func__, __LINE__); \
} while (0)

#endif

#if 1				/* moved from wmt_lib.h */
extern UINT32 gWmtDbgLvl;
#endif
extern OSAL_BIT_OP_VAR gBtWifiGpsState;
extern OSAL_BIT_OP_VAR gGpsFmState;
extern UINT32 gWifiProbed;
extern MTK_WCN_BOOL g_pwr_off_flag;
extern UINT32 g_IsNeedDoChipReset;
#if 1				/* moved from wmt_lib.h */
#define WMT_LOG_LOUD    4
#define WMT_LOG_DBG     3
#define WMT_LOG_INFO    2
#define WMT_LOG_WARN    1
#define WMT_LOG_ERR     0
#endif
#define CFG_CORE_INTERNAL_TXRX 0	/*just do TX/RX in host side */

#ifndef MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT
typedef enum _ENUM_WMTDRV_TYPE_T {
	WMTDRV_TYPE_BT = 0,
	WMTDRV_TYPE_FM = 1,
	WMTDRV_TYPE_GPS = 2,
	WMTDRV_TYPE_WIFI = 3,
	WMTDRV_TYPE_WMT = 4,
	WMTDRV_TYPE_STP = 5,
	WMTDRV_TYPE_LPBK = 6,
	WMTDRV_TYPE_COREDUMP = 7,
	WMTDRV_TYPE_MAX
} ENUM_WMTDRV_TYPE_T, *P_ENUM_WMTDRV_TYPE_T;

/* TODO: [ChangeFeature][GeorgeKuo] Reconsider usage of this type */
/* TODO: how do we extend for new chip and newer revision? */
/* TODO: This way is hard to extend */
typedef enum _ENUM_WMTHWVER_TYPE_T {
	WMTHWVER_E1 = 0x0,
	WMTHWVER_E2 = 0x1,
	WMTHWVER_E3 = 0x2,
	WMTHWVER_E4 = 0x3,
	WMTHWVER_E5 = 0x4,
	WMTHWVER_E6 = 0x5,
	WMTHWVER_MAX,
	WMTHWVER_INVALID = 0xff
} ENUM_WMTHWVER_TYPE_T, *P_ENUM_WMTHWVER_TYPE_T;

typedef enum _ENUM_WMTDSNS_TYPE_T {
	WMTDSNS_FM_DISABLE = 0,
	WMTDSNS_FM_ENABLE = 1,
	WMTDSNS_FM_GPS_DISABLE = 2,
	WMTDSNS_FM_GPS_ENABLE = 3,
	WMTDSNS_MAX
} ENUM_WMTDSNS_TYPE_T, *P_ENUM_WMTDSNS_TYPE_T;

typedef enum _ENUM_WMTTHERM_TYPE_T {
	WMTTHERM_ZERO = 0,
	WMTTHERM_ENABLE = WMTTHERM_ZERO + 1,
	WMTTHERM_READ = WMTTHERM_ENABLE + 1,
	WMTTHERM_DISABLE = WMTTHERM_READ + 1,
	WMTTHERM_MAX
} ENUM_WMTTHERM_TYPE_T, *P_ENUM_WMTTHERM_TYPE_T;

typedef enum _ENUM_WMTMSG_TYPE_T {
	WMTMSG_TYPE_POWER_ON = 0,
	WMTMSG_TYPE_POWER_OFF = 1,
	WMTMSG_TYPE_RESET = 2,
	WMTMSG_TYPE_STP_RDY = 3,
	WMTMSG_TYPE_HW_FUNC_ON = 4,
	WMTMSG_TYPE_MAX
} ENUM_WMTMSG_TYPE_T, *P_ENUM_WMTMSG_TYPE_T;

typedef void (*PF_WMT_CB) (ENUM_WMTDRV_TYPE_T,	/* Source driver type */
			   ENUM_WMTDRV_TYPE_T,	/* Destination driver type */
			   ENUM_WMTMSG_TYPE_T,	/* Message type */
			   VOID *,	/* READ-ONLY buffer. Buffer is allocated and freed by WMT_drv. Client
					   can't touch this buffer after this function return. */
			   UINT32	/* Buffer size in unit of byte */
);

typedef enum _SDIO_PS_OP {
	OWN_SET = 0,
	OWN_CLR = 1,
	OWN_STATE = 2,
} SDIO_PS_OP;

typedef INT32(*PF_WMT_SDIO_PSOP) (SDIO_PS_OP);

typedef enum _ENUM_WMTCHIN_TYPE_T {
	WMTCHIN_CHIPID = 0x0,
	WMTCHIN_HWVER = WMTCHIN_CHIPID + 1,
	WMTCHIN_MAPPINGHWVER = WMTCHIN_HWVER + 1,
	WMTCHIN_FWVER = WMTCHIN_MAPPINGHWVER + 1,
	WMTCHIN_MAX,

} ENUM_WMT_CHIPINFO_TYPE_T, *P_ENUM_WMT_CHIPINFO_TYPE_T;

#endif

typedef enum _ENUM_WMTRSTMSG_TYPE_T {
	WMTRSTMSG_RESET_START = 0x0,
	WMTRSTMSG_RESET_END = 0x1,
	WMTRSTMSG_RESET_END_FAIL = 0x2,
	WMTRSTMSG_RESET_MAX,
	WMTRSTMSG_RESET_INVALID = 0xff
} ENUM_WMTRSTMSG_TYPE_T, *P_ENUM_WMTRSTMSG_TYPE_T;

typedef enum _ENUM_BT_GPS_ONOFF_STATE_T {
	WMT_BT_ON = 0,
	WMT_GPS_ON = 1,
	WMT_WIFI_ON = 2,
	WMT_FM_ON = 3,
	WMT_BT_GPS_STATE_MAX,
	WMT_BT_GPS_STATE_INVALID = 0xff
} ENUM_BT_GPS_ONOFF_STATE_T, *P_ENUM_BT_GPS_ONOFF_STATE_T;

#if 1				/* moved from wmt_core.h */
typedef enum {
	WMT_SDIO_SLOT_INVALID = 0,
	WMT_SDIO_SLOT_SDIO1 = 1,	/* Wi-Fi dedicated SDIO1 */
	WMT_SDIO_SLOT_SDIO2 = 2,
	WMT_SDIO_SLOT_MAX
} WMT_SDIO_SLOT_NUM;

typedef enum {
	WMT_SDIO_FUNC_STP = 0,
	WMT_SDIO_FUNC_WIFI = 1,
	WMT_SDIO_FUNC_MAX
} WMT_SDIO_FUNC_TYPE;
#endif

typedef INT32(*wmt_wlan_probe_cb) (VOID);
typedef INT32(*wmt_wlan_remove_cb) (VOID);
typedef INT32(*wmt_wlan_bus_cnt_get_cb) (VOID);
typedef INT32(*wmt_wlan_bus_cnt_clr_cb) (VOID);

typedef struct _MTK_WCN_WMT_WLAN_CB_INFO {
	wmt_wlan_probe_cb wlan_probe_cb;
	wmt_wlan_remove_cb wlan_remove_cb;
	wmt_wlan_bus_cnt_get_cb wlan_bus_cnt_get_cb;
	wmt_wlan_bus_cnt_clr_cb wlan_bus_cnt_clr_cb;
} MTK_WCN_WMT_WLAN_CB_INFO, *P_MTK_WCN_WMT_WLAN_CB_INFO;

extern INT32 mtk_wcn_wmt_wlan_reg(P_MTK_WCN_WMT_WLAN_CB_INFO pWmtWlanCbInfo);
extern INT32 mtk_wcn_wmt_wlan_unreg(VOID);
extern wmt_wlan_probe_cb mtk_wcn_wlan_probe;
extern wmt_wlan_remove_cb mtk_wcn_wlan_remove;
extern wmt_wlan_bus_cnt_get_cb mtk_wcn_wlan_bus_tx_cnt;
extern wmt_wlan_bus_cnt_clr_cb mtk_wcn_wlan_bus_tx_cnt_clr;

/*subsystem function ctrl APIs*/
extern MTK_WCN_BOOL mtk_wcn_wmt_assert(ENUM_WMTDRV_TYPE_T type, UINT32 reason);

#ifndef MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT
#define WMT_EXP_HID_API_EXPORT 0

extern MTK_WCN_BOOL mtk_wcn_wmt_func_off(ENUM_WMTDRV_TYPE_T type);

extern MTK_WCN_BOOL mtk_wcn_wmt_func_on(ENUM_WMTDRV_TYPE_T type);

extern MTK_WCN_BOOL mtk_wcn_wmt_dsns_ctrl(ENUM_WMTDSNS_TYPE_T eType);

extern MTK_WCN_BOOL mtk_wcn_wmt_assert(ENUM_WMTDRV_TYPE_T type, UINT32 reason);

extern INT32 mtk_wcn_wmt_msgcb_reg(ENUM_WMTDRV_TYPE_T eType, PF_WMT_CB pCb);

extern INT32 mtk_wcn_wmt_msgcb_unreg(ENUM_WMTDRV_TYPE_T eType);

extern INT32 mtk_wcn_stp_wmt_sdio_op_reg(PF_WMT_SDIO_PSOP own_cb);

extern INT32 mtk_wcn_stp_wmt_sdio_host_awake(VOID);
extern INT8 mtk_wcn_wmt_therm_ctrl(ENUM_WMTTHERM_TYPE_T eType);

extern ENUM_WMTHWVER_TYPE_T mtk_wcn_wmt_hwver_get(VOID);

#else
#define WMT_EXP_HID_API_EXPORT 1
#endif

extern INT32 wmt_lib_set_aif(CMB_STUB_AIF_X aif, MTK_WCN_BOOL share);	/* set AUDIO interface options */
extern VOID wmt_lib_ps_irq_cb(VOID);

extern VOID mtk_wcn_wmt_func_ctrl_for_plat(UINT32 on, ENUM_WMTDRV_TYPE_T type);

extern INT32 mtk_wcn_wmt_system_state_reset(VOID);
extern MTK_WCN_BOOL mtk_wcn_set_connsys_power_off_flag(MTK_WCN_BOOL value);
#ifdef MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT
extern VOID mtk_wcn_wmt_exp_init(VOID);
extern VOID mtk_wcn_wmt_exp_deinit(VOID);
#endif
extern INT8 mtk_wcn_wmt_co_clock_flag_get(VOID);

#endif /* _WMT_EXP_H_ */
