#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "tips_interface.h"

#include "mozart_config.h"

#define TTS_DEBUG

#define BAIDU 1
#define AISPEECH 2
#define JIETONG 3

#ifndef SUPPORT_TTS
#define SUPPORT_TTS BAIDU
#endif

#if (SUPPORT_TTS == BAIDU)
#define TTS_PROVIDER BAIDU
#elif (SUPPORT_TTS == JIETONG)
#define TTS_PROVIDER JIETONG
#elif (SUPPORT_TTS == AISPEECH)
#define TTS_PROVIDER AISPEECH
#endif

#if (TTS_PROVIDER == BAIDU)
#include <tts_baidu.h>
#elif (TTS_PROVIDER == JIETONG)
#include <tts_jietong_interface.h>
#define TTS_FILE_NAME "/tmp/jt_tts.mp3"
#elif (TTS_PROVIDER == AISPEECH)
#include <vr_interface.h>
#endif

char *encode_url(char *url)
{
	int i = 0;
	char *dst = NULL;

	if (!url)
		return NULL;

	dst = malloc(strlen(url) * 3 + 1);
	if (!dst)
		return NULL;
	dst[0] = '\0';

	for (i = 0; url[i]; i++) {
		switch (url[i]) {
		case ' ':
			strcat(dst, "%20");
			break;
		default:
			strncat(dst, url + i, 1);
			break;
		}
	}

	return dst;
}

static void _mozart_tts(const char *word, bool sync)
{
	char *url = NULL;
	if (word && *word) {
#if (TTS_PROVIDER == BAIDU)
		url = get_baidu_tts_url(word);
			printf("usrl:  %s\n", url);
#elif (TTS_PROVIDER == JIETONG)
		url = malloc(strlen(TTS_FILE_NAME) + 1);
		strcpy(url, TTS_FILE_NAME);
		tts_jietong((char *)word);
#elif (TTS_PROVIDER == AISPEECH)
		url = mozart_aispeech_tts((char *)word);
#endif

#ifdef TTS_DEBUG
		if (url) {
			printf("%s\n", url);
		} else {
			printf("tts error.\n");
			return;
		}
#endif
		if (sync)
			mozart_play_tone_sync(url);
		else
			mozart_play_tone(url);

#if (TTS_PROVIDER != AISPEECH)
		free(url);
#endif
	}
}

void mozart_tts(const char *word)
{
	_mozart_tts(word, false);
}

void mozart_tts_sync(const char *word)
{
	_mozart_tts(word, true);
}

void mozart_tts_wait(void)
{
	mozart_play_tone_sync(NULL);
}
