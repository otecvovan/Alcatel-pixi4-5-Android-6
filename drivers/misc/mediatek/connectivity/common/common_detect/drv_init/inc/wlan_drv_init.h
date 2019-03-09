
#ifndef _WLAN_DRV_INIT_H_
#define _WLAN_DRV_INIT_H_


extern int do_wlan_drv_init(int chip_id);

extern int mtk_wcn_wmt_wifi_init(void);

#ifdef MTK_WCN_WLAN_GEN2
extern int mtk_wcn_wlan_gen2_init(void);
#endif
#ifdef MTK_WCN_WLAN_GEN3
extern int mtk_wcn_wlan_gen3_init(void);
#endif

#endif
