#ifndef __MOZART_MODULE_DMR_H__
#define __MOZART_MODULE_DMR_H__

#include <stdbool.h>
#include "render_interface.h"

#ifdef  __cplusplus
extern "C" {
#endif

	extern bool qplay_is_controler(void);
	extern bool dmr_is_started(void);
	extern void dmr_start(void);

#ifdef  __cplusplus
}
#endif

#endif	/* __MOZART_MODULE_DMR_H__ */
