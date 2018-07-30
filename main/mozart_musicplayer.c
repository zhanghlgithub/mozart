#include <stdio.h>
#include <string.h>

#include "sharememory_interface.h"
#include "event_interface.h"
#include "utils_interface.h"
#include "tips_interface.h"

#include "mozart_config.h"
#include "ingenicplayer.h"
#include "mozart_musicplayer.h"
#include "modules/mozart_module_local_music.h"
#include "modules/mozart_module_bt.h"
#include "lifting_appliance.h"
#include "egoserver.h"

#include "mozart_key_function.h"	//新添加于2018.7.11号


//#define MOZART_MUSICPLAYER_DEBUG 
#ifdef MOZART_MUSICPLAYER_DEBUG
#define pr_debug(fmt, args...)				\
	printf("[MUSICPLAYER] [DEBUG] {%s, %d}: "fmt, __func__, __LINE__, ##args)
#else  /* MOZART_MUSICPLAYER_DEBUG */
#define pr_debug(fmt, args...)			\
	do {} while (0)
#endif	/* MOZART_MUSICPLAYER_DEBUG */

#define pr_info(fmt, args...)						\
	printf("[MUSICPLAYER] [INFO] "fmt, ##args)

#define pr_err(fmt, args...)						\
	fprintf(stderr, "[MUSICPLAYER] [ERROR] {%s, %d}: "fmt, __func__, __LINE__, ##args)

#define musicplayer_lock(lock)						\
	do {                                                            \
		int i = 0;                                              \
		while (pthread_mutex_trylock(lock)) {                   \
			if (i++ >= 100) {                               \
				pr_err("#### {%s, %s, %d} dead lock (last: %d)####\n", \
				       __FILE__, __func__, __LINE__, last_lock_line); \
				i = 0;                                  \
			}                                               \
			usleep(100 * 1000);                             \
		}                                                       \
		last_lock_line = __LINE__;				\
	} while (0)

#define musicplayer_unlock(lock)					\
	do {												\
		pthread_mutex_unlock(lock);				\
	} while (0)

#define VOLUME_VARIATION 10

musicplayer_handler_t *mozart_musicplayer_handler;

static bool autoplay_flag;
static int in_musicplayer;
static int last_lock_line;
static player_handler_t current_player_handler;
static player_status_t musicplayer_status;
static struct music_list *musicplayer_list = NULL;
static pthread_t music_status_event;
static pthread_mutex_t musicplayer_mutex = PTHREAD_MUTEX_INITIALIZER;

int reply_time = 1;

static mozart_event mozart_musicplayer_event = {
	.event = {
		.misc = {
			.name = "musicplayer",
		},
	},
	.type = EVENT_MISC,
};

static int __mozart_musicplayer_play_music_info(musicplayer_handler_t *handler,
						struct music_info *info);

static int check_handler_uuid(musicplayer_handler_t *handler)
{
	if (handler == NULL || handler->player_handler == NULL) {
		pr_err("handler is null\n");
		return -1;
	}

	if (strcmp(current_player_handler.uuid, handler->player_handler->uuid)) {
		pr_debug("%s can't control, controler is %s\n", handler->player_handler->uuid,
			 current_player_handler.uuid);
		return -1;
	}

	return 0;
}

bool mozart_musicplayer_is_active(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	ret = check_handler_uuid(handler);
	musicplayer_unlock(&musicplayer_mutex);

	return ret ? false : true;
}
/**********************************************************************************************
 * music player list
 *********************************************************************************************/
static void free_data(void *arg)
{
	return;
}

int mozart_musicplayer_musiclist_insert(musicplayer_handler_t *handler, struct music_info *info)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = mozart_musiclist_insert(musicplayer_list, info);
	musicplayer_unlock(&musicplayer_mutex);

	/* TODO-tjiang: notify song queue ? */

	return ret;
}

int mozart_musicplayer_musiclist_delete_index(musicplayer_handler_t *handler, int index)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = mozart_musiclist_delete_index(musicplayer_list, index);
	musicplayer_unlock(&musicplayer_mutex);

	/* TODO-tjiang: notify song queue ? */

	return ret;
}

//功能：获取当前音乐播放模式：顺序播放，列表循环，随机播放
enum play_mode mozart_musicplayer_musiclist_get_play_mode(musicplayer_handler_t *handler)
{
	enum play_mode mode;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_debug("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return play_mode_order;
	}

	mode = mozart_musiclist_get_play_mode(musicplayer_list);
	musicplayer_unlock(&musicplayer_mutex);

	return mode;
}

//音乐列表播放模式设置：1-单曲播放、2-单曲循环、3-顺序播放、4-列表循环、5-随机播放。
int mozart_musicplayer_musiclist_set_play_mode(musicplayer_handler_t *handler, enum play_mode mode)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = mozart_musiclist_set_play_mode(musicplayer_list, mode);
	musicplayer_unlock(&musicplayer_mutex);

	mozart_ingenicplayer_notify_play_mode(mode);

	return ret;
}

