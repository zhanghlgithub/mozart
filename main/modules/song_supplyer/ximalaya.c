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
#include "ximalaya_interface.h"

#include "mozart_config.h"
#include "mozart_musicplayer.h"
#include "ximalaya.h"

#if (SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_XIMALAYA)

int ximalaya_cloudmusic_play(char *key)
{
	int ret, i, music_num = 0, id = 0;
	char *music = NULL;
	const char *music_name = NULL;
	const char *music_url = NULL;
	const char *music_picture = NULL;
	const char *albums_name = NULL;
	const char *artists_name = NULL;
	json_object *object, *tracks, *song, *tmp;
	struct music_info *info = NULL;

	if (key == NULL)
		return -1;

	music = mozart_ximalaya_search_music(key, 10);
	if (music == NULL) {
		printf("ximalaya's music list is empty\n");
		return -1;
	}

	object = json_tokener_parse(music);
	if (object == NULL) {
		printf("%s: json parse failed\n", __func__);
		free(music);
		return -1;
	}

	if (json_object_object_get_ex(object, "tracks", &tracks))
		music_num = json_object_array_length(tracks);

	for (i = 0; i < music_num; i++) {
		song = json_object_array_get_idx(tracks, i);
		if (json_object_object_get_ex(song, "id", &tmp))
			id = json_object_get_int(tmp);
		if (json_object_object_get_ex(song, "track_title", &tmp))
			music_name = json_object_get_string(tmp);
		if (json_object_object_get_ex(song, "play_url_32", &tmp))
			music_url = json_object_get_string(tmp);
		if (json_object_object_get_ex(song, "cover_url_large", &tmp))
			music_picture = json_object_get_string(tmp);
		if (json_object_object_get_ex(song, "subordinated_album", &tmp)) {
			if (json_object_object_get_ex(tmp, "album_title", &tmp))
				albums_name = json_object_get_string(tmp);
		}
		if (json_object_object_get_ex(song, "announcer", &tmp)) {
			if (json_object_object_get_ex(tmp, "nickname", &tmp))
				artists_name = json_object_get_string(tmp);
		}

		info = mozart_musiclist_get_info(id, (char *)music_name, (char *)music_url,
						 (char *)music_picture, (char *)albums_name,
						 (char *)artists_name, NULL);
		if (info)
			mozart_musicplayer_musiclist_insert(mozart_musicplayer_handler, info);
	}

	ret = mozart_musicplayer_play_index(mozart_musicplayer_handler, 0);

	json_object_put(object);
	free(music);

	return ret;
}

#endif	/* SUPPORT_SONG_SUPPLYER == SONG_SUPPLYER_XIMALAYA */
