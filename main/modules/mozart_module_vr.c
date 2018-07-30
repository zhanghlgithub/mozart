#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "sharememory_interface.h"
#include "wifi_interface.h"
#include "tips_interface.h"
#include "power_interface.h"
#include "lapsule_control.h"
#include "utils_interface.h"
#include "ini_interface.h"
#include "json-c/json.h"

#include "mozart_config.h"
#include "appserver.h"

#include "mozart_app.h"
#include "mozart_ui.h"
#include "mozart_tts.h"
#include "mozart_key_function.h"
#include "mozart_musicplayer.h"
#include "mozart_camera.h"

#ifdef SUPPORT_BT
#include "modules/mozart_module_bt.h"
#endif

#if (SUPPORT_DMR == 1)
#include "modules/mozart_module_dmr.h"
#endif

#include "modules/mozart_module_vr.h"
#include "modules/mozart_module_cloud_music.h"

#include "vr_interface.h"
#include "volume_interface.h"
#include "tips_interface.h"
#include "song_supplyer/speech.h"

static bool need_resume_after_asr = false;

static vr_result_t process_vr_aec_callback(aec_info_t *aec)
{
	vr_result_t result;
	memset(&result, 0, sizeof(result));
      printf("-----------------------\n");
	if (aec->wakeup) {
		printf("aec wakeup.\n");
		//FIXME: maybe not paused here.
		mozart_module_pause();

		mozart_play_key_sync("welcome");
		result.vr_next = VR_TO_ASR;
	} else {
		/* aec error, back to AEC again. */
		result.vr_next = VR_TO_AEC;
	}

	return result;
}

static void process_vr_asr_command_command(asr_info_t *asr)
{
	sem_command_t *command = &asr->sem.request.command;

	/* response volume first. */
	if (command->volume) {
		if (!strcmp(command->volume, "+")) {
			mozart_volume_up(10);
			mozart_play_key_sync("volume_up");
		} else if (!strcmp(command->volume, "-")) {
			mozart_volume_down(10);
			mozart_play_key_sync("volume_down");
		} else if (!strcmp(command->volume, "max")) {
			mozart_volume_set(100, MUSIC_VOLUME);
			mozart_play_key_sync("volume_max");
		} else if (!strcmp(command->volume, "min")) {
			mozart_volume_set(10, MUSIC_VOLUME);
			mozart_play_key_sync("volume_min");
		} else if (command->volume[0] >= '0' && command->volume[0] <= '9') {
			mozart_volume_set(atoi(command->volume), MUSIC_VOLUME);
		} else {
			printf("TODO: unsupport volume set format: %s.\n", command->volume);
		}
	}

	/* then response operation */
	if (command->operation) {
		if (strcmp(command->operation, "暂停") == 0) {
			printf("暂停\n");
			mozart_module_pause();
			//FIXME: maybe not paused here.
			mozart_play_key_sync("paused");
			need_resume_after_asr = false;
		} else if (strcmp(command->operation, "播放") == 0) {
			mozart_play_key_sync("resume");
			mozart_play_pause();
			need_resume_after_asr = false;
		} else if (strcmp(command->operation, "继续") == 0) {
			mozart_play_key_sync("resume");
			mozart_play_pause();
			need_resume_after_asr = false;
		} else if (strcmp(command->operation, "停止") == 0) {
			mozart_module_stop();
			//FIXME: maybe not stop here.
			mozart_play_key_sync("stopped");
			need_resume_after_asr = false;
		} else if (strcmp(command->operation, "上一首") == 0) {
			mozart_play_key_sync("previous_song");
			mozart_previous_song();
			need_resume_after_asr = false;
		} else if (strcmp(command->operation, "下一首") == 0) {
			mozart_play_key_sync("next_song");
			mozart_next_song();
			need_resume_after_asr = false;
		} else if (strcmp(command->operation, "退出") == 0) {
			mozart_play_key_sync("exit");
		} else if (strcmp(command->operation, "结束") == 0) {
			mozart_play_key_sync("exit");
		} else {
			printf("小乐暂时不支持该领域指令：%s\n", command->operation);
			mozart_play_key_sync("error_invalid_domain");
		}
	}

	return;
}