int mozart_musicplayer_musiclist_get_length(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_debug("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = mozart_musiclist_get_length(musicplayer_list);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

int mozart_musicplayer_musiclist_get_current_index(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_debug("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = mozart_musiclist_get_current_index(musicplayer_list);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

//获取当前正在播放的音乐的信息，返回值是当前正在播放的音乐的信息
struct music_info *mozart_musicplayer_musiclist_get_current_info(musicplayer_handler_t *handler)
{
	struct music_info *info;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_debug("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return NULL;
	}

	info = mozart_musiclist_get_current(musicplayer_list);
	musicplayer_unlock(&musicplayer_mutex);

	return info;
}

//获取index结点的信息，并返回index结点处的音乐信息
struct music_info *mozart_musicplayer_musiclist_get_index_info(musicplayer_handler_t *handler,
							       int index)
{
	struct music_info *info;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_debug("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return NULL;
	}

	info = mozart_musiclist_get_index(musicplayer_list, index);
	musicplayer_unlock(&musicplayer_mutex);

	return info;
}
								   
//清空播放列表
int mozart_musicplayer_musiclist_clean(musicplayer_handler_t *handler)
{
	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	mozart_musiclist_clean(musicplayer_list);
	musicplayer_unlock(&musicplayer_mutex);

	return 0;
}

int mozart_musicplayer_play_shortcut(musicplayer_handler_t *handler, int index)
{
	char shortcut_name[20] = {};

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	sprintf(shortcut_name, "shortcut_%d", index);
	//mozart_ingenicplayer_notify_play_shortcut(shortcut_name);
	musicplayer_unlock(&musicplayer_mutex);

	return 0;
}

//功能：播放音乐列表位于第index处的音乐文件
int mozart_musicplayer_play_index(musicplayer_handler_t *handler, int index)
{
	int ret;
	struct music_info *info = NULL;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	info = mozart_musiclist_set_index(musicplayer_list, index);
	if (info == NULL) {
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = __mozart_musicplayer_play_music_info(handler, info);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

//功能：播放上一首音乐
int mozart_musicplayer_prev_music(musicplayer_handler_t *handler)
{
	int ret;
	struct music_info *info = NULL;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	info = mozart_musiclist_set_prev(musicplayer_list);
	if (info == NULL) {
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = __mozart_musicplayer_play_music_info(handler, info);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

static int __mozart_musicplayer_next_music(musicplayer_handler_t *handler, bool force)
{
	struct music_info *info = NULL;

	info = mozart_musiclist_set_next(musicplayer_list, force);
	if (info == NULL)
		return -1;

	return __mozart_musicplayer_play_music_info(handler, info);
}

/* for stop */
int mozart_musicplayer_next_music_false(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = __mozart_musicplayer_next_music(handler, false);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

int mozart_musicplayer_next_music(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = __mozart_musicplayer_next_music(handler, true);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

/**********************************************************************************************
 * music player handler
 *********************************************************************************************/
 //获取该音乐的音长
int mozart_musicplayer_get_duration(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return PLAYER_PAUSED;
	}

	ret = mozart_player_getduration(handler->player_handler);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

static int __mozart_musicplayer_get_pos(musicplayer_handler_t *handler)
{
	return mozart_player_getpos(handler->player_handler);
}

//获取当前音乐播放的位置
int mozart_musicplayer_get_pos(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return PLAYER_PAUSED;
	}

	ret = __mozart_musicplayer_get_pos(handler);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

//功能：从某个歌曲的具体位置开始放，seek是开始的秒数。
int mozart_musicplayer_set_seek(musicplayer_handler_t *handler, int seek)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return PLAYER_PAUSED;
	}

	ret = mozart_player_seek(handler->player_handler, seek);
	musicplayer_unlock(&musicplayer_mutex);

	//if (ret == 0)
		//mozart_ingenicplayer_notify_pos();

	return ret;
}

player_status_t mozart_musicplayer_get_status(musicplayer_handler_t *handler)
{
	player_status_t status;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return PLAYER_PAUSED;
	}

	status = mozart_player_getstatus(handler->player_handler);
	musicplayer_unlock(&musicplayer_mutex);

	return status;
}

static int __mozart_musicplayer_get_volume(musicplayer_handler_t *handler)
{
	return mozart_player_volget(handler->player_handler);
}

//获取当前音乐的播放音量
int mozart_musicplayer_get_volume(musicplayer_handler_t *handler)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return 0;
	}

	ret = __mozart_musicplayer_get_volume(handler);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

static int __mozart_musicplayer_set_volume(musicplayer_handler_t *handler, int volume)
{
	int ret;

	ret = mozart_player_volset(handler->player_handler, volume);

	if (ret == 0)
		mozart_ingenicplayer_notify_volume(__mozart_musicplayer_get_volume(handler));

	return ret;
}

int mozart_musicplayer_set_volume(musicplayer_handler_t *handler, int volume)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = __mozart_musicplayer_set_volume(handler, volume);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

int mozart_musicplayer_volume_up(musicplayer_handler_t *handler)
{
	int ret, volume;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	volume = __mozart_musicplayer_get_volume(handler) + VOLUME_VARIATION;
	if (volume > 100)
		volume = 100;
	else if (volume < 0)
		volume = 0;
	ret = __mozart_musicplayer_set_volume(handler, volume);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

int mozart_musicplayer_volume_down(musicplayer_handler_t *handler)
{
	int ret, volume;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	volume = __mozart_musicplayer_get_volume(handler) - VOLUME_VARIATION;
	if (volume > 100)
		volume = 100;
	else if (volume < 0)
		volume = 0;
	ret = __mozart_musicplayer_set_volume(handler, volume);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

//功能：开始播放
int mozart_musicplayer_play_pause_play(musicplayer_handler_t *handler)
{
    int ret = -1;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return PLAYER_PAUSED;
	}
	
	ret = mozart_player_resume(handler->player_handler);
	snprintf(mozart_musicplayer_event.event.misc.type, 16, "resume");
	
	if (mozart_event_send(mozart_musicplayer_event) < 0)
		pr_err("%s: Send event fail\n", __func__);

	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

//功能：暂停音乐播放。
int mozart_musicplayer_play_pause_pause(musicplayer_handler_t *handler)
{
    int ret = -1;
	
	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return PLAYER_PAUSED;
	}

	ret = mozart_player_pause(handler->player_handler);
	snprintf(mozart_musicplayer_event.event.misc.type, 16, "pause");

    if (mozart_event_send(mozart_musicplayer_event) < 0)
		pr_err("%s: Send event fail\n", __func__);

	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}
//如果播放就暂停，如果暂停就播放
int mozart_musicplayer_play_pause(musicplayer_handler_t *handler)
{
	int ret = -1;
	player_status_t status;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		musicplayer_unlock(&musicplayer_mutex);
		return PLAYER_PAUSED;
	}

	status = mozart_player_getstatus(handler->player_handler);
	if (status == PLAYER_PLAYING) {
		ret = mozart_player_pause(handler->player_handler);
		snprintf(mozart_musicplayer_event.event.misc.type, 16, "pause");
	} else if (status == PLAYER_PAUSED) {
		ret = mozart_player_resume(handler->player_handler);
		snprintf(mozart_musicplayer_event.event.misc.type, 16, "resume");
	}

	if (mozart_event_send(mozart_musicplayer_event) < 0)
		pr_err("%s: Send event fail\n", __func__);

	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

int mozart_musicplayer_stop_playback(musicplayer_handler_t *handler)
{
	autoplay_flag = false;
	if (mozart_player_stop(handler->player_handler)) {
		pr_err("send stop cmd failed\n");
		return -1;
	}

	if (AUDIO_OSS == get_audio_type()) {
		int cnt = 500;
		while (cnt--) {
			if (musicplayer_status == PLAYER_STOPPED)
				break;
			usleep(10 * 1000);
		}
		if (cnt < 0) {
			pr_err("waiting for stopped status timeout.\n");
			return -1;
		}
	}
	if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_RUNNING))
		pr_err("share_mem_set failure.\n");

	return 0;
}

//功能：播放当前info的音乐
static int __mozart_musicplayer_play_music_info(musicplayer_handler_t *handler,
		struct music_info *info)
{
	int i, ret;
	if(in_musicplayer) {
		if (mozart_player_playurl(handler->player_handler, info->music_url)) {
			pr_err("player play url failed\n");
			return -1;
		}
	} else {
		__attribute__((unused)) struct music_info *dump;
		module_status domain_status;

		share_mem_set(MUSICPLAYER_DOMAIN, WAIT_RESPONSE);
		snprintf(mozart_musicplayer_event.event.misc.type,
				sizeof(mozart_musicplayer_event.event.misc.type), "play");
		if (mozart_event_send(mozart_musicplayer_event))
			pr_err("send musicplayer play event failure\n");

		/* wait RENDER_DOMAIN become to RESPONSE_DONE status for 10 seconds. */
		for (i = 0; i < 1000; i++) {
			ret = share_mem_get(MUSICPLAYER_DOMAIN, &domain_status);
			pr_debug("domain_status: %s.\n", module_status_str[domain_status]);
			if (!ret)
				if (domain_status == RESPONSE_DONE ||
						domain_status == RESPONSE_PAUSE ||
						domain_status == RESPONSE_CANCEL)
					break;

			usleep(10 * 1000);
		}

		if (i == 1000) {
			pr_err("Wait musicplayer play event reponse done timeout, will not play music");
			return -1;
		}

		pr_debug("========================================================================\n");
		pr_debug("url: %s, len: %d\n", info->music_url,
				mozart_musiclist_get_length(musicplayer_list));
		for (i = 0; i < mozart_musiclist_get_length(musicplayer_list); i++) {
			dump = mozart_musiclist_get_index(musicplayer_list, i);
			pr_debug("%d: %s, %s\n", i, dump->music_name, dump->music_url);
		}
		pr_debug("========================================================================\n");

		switch (domain_status) {
			case RESPONSE_DONE:
				if (mozart_player_playurl(handler->player_handler, info->music_url)) {
					pr_err("player play url failed\n");
					return -1;
				}
				break;
			case RESPONSE_PAUSE:
				if (mozart_player_cacheurl(handler->player_handler, info->music_url)) {
					pr_err("player play url failed\n");
					return -1;
				}
				break;
			case RESPONSE_CANCEL:
			default:
				pr_err("musicplayer response cancel\n");
				return -1;
		}
		in_musicplayer = 1;
	}

	autoplay_flag = true;

	snprintf(mozart_musicplayer_event.event.misc.type, 16, "playing");
	if (mozart_event_send(mozart_musicplayer_event) < 0)
		pr_err("%s: Send event fail\n", __func__);

	return 0;
}

int mozart_musicplayer_play_music_info(musicplayer_handler_t *handler, struct music_info *info)
{
	int ret;

	musicplayer_lock(&musicplayer_mutex);
	if (check_handler_uuid(handler) < 0) {
		pr_err("check handler uuid failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	ret = __mozart_musicplayer_play_music_info(handler, info);
	musicplayer_unlock(&musicplayer_mutex);

	return ret;
}

//当音乐播放状态改变的时候执行的回调函数
static int musicplayer_status_change_callback(player_snapshot_t *snapshot,
					      struct player_handler *handler, void *param)
{
	musicplayer_handler_t *musicplayer_handler = (musicplayer_handler_t *)param;
	unsigned char da[2];

	if (strcmp(handler->uuid, snapshot->uuid)) {
		pr_debug("%s can't control, controler is %s\n", handler->uuid, snapshot->uuid);
		if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_RUNNING))
			pr_err("share_mem_set failure.\n");
		musicplayer_status = PLAYER_STOPPED;
		return 0;
	}

	musicplayer_status = snapshot->status;

	pr_debug("mozart musicplayer recv player status: %d, %s.\n",
		 snapshot->status, player_status_str[snapshot->status]);
    mozart_ingenicplayer_notify_work_status();		//通知给app当前工作的状态
	switch (snapshot->status) {
	case PLAYER_TRANSPORT:
	case PLAYER_PLAYING:
		if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_PLAYING))
			pr_err("share_mem_set failure.\n");
		     da[0]=mozart_mozart_mode;
		     da[1]=0x01;
		     Pack_write(Base_station,da,2);		//发送给tower状态机的状态
		     printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);
		break;
	case PLAYER_PAUSED:
		if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_PAUSE))
			pr_err("share_mem_set failure.\n");
		     da[0]=mozart_mozart_mode;
		     da[1]=0x00;
		     Pack_write(Base_station,da,2);
			 printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);
		break;
	case PLAYER_STOPPED:
		if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_RUNNING))
			pr_err("share_mem_set failure.\n");
		if (autoplay_flag)
			mozart_musicplayer_next_music_false(musicplayer_handler);
		else
	
		break;
	case PLAYER_UNKNOWN:
		musicplayer_status = PLAYER_STOPPED;
		if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_RUNNING))
			pr_err("share_mem_set failure.\n");
		break;
	default:
		musicplayer_status = PLAYER_STOPPED;
		pr_err("Unsupported status: %d.\n", snapshot->status);
		break;
	}
	return 0;
}

