#ifndef __LAPSULE_CONTROL_H__
#define __LAPSULE_CONTROL_H__

#include <stdbool.h>
#include "mozart_config.h"

#ifdef  __cplusplus
extern "C" {
#endif

#if (SUPPORT_LAPSULE == 1)

#define LAPSULE_PROVIDER "lapsule"

	extern int net_flag;
	extern bool lapsule_is_controler(void);
	extern int start_lapsule_app(void);
	extern void stop_lapsule_app(void);
	extern int lapsule_do_wakeup(void);
	extern int lapsule_do_prev_song(void);
	extern int lapsule_do_next_song(void);
	extern int lapsule_do_toggle(void);
	extern int lapsule_do_stop_play(void);
	extern int lapsule_do_set_vol(int vol);
	extern int lapsule_do_play(void);
	extern int lapsule_do_pause(void);
	extern int lapsule_do_search_song(char *song);
	extern int lapsule_do_music_list(int keyIndex);
	extern int lapsule_do_linein_on(void);
	extern int lapsule_do_linein_off(void);

#else  /* SUPPORT_LAPSULE */
	static inline bool lapsule_is_controler(void) { return false; }
	static inline int start_lapsule_app(void) { return 0; }
	static inline void stop_lapsule_app(void) { }
	static inline int lapsule_do_wakeup(void) { return 0; }
	static inline int lapsule_do_prev_song(void) { return 0; }
	static inline int lapsule_do_next_song(void) { return 0; }
	static inline int lapsule_do_toggle(void) { return 0; }
	static inline int lapsule_do_stop_play(void) { return 0; }
	static inline int lapsule_do_set_vol(int vol) { return 0; }
	static inline int lapsule_do_play(void) { return 0; }
	static inline int lapsule_do_pause(void) { return 0; }
	static inline int lapsule_do_search_song(char *song) { return 0; }
	static inline int lapsule_do_music_list(int keyIndex) { return 0; }
	static inline int lapsule_do_linein_on(void) { return 0; }
	static inline int lapsule_do_linein_off(void) { return 0; }
#endif /* SUPPORT_LAPSULE */

#ifdef  __cplusplus
}
#endif

#endif /* __LAPSULE_CONTROL_H__ */