static void process_vr_asr_command(asr_info_t *asr)
{
	printf("sem.request.domain: %s\n", vr_domain_str[asr->sem.request.domain]);

	switch (asr->sem.request.domain) {
	case DOMAIN_MUSIC:
		printf("小乐暂时不支持该领域指令：%s\n", vr_domain_str[asr->sem.request.domain]);
		mozart_play_key_sync("error_invalid_domain");
		break;
	case DOMAIN_MOVIE:
		printf("小乐暂时不支持该领域指令：%s\n", vr_domain_str[asr->sem.request.domain]);
		mozart_play_key_sync("error_invalid_domain");
		break;
	case DOMAIN_NETFM:
		printf("小乐暂时不支持该领域指令：%s\n", vr_domain_str[asr->sem.request.domain]);
		mozart_play_key_sync("error_invalid_domain");
		break;
	case DOMAIN_COMMAND:
		process_vr_asr_command_command(asr);
		break;
	default:
		printf("小乐暂时不支持该领域指令：%d\n", asr->sem.request.domain);
		mozart_play_key_sync("error_invalid_domain");
		break;
	}

	return;
}

static vr_result_t process_vr_asr_callback(asr_info_t *asr)
{
	vr_result_t result;
	memset(&result, 0, sizeof(result));

	printf("asr result, domain: %s.\n", vr_domain_str[asr->sds.domain]);

	/* (default)resume music after response done */
	need_resume_after_asr = true;

	switch (asr->sds.domain) {
	case DOMAIN_MUSIC:
		{
#if (SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_AISPEECH)
			if (asr->sds.music.number >= 1) {
				char tips[128] = {};
				int index = 0;
				if (asr->sds.music.number > 1) {
					srandom(time(NULL));
					index = random() % asr->sds.music.number;
				}
				/* play tts */
				sprintf(tips, "%s,%s", asr->sds.music.data[index].artist, asr->sds.music.data[index].title);
				mozart_tts_sync(tips);

				speech_cloudmusic_playmusic(&asr->sds.music, index);
				need_resume_after_asr = false;
			} else {
				/* TODO: command, such as volume control. */
				/* TODO: speak 我想听, will recieve a tips, process as multi chat. */
				mozart_tts_sync(asr->sds.output);
			}
#else
			char key[128] = {};
			char tips[128] = {};
			char *artist = asr->sem.request.music.artist;
			char *title = asr->sem.request.music.title;
			if (!artist && !title) {
				strcpy(key, "我想听歌");
				strcpy(tips, "为您播放随机音乐");
			} else if (artist && !title) {
				sprintf(key, "%s的歌", artist);
				sprintf(tips, "为您播放%s的歌", artist);
			} else if (!artist && title) {
				sprintf(key, "我想听%s", title);
				sprintf(tips, "为您播放%s", title);
			} else {
				sprintf(key, "%s的%s", artist, title);
				sprintf(tips, "为您播放%s的%s", artist, title);
			}

			mozart_tts_sync(tips);
			mozart_module_cloud_music_search_and_play(key);
			asr->sds.state = SDS_STATE_OFFER;
			need_resume_after_asr = false;
#endif
		}
		break;
	case DOMAIN_NETFM:
                mozart_play_key_sync("error_invalid_domain");
		/*if (asr->sds.netfm.number >= 1) {
			int index = 0;
			if (asr->sds.netfm.number > 1) {
				srandom(time(NULL));
				index = random() % asr->sds.netfm.number;
			}
			
			mozart_tts_sync(asr->sds.netfm.data[index].track);

			speech_cloudmusic_playfm(&asr->sds.netfm, index);

			need_resume_after_asr = false;
		} else {
			
			mozart_tts_sync(asr->sds.output);
		}*/
		break;
	case DOMAIN_CHAT:
                mozart_play_key_sync("error_invalid_domain");
	/*	if (!strcmp(asr->input, "")) {
			mozart_play_key_sync("error_no_voice");
			asr->sds.state = SDS_STATE_OFFER;
			break;
		}

		if (asr->sds.chat.url) {

			speech_cloudmusic_playjoke(asr->sds.chat.url);

			asr->sds.state = SDS_STATE_OFFER;

			need_resume_after_asr = false;
		} else {
			printf("output: %s.\n", asr->sds.output);
			mozart_tts_sync(asr->sds.output);
		}*/
		break;
	case DOMAIN_WEATHER:
	case DOMAIN_CALENDAR:
	case DOMAIN_STOCK:
	case DOMAIN_POETRY:
	case DOMAIN_MOVIE:
		printf("output: %s.\n", asr->sds.output);
		mozart_tts_sync(asr->sds.output);
		break;
	case DOMAIN_REMINDER:
		printf("提醒类功能未实现.\n");
		mozart_play_key_sync("error_invalid_domain");
		break;
	case DOMAIN_COMMAND:
		process_vr_asr_command(asr);
		break;
	default:
		mozart_play_key_sync("error_invalid_domain");
		break;
	}

	if (asr->sds.state == SDS_STATE_OFFERNONE ||
		asr->sds.state == SDS_STATE_INTERACT) {
		result.vr_next = VR_TO_ASR_SDS;
		mozart_play_key_sync("asr_continue");
	} else {
		if (need_resume_after_asr) {
			printf("do not need resume.\n");
			mozart_play_pause();
		}
		result.vr_next = VR_TO_AEC;
	}

	return result;
}

