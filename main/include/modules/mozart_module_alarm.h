#ifndef __MOZART_MODULE_ALARM_H__
#define __MOZART_MODULE_ALARM_H__
#include <stdbool.h>
#include "alarm_interface.h"

//有几个参数不是很清楚
typedef struct module_alarm_info {  
	int id;
	char name[32];
    int hour;
	int minute;   
	int date;    //每周那天响应
	bool enable;
	bool repeat;
	char mname[32];
	int mvol;
	int si;   //贪睡的时间间隔
	int sr;  //贪睡次数
	int led;
}module_alarm_info;

extern int alarm_end;
extern int alarm_pause;
extern bool alarm_start;
struct module_alarm_info *mozart_module_alarm_getinfo(char *data);
int alarm_callback(struct alarm_info *a);

#endif /* __MOZART_MODULE_ALARM_H__ */
