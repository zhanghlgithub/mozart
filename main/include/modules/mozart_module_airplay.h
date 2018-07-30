#ifndef __MOZART_MODULE_AIRPLAY_H__
#define __MOZART_MODULE_AIRPLAY_H__

/**
 * @brief Start shairport service
 *
 * @return return 0 on successfully, -1 otherwise.
 */
extern int mozart_airplay_start_service(void);

/**
 * @brief Stop shairport music player
 *
 * @return return 0 on successfully, -1 otherwise.
 */
extern int mozart_airplay_stop_playback(void);

/**
 * @brief Pause shairport music player
 *
 * @return return 0 on successfully, -1 otherwise.
 */
extern int mozart_airplay_pause(void);
/**
 * @brief Resume shairport music player
 *
 * @return return 0 on successfully, -1 otherwise.
 */
extern int mozart_airplay_resume(void);

extern int mozart_airplay_play_pause(void);
/**
 * @brief Close airplay service.
 *
 * @return return 0 on successfully, -1 otherwise.
 */
extern int mozart_airplay_shutdown(void);

#endif /* __MOZART_MODULE_AIRPLAY_H__ */
