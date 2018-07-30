#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define BAIDU_TTS_URL "http://tts.baidu.com/text2audio?cuid=48-5A-B6-47-0A-BB&lan=zh&ctp=1&pdt=90&tex="
extern char *encode_url(char *url);

char *get_baidu_tts_url(const char *word)
{
	char *url = NULL;
	char *url_encode = NULL;
	url = malloc(strlen(BAIDU_TTS_URL) + strlen(word) + 1);
	if (!url) {
		printf("malloc for baidu-tts-url error: %s.\n", strerror(errno));
		return NULL;
	}

	sprintf(url, "%s%s", BAIDU_TTS_URL, word);
	url_encode = encode_url(url);
	free(url);
	return url_encode;
}

