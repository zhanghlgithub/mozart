#ifndef __BATTERY_CAPACITY_H__
#define __BATTERY_CAPACITY_H__

#include <unistd.h>

#ifdef  __cplusplus
extern "C" {
#endif

       extern int capacity;
       void power_led_set(int i);
       void power_off();
	void register_battery_capacity(void);
	void unregister_battery_capacity(void);

#ifdef  __cplusplus
}
#endif
#endif	/* __BATTERY_CAPACITY_H__ */
