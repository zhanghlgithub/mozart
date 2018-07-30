#include <stdio.h>
#include <stdlib.h>

#include "vr-jushang_interface.h"
#include "tips_interface.h"
#include "mozart_config.h"
#include "lapsule_control.h"
#include "mozart_key_function.h"
#include "mozart_tts.h"
#include "mozart_ui.h"
#include "mozart_musicplayer.h"
#include "modules/mozart_module_cloud_music.h"
#include "sharememory_interface.h"

#if(SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_XIMALAYA)
#include "song_supplyer/ximalaya.h"
#endif
#if(SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_JUSHANG)
#include "song_supplyer/jushang.h"
#endif
#if(SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_AISPEECH)
#include "song_supplyer/speech.h"
#endif

#ifdef SUPPORT_SONG_SUPPLYER
static char *artists[] = {"王菲", "刘德华", "陈奕迅", "汪峰",
			  "刘德华", "凤凰传奇", "张学友"};

static int mozart_module_cloud_music_start(void)
{
	if (snd_source != SND_SRC_CLOUD) {
		if (mozart_module_stop())
			return -1;
		if (mozart_musicplayer_start(mozart_musicplayer_handler))
			return -1;
		mozart_ui_localplayer_plugin();
		snd_source = SND_SRC_CLOUD;
	} else if (!mozart_musicplayer_is_active(mozart_musicplayer_handler)) {
		if (mozart_musicplayer_start(mozart_musicplayer_handler))
			return -1;
	} else {
		mozart_musicplayer_musiclist_clean(mozart_musicplayer_handler);
	}

	return 0;
}

int mozart_module_cloud_music_search_and_play(char *key)
{
	if (key == NULL) {
		fprintf(stderr, "key is null\n");
		return -1;
	}

	if (mozart_module_cloud_music_start())
		return -1;

#if (SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_XIMALAYA)
	return ximalaya_cloudmusic_play(key);
#elif (SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_LAPSULE)
	lapsule_do_search_song(key);
	return 0;
#elif (SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_JUSHANG)
	return jushang_cloudmusic_play(key);
#elif (SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_AISPEECH)
	return speech_cloudmusic_play(key);
#else
	return 0;
#endif
}

/* TODO: cache url */
int mozart_module_cloud_music_startup(void)
{
	memory_domain domain;
	int ret;
	char buf[1024] = {};
	int idx = random() % (sizeof(artists) / sizeof(artists[0]));

	ret = share_mem_get_active_domain(&domain);
	if(ret != 0 || domain != 0)
		return 0;
	if (mozart_module_cloud_music_start())
		return -1;
	ret = share_mem_get_active_domain(&domain);
	mozart_play_key("cloud_mode");    //进入云播放模式
	sprintf(buf, "欢迎回来，为您播放%s的歌", artists[idx]);
	ret = share_mem_get_active_domain(&domain);
	if(ret != 0 || domain != 0)
		return 0;
	mozart_tts(buf);
	mozart_tts_wait();

#if (SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_XIMALAYA)
	return ximalaya_cloudmusic_play(artists[idx]);
#elif (SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_LAPSULE)
	return 0;
#elif (SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_JUSHANG)
	return jushang_cloudmusic_play(artists[idx]);
#elif (SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_AISPEECH)
	return speech_cloudmusic_play(artists[idx]);
#else
	return 0;
#endif
}

#endif	/* SUPPORT_SONG_SUPPLYER */
