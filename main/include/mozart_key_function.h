#ifndef __MOZART_KEY_FUNCTION_H__
#define __MOZART_KEY_FUNCTION_H__

#include "mozart_config.h"

typedef enum snd_source_t{
	SND_SRC_NONE = -1,
#if (SUPPORT_BT == BT_BCM)
	SND_SRC_BT_AVK,
#endif

#if (SUPPORT_LOCALPLAYER == 1)
	SND_SRC_LOCALPLAY,
#endif

#ifdef SUPPORT_SONG_SUPPLYER
	SND_SRC_CLOUD,
#endif

#if (SUPPORT_INGENICPLAYER == 1)
	SND_SRC_INGENICPLAYER,
#endif
	SND_SRC_LINEIN,
	SND_SRC_MAX,
} snd_source_t;

enum mozart_module_play_status {
	mozart_module_status_stop = 0,
	mozart_module_status_play,
	mozart_module_status_pause,
};

extern snd_source_t snd_source;
extern char *keyvalue_str[];
extern char *keycode_str[];
extern int tfcard_status;

extern char *app_name;
extern snd_source_t snd_source;
extern int mozart_module_pause(void);
extern int mozart_module_stop(void);
extern void mozart_wifi_mode(void);
extern void mozart_config_wifi(void);
extern void mozart_previous_song(void);
extern void mozart_next_song(void);
extern void mozart_play_pause(void);
extern void mozart_volume_up(int i);
extern void mozart_volume_down(int i);
extern void mozart_linein_on(void);
extern void mozart_linein_off(void);
extern void mozart_snd_source_switch(void);
extern void mozart_music_list(int);
extern enum mozart_module_play_status mozart_module_get_play_status(void);
#if(SUPPORT_USB_AUDIO == 1)
extern void mozart_usb_audio_plug_in(void);
extern void mozart_usb_audio_plug_out(void);
#endif
#endif	/* __MOZART_KEY_FUNCTION_H__ */
