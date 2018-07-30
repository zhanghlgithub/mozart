#include "mozart_config.h"
#include "mozart_ui.h"

void mozart_ui_localplayer_plugin(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(TF_PNG_PATH, TF_PLAY_CN);
#elif defined CONFIG_UI_SMARTUI
#endif
}

void mozart_ui_localplayer_plugout(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(TF_PNG_PATH, TF_REMOVE_CN);
#endif
}

void mozart_ui_linein_plugin(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(LINEIN_PNG_PATH, LINEIN_PLAY_CN);
#endif
}

void mozart_ui_linein_plugout(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(LINEIN_PNG_PATH, LINEIN_DISCONNECT_CN);
#endif
}

void mozart_ui_cloudplayer(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(WIFI_PNG_PATH, WIFI_PLAY_CN);
#endif
}

void mozart_ui_bt_connected(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(BT_PNG_PATH, BT_PLAY_CN);
#endif
}

void mozart_ui_bt_disconnected(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(BT_PNG_PATH, BT_DISCONNECT_CN);
#endif
}

void mozart_ui_bt_avk_play(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(BT_PNG_PATH, BT_PLAY_CN);
#endif
}

void mozart_ui_bt_avk_stop(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(BT_PNG_PATH, BT_DISCONNECT_CN);
#endif
}

void mozart_ui_asr_wakeup(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(NULL, VR_WAKE_UP_CN);
#endif
}

void mozart_ui_asr_failed(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(NULL, VR_FAILED_CN);
#endif
}

void mozart_ui_asr_unclear(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(NULL, VR_UNCLEAR_SPEECH_CN);
#endif
}

void mozart_ui_asr_result(char *result)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(NULL, result);
#endif
}

void mozart_ui_net_connecting(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(NULL, WIFI_CONNECTING_CN);
#endif
}

void mozart_ui_net_connect_failed(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(NULL, WIFI_CONNECT_FAILED_CN);
#endif
}

void mozart_ui_net_connected(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(NULL, WIFI_CONNECTED_CN);
#endif
}

void mozart_ui_net_config(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(NULL, WIFI_CONFIG_CN);
#endif
}

void mozart_ui_net_config_failed(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(NULL, WIFI_CONFIG_FAIL_CN);
#endif
}

void mozart_ui_net_config_success(void)
{
#ifdef SUPPORT_UI_LCD
	mozart_lcd_display(NULL, WIFI_CONFIG_SUC_CN);
#endif
}
