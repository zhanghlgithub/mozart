#ifndef __MOZART_UI_H__
#define __MOZART_UI_H__

#include "mozart_config.h"

#ifdef SUPPORT_UI_LCD
#include "lcd_interface.h"
#endif

#if defined (SUPPORT_UI_LCD) || (SUPPORT_UI_SMARTUI) || (SUPPORT_UI_MINIGUI)
void mozart_ui_localplayer_plugin(void);
void mozart_ui_localplayer_plugout(void);
void mozart_ui_linein_plugin(void);
void mozart_ui_linein_plugout(void);
void mozart_ui_cloudplayer(void);
void mozart_ui_bt_connected(void);
void mozart_ui_bt_disconnected(void);
void mozart_ui_bt_avk_play(void);
void mozart_ui_bt_avk_stop(void);
void mozart_ui_asr_wakeup(void);
void mozart_ui_asr_failed(void);
void mozart_ui_asr_unclear(void);
void mozart_ui_asr_result(char *result);
void mozart_ui_net_connecting(void);
void mozart_ui_net_connect_failed(void);
void mozart_ui_net_connected(void);
void mozart_ui_net_config(void);
void mozart_ui_net_config_failed(void);
void mozart_ui_net_config_success(void);
#else
static inline void mozart_ui_localplayer_plugin(void) { }
static inline void mozart_ui_localplayer_plugout(void) { }
static inline void mozart_ui_linein_plugin(void) { }
static inline void mozart_ui_linein_plugout(void) { }
static inline void mozart_ui_cloudplayer(void) { }
static inline void mozart_ui_bt_connected(void) { }
static inline void mozart_ui_bt_disconnected(void) { }
static inline void mozart_ui_bt_avk_play(void) { }
static inline void mozart_ui_bt_avk_stop(void) { }
static inline void mozart_ui_asr_wakeup(void) { }
static inline void mozart_ui_asr_failed(void) { }
static inline void mozart_ui_asr_unclear(void) { }
static inline void mozart_ui_asr_result(char *result) { }
static inline void mozart_ui_net_connecting(void) { }
static inline void mozart_ui_net_connect_failed(void) { }
static inline void mozart_ui_net_connected(void) { }
static inline void mozart_ui_net_config(void) { }
static inline void mozart_ui_net_config_failed(void) { }
static inline void mozart_ui_net_config_success(void) { }
#endif

#endif	/* __MOZART_UI_H__ */
