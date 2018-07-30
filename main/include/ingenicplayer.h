#ifndef __INGENICPLAYER_H__
#define __INGENICPLAYER_H__

#include "mozart_config.h"
#include "appserver.h"
#include "musiclist_interface.h"

#if (SUPPORT_ALARM == 1)
#include "alarm_interface.h"
#include "modules/mozart_module_alarm.h"
#endif

typedef struct reply_app_download_info{
	int list;
	char *local;
	int size;
}Reply_Download;


#if (SUPPORT_INGENICPLAYER == 1)
extern int mozart_ingenicplayer_light_list(int i);
extern int mozart_ingenicplayer_gif_state(unsigned char *data);
extern int mozart_ingenicplayer_light_state(unsigned char *data);
extern int mozart_ingenicplayer_wifi_state(unsigned char *data);
extern int mozart_ingenicplayer_upgrade_state(unsigned char *data);
extern int mozart_ingenicplayer_gif_list(int i);
extern int mozart_ingenicplayer_notify_brain_data();
	

//extern int mozart_ingenicplayer_notify_pos(void);
extern int mozart_ingenicplayer_notify_volume(int volume);
extern int mozart_ingenicplayer_notify_play_mode(enum play_mode mode);
extern int mozart_ingenicplayer_notify_alarm(struct module_alarm_info *alarm);
extern int mozart_ingenicplayer_notify_song_info(void);
extern int mozart_ingenicplayer_notify_get_hight(int i);
extern int mozart_ingenicplayer_notify_work_status(void);
extern int mozart_ingenicplayer_get_med_data(void);
extern int mozart_ingenicplayer_response_cmd(char *command, char *data, struct appserver_cli_info *owner);
extern int mozart_ingenicplayer_startup(void);
extern int mozart_ingenicplayer_shutdown(void);
extern int reply_beiyong(unsigned char *da);
//extern int upload_music(char *data);

extern int up_tower_sysc_time();  //上传同步时间到tower
extern char *new_mozart_music_list;


#else
//static inline int mozart_ingenicplayer_notify_pos(void) { return 0; }
static inline int mozart_ingenicplayer_notify_volume(int volume) { return 0; }
static inline int mozart_ingenicplayer_notify_play_mode(enum play_mode mode) { return 0; }
static inline int mozart_ingenicplayer_notify_song_info(void) { return 0; }
static inline int mozart_ingenicplayer_response_cmd(char *command,
						    char *data, struct appserver_cli_info *owner) { return 0; }
static inline int mozart_ingenicplayer_startup(void) { return 0; }
static inline int mozart_ingenicplayer_shutdown(void) { return 0; }

#endif

#endif /* __INGENICPLAYER_H__ */
