#ifndef __MOZART_MODULE_MULROOM_H__
#define __MOZART_MODULE_MULROOM_H__

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
	MR_AO_NORMAL,
	MR_AO_DISTRIBUTOR,
} mulroom_ao_mode_t;

/**
 * module_mulroom_audio_change -
 * @mode:
 */
extern int module_mulroom_audio_change(mulroom_ao_mode_t mode);

/**
 * module_mulroom_run_ntpd -
 */
extern int module_mulroom_run_ntpd(void);

#ifdef  __cplusplus
}
#endif

#endif /* __MOZART_MODULE_MULROOM_H__ */
