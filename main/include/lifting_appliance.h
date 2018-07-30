#ifndef __LIFTING_APPLIANCE_H__
#define __LIFTING_APPLIANCE_H__


extern int lifting_appliance_high;
extern int lifting_appliance_go_high;
extern void lifting_appliance_led(int i);
extern void lifting_appliance_init(void);
extern void lifting_appliance_control(int sign);
void lifting_appliance_start(void);


#endif