int mozart_musicplayer_start(musicplayer_handler_t *handler)
{
	if (handler == NULL || handler->player_handler == NULL) {
		pr_err("handler is null\n");
		return -1;
	}

	musicplayer_lock(&musicplayer_mutex);

	if (strcmp(current_player_handler.uuid, "invalid")) {
		pr_err("current handler isn't invalid\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	memcpy(current_player_handler.uuid, handler->player_handler->uuid,
	       sizeof(current_player_handler.uuid));
	pr_debug("current is %s\n", current_player_handler.uuid);

	musicplayer_unlock(&musicplayer_mutex);

	return 0;
}

int mozart_musicplayer_stop(musicplayer_handler_t *handler)
{
	printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);
	musicplayer_lock(&musicplayer_mutex);
	in_musicplayer = 0;
	if (!strcmp(current_player_handler.uuid, "invalid")) {
		musicplayer_unlock(&musicplayer_mutex);
		return 0;
	}
	if (strcmp(current_player_handler.uuid, handler->player_handler->uuid)) {
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	pr_debug("stop %s\n", current_player_handler.uuid);
	if (mozart_player_getstatus(handler->player_handler) != PLAYER_STOPPED)
		mozart_musicplayer_stop_playback(handler);
	mozart_musiclist_clean(musicplayer_list);
	strcpy(current_player_handler.uuid, "invalid");
	musicplayer_unlock(&musicplayer_mutex);

	return 0;
}

//功能：将音乐的状态回复给APP，单独的一个线程，(每秒钟执行一次)。
static void *music_status_reply_thread(void *arg)
{
	player_status_t status;
    time_t timep;
    struct info *info;
    while(1)
    {
		status =  mozart_musicplayer_get_status(mozart_musicplayer_handler);
	    if (status == PLAYER_PLAYING) 
		{   
			/*如果是正在播放歌曲，就给APP发送歌曲的信息，包括：播放状态，播放时间，播放进度，歌曲信息，【冥想数据】*/
			mozart_ingenicplayer_notify_song_info(); 	//逻辑：如果有歌曲正在播放，通知app当前歌曲的播放状态
		} 
		if(mozart_mozart_mode == SLEEP)
		{
			if(sleep_info.sleeping)
			{
            	if(sleep_info.med_time>=mozart_duration)
                {
					mozart_musicplayer_play_pause_pause(mozart_musicplayer_handler);
                    sleep_info.sleeping=false;
					if(sleep_info.alarm_id==0)
					{	
						//conserve_sleep_data();	//修改于2018.7.9号
						mozart_start_mode(FREE);  //新添加的于5.12号
					}
                }
				else
				{
					/*添加于5.17号*/
					if(mozart_musicplayer_get_status(mozart_musicplayer_handler) == PLAYER_PLAYING)
					{
						sleep_info.med_time++;
					}
				}
			}
		}
	    if((mozart_mozart_mode == FREE||mozart_mozart_mode == SLEEP) && get_cuband_state())   //检测到有脑电数据过来的时候开始进行冥想训练
        {
			mozart_start_mode(MED);
			system("echo 1 > /sys/class/leds/led-sleep/brightness");
			med_info.meding = false;			//逻辑：代表用户是否按冥想按钮
            usleep(50000);
            mozart_ingenicplayer_notify_work_status();  //回复给app当前的音乐的工作状态
        }
 
        if(mozart_mozart_mode == MED)
        {

		/*****************************新添加于2018.6.25号**********************************************/
			mozart_ingenicplayer_notify_brain_data();
		/*********************************************************************************************/

			info = get_meddata_form_ble();
            if((int)(info->X_brain_attention)==0)
            {
             	//printf("\n  x:%d",(int)(info->X_brain_attention));
				lifting_appliance_go_high=0; 
                lifting_appliance_led(lifting_appliance_go_high);  
            }
			else
			{	
				if(info->Tower_power > 20)	//修改于2018.7.30号
				{
					lifting_appliance_go_high=((int)(info->X_brain_attention))/20+1;
					//printf("\n  x:%d",(int)(info->X_brain_attention));	
					if(lifting_appliance_go_high==6)
					lifting_appliance_go_high=5;
					lifting_appliance_led(lifting_appliance_go_high);
					if(lifting_appliance_go_high==1||lifting_appliance_go_high==2)
					lifting_appliance_go_high=3;
				}
			}            
			if(!get_cuband_state())  //发带断开连接执行，返回0表示未连接，1表示已连接
			{
				printf("发带断开连接关闭灯光嘛................\n");
            	lifting_appliance_control(2);
				lifting_appliance_led(6);			  
				mozart_start_mode(FREE);		
				info->X_brain_attention=0x00;
				lifting_appliance_go_high=0;
                usleep(50000);
				mozart_ingenicplayer_notify_work_status();
			}         	
			if(med_info.meding)
		   	{
            	info = get_meddata_form_ble();
                med_info.zhuanzhu[med_info.med_time]=(int)(info->X_brain_concentration);
                med_info.fangsong[med_info.med_time]=(int)(info->X_brain_attention);
				//逻辑：如果训练时长大于30秒，且一首歌播放完毕
				printf("【 %d 】 med_info.med_time：%d   播放时长：%d,总时间长：%d\n",__LINE__,med_info.med_time,
						mozart_musicplayer_get_pos(mozart_musicplayer_handler),mozart_musicplayer_get_duration(mozart_musicplayer_handler));
 	           	if((med_info.med_time>30) && (mozart_musicplayer_get_pos(mozart_musicplayer_handler) > (mozart_musicplayer_get_duration(mozart_musicplayer_handler)-3)))
				{
					printf("【%s】 %s %d  med_info.med_time：%d\n",__FILE__, __func__, __LINE__,med_info.med_time);
			 		med_info.meding = false;
                	time(&timep);
		            sprintf(med_info.medendtime, "%d-%d-%d %d:%d:%d",localtime(&timep)->tm_year+1900,localtime(&timep)->tm_mon+1,
	 	                                                                                                    localtime(&timep)->tm_mday,localtime(&timep)->tm_hour,
	 	                                                                                                    localtime(&timep)->tm_min,localtime(&timep)->tm_sec);
				   	med_file_close();	
					/*******************************修改于2018.7.10号*****************************************************/
				    //if ( mozart_musicplayer_get_status(mozart_musicplayer_handler) != PLAYER_STOPPED)
		             //           mozart_musicplayer_stop_playback(mozart_musicplayer_handler);  
		             if ( mozart_musicplayer_get_status(mozart_musicplayer_handler) != PLAYER_PAUSED)   
	                        mozart_play_pause();
					 		//mozart_musicplayer_play_pause_pause(mozart_musicplayer_handler);

					 /************************************************************************************************************/
				   	mozart_ingenicplayer_notify_work_status();
				   	mozart_ingenicplayer_get_med_data();	//逻辑：在和app连接的情况下将冥想数据发给app
		        }
		
                med_info.med_time++; 
                if(med_info.med_time==5)
			    med_info_init();
		   	}
        }
		
		//sleep(reply_time);	
		sleep(1);   //每隔一秒钟给APP做一次回复。
    }
	return NULL;
}

static void music_status_reply(void)
{
	if(0 != pthread_create(&music_status_event, NULL, music_status_reply_thread, NULL))
		printf("Can't create music status reply!\n");
	pthread_detach(music_status_event);
}

int mozart_musicplayer_startup(void)
{
	pr_debug("...\n");

	musicplayer_lock(&musicplayer_mutex);
	if (mozart_musicplayer_handler) {
		pr_info("musicplayer is running\n");
		musicplayer_unlock(&musicplayer_mutex);
		return 0;
	}

	musicplayer_list = mozart_musiclist_create(free_data);	//创建了音乐播放的队列，所有需要播放的音乐都是对这个列表的操作

	if (share_mem_init() != 0)
		pr_err("share_mem_init failed\n");

	if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_RUNNING))
		pr_err("share_mem_set MUSICPLAYER_DOMAIN failed\n");

	mozart_musicplayer_handler = calloc(sizeof(*mozart_musicplayer_handler), 1);
	if (mozart_musicplayer_handler == NULL) {
		pr_err("calloc musicplayer handler failed\n");
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}
	
	//回调函数 musicplayer_status_change_callback
	mozart_musicplayer_handler->player_handler = mozart_player_handler_get(
		"musicplayer", musicplayer_status_change_callback, mozart_musicplayer_handler);
	if (mozart_musicplayer_handler->player_handler == NULL) {
		pr_err("get player handler failed\n");
		free(mozart_musicplayer_handler);
		mozart_musicplayer_handler = NULL;
		musicplayer_unlock(&musicplayer_mutex);
		return -1;
	}

	strcpy(current_player_handler.uuid, "invalid");
	musicplayer_unlock(&musicplayer_mutex);

	music_status_reply();     //连上网络后将音乐的状态恢复给app

	return 0;
}

int mozart_musicplayer_shutdown(void)
{
	pr_debug("...\n");

	musicplayer_lock(&musicplayer_mutex);
	if (mozart_musicplayer_handler == NULL) {
		pr_info("ingenic player not running\n");
		musicplayer_unlock(&musicplayer_mutex);
		return 0;
	}

	if (check_handler_uuid(mozart_musicplayer_handler) == 0 &&
	    mozart_player_getstatus(mozart_musicplayer_handler->player_handler) != PLAYER_STOPPED)
		mozart_musicplayer_stop_playback(mozart_musicplayer_handler);
	strcpy(current_player_handler.uuid, "invalid");

	mozart_player_handler_put(mozart_musicplayer_handler->player_handler);
	memset(mozart_musicplayer_handler, 0, sizeof(*mozart_musicplayer_handler));

	printf("\n%s:free \n",__func__);
	free(mozart_musicplayer_handler);
	mozart_musicplayer_handler = NULL;

	mozart_musiclist_destory(musicplayer_list);

	if (share_mem_set(MUSICPLAYER_DOMAIN, STATUS_SHUTDOWN))
		pr_err("share_mem_set failed\n");

	musicplayer_unlock(&musicplayer_mutex);

	return 0;
}
