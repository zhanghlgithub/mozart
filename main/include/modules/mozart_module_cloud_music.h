#ifndef __MOZART_MODULE_CLOUD_MUSIC_H__
#define __MOZART_MODULE_CLOUD_MUSIC_H__

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef SUPPORT_SONG_SUPPLYER
	extern int mozart_module_cloud_music_search_and_play(char *key);
	extern int mozart_module_cloud_music_startup(void);
#else
	static inline int mozart_module_cloud_music_search_and_play(char *key) { return 0; }
	static inline int mozart_module_cloud_music_startup(void) { return 0; }
#endif

#ifdef  __cplusplus
}
#endif

#endif	/* __MOZART_MODULE_CLOUD_MUSIC_H__ */
