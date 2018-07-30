#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>

#include <json-c/json.h>
#include "vr-jushang_interface.h"

#include "mozart_config.h"
#include "mozart_musicplayer.h"
#include "jushang.h"

#if (SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_JUSHANG)

int jushang_cloudmusic_play(char *key)
{
	int ret, i, music_num, id = 0;
	char *music = NULL;
	const char *music_name = NULL;
	const char *music_url = NULL;
	const char *artists_name = NULL;
	json_object *object, *song, *tmp;
	struct music_info *info = NULL;

	if (key == NULL)
		return -1;

	mozart_jushang_sem(key);
	music = mozart_jushang_get_musiclist();
	if (music == NULL) {
		printf("jushang's music list is empty\n");
		return -1;
	}

	object = json_tokener_parse(music);
	if (object == NULL) {
		printf("%s: json parse failed\n", __func__);
		free(music);
		return -1;
	}

	music_num = json_object_array_length(object);
	for (i = 0; i < music_num; i++) {
		song = json_object_array_get_idx(object, i);
		if (json_object_object_get_ex(song, "songID", &tmp))
			id = json_object_get_int(tmp);
		if (json_object_object_get_ex(song, "songName", &tmp))
			music_name = json_object_get_string(tmp);
		if (json_object_object_get_ex(song, "url", &tmp))
			music_url = json_object_get_string(tmp);
		if (json_object_object_get_ex(song, "artist", &tmp))
			artists_name = json_object_get_string(tmp);

		info = mozart_musiclist_get_info(id, (char *)music_name, (char *)music_url, NULL,
						 NULL, (char *)artists_name, NULL);
		if (info)
			mozart_musicplayer_musiclist_insert(mozart_musicplayer_handler, info);
	}

	ret = mozart_musicplayer_play_index(mozart_musicplayer_handler, 0);

	json_object_put(object);
	free(music);

	return ret;
}

#endif	/* SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_JUSHANG */