static int process_vr_content_callback(content_info_t *content)
{
	printf("content result, domain: %s.\n", vr_domain_str[content->sds.domain]);

	switch (content->sds.domain) {
	case DOMAIN_MUSIC:
		{
			if (content->sds.music.number >= 1) {
				char tips[128] = {};
				int index = 0;
				if (content->sds.music.number > 1) {
					srandom(time(NULL));
					index = random() % content->sds.music.number;
				}
				/* play tts */
				sprintf(tips, "%s,%s", content->sds.music.data[index].artist, content->sds.music.data[index].title);
				mozart_tts_sync(tips);

				speech_cloudmusic_playmusic(&content->sds.music, index);
			}
		}
		break;
	case DOMAIN_NETFM:
		if (content->sds.netfm.number >= 1) {
			int index = 0;
			if (content->sds.netfm.number > 1) {
				srandom(time(NULL));
				index = random() % content->sds.netfm.number;
			}
			/* play tts */
			mozart_tts_sync(content->sds.netfm.data[index].track);

			speech_cloudmusic_playfm(&content->sds.netfm, index);
		} else {
			/* TODO: command, such as volume control. */
			mozart_tts_sync(content->sds.output);
		}
		break;
	case DOMAIN_CHAT:
		if (content->sds.chat.url) {
			/* joke, is offer */
			speech_cloudmusic_playjoke(content->sds.chat.url);
		}
		break;
	default:
		printf("Unhandled domain: %s.\n", vr_domain_str[content->sds.domain]);
		break;
	}

	return 0;
}

static void *process_vr_content_func(void *arg)
{
	pthread_detach(pthread_self());

	vr_info_t *vr_info = (vr_info_t *)arg;

	if (vr_info->content.state == CONTENT_SUCCESS) {
		process_vr_content_callback(&vr_info->content);
	} else if (vr_info->content.state == CONTENT_FAIL) {
		printf("content errId: %d, error: %s\n",
			   vr_info->content.errId, vr_info->content.error);
		switch (vr_info->content.errId) {
		case 70604:
		case 70605:
			mozart_play_key_sync("error_net_fail");
			break;
		case 70603:
		case 70613:
			mozart_play_key_sync("error_net_slow_wait");
			break;
		default:
			mozart_play_key_sync("error_server_busy");
			break;
		}
	} else if (vr_info->content.state == CONTENT_INTERRUPT) {
		printf("get content request has been break.\n");
	}
	content_free(&vr_info->content);

	return NULL;
}

vr_result_t process_vr_callback(vr_info_t *vr_info)
{
	vr_result_t result;
	memset(&result, 0, sizeof(result));
       printf("==================vr==============================");
	if (vr_info->from == VR_FROM_AEC) {
		return process_vr_aec_callback(&vr_info->aec);
	} else if (vr_info->from == VR_FROM_ASR) {
		switch (vr_info->asr.errId) {
		case 70604:
		case 70605:
			mozart_play_key_sync("error_net_fail");
			break;
		case 70603:
		case 70613:
			mozart_play_key_sync("error_net_slow_wait");
			break;
		case 0:
			if (vr_info->asr.state == ASR_BREAK) {
				printf("asr interrupt.\n");
				break;
			} else {
				return process_vr_asr_callback(&vr_info->asr);
			}
		default:
			printf("errId: %d, error: %s\n", vr_info->asr.errId, vr_info->asr.error);
			mozart_play_key_sync("error_server_busy");
			break;
		}
		result.vr_next = VR_TO_AEC;
		return result;
	} else if (vr_info->from == VR_FROM_CONTENT) {
		result.vr_next = VR_TO_NULL;
		pthread_t process_vr_content_thread;

		if (pthread_create(&process_vr_content_thread, NULL, process_vr_content_func, vr_info) == -1)
			printf("Create process vr content pthread failed: %s.\n", strerror(errno));

		/* return-ed result will be ignored */
		return result;
	} else {
		printf("Unsupport callback source: %d, back to AEC mode.\n", vr_info->from);
		result.vr_next = VR_TO_AEC;
		return result;
	}
}
