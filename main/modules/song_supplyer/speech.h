#ifndef __SPEECH_H__
#define __SPEECH_H__

#include "vr_interface.h"

extern int speech_cloudmusic_playmusic(sds_music_t *music, int index);
extern int speech_cloudmusic_playfm(sds_netfm_t *netfm, int index);
extern int speech_cloudmusic_playjoke(char *url);
extern int speech_cloudmusic_play(char *key);

#endif	/* __SPEECH_H__ */
