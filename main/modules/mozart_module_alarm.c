#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "json-c/json.h"
#include "modules/mozart_module_alarm.h"
#include "mozart_key_function.h"
#include "mozart_musicplayer.h"
#include "ingenicplayer.h"
#include "egoserver.h"

#include "modules/mozart_module_local_music.h"
#include "modules/mozart_module_bt.h"



int alarm_end=0;
int alarm_pause=0;
bool alarm_start = false;	//新添加于2018.7.16号
//struct alarm_info *aNew;




//注：使用次函数的时候千万别忘了释放内存。按照（0x26）对发来的数据做校验
//功能：把app传来的数据剖析成报文要求的格式！！
struct module_alarm_info *mozart_module_alarm_getinfo(char *data)
{
	struct module_alarm_info *malarm = NULL;
  	json_object *object = NULL;
  	json_object *tmp = NULL;
  
  	malarm = calloc(sizeof(struct module_alarm_info) , 1);   //在哪里释放的？？，每次使用是的时候，使用完释放！！
  	if ((object = json_tokener_parse( data)) == NULL)
		return malarm;

    if (json_object_object_get_ex(object, "id", &tmp))
		malarm->id = json_object_get_int(tmp);
		
	if (json_object_object_get_ex(object, "date", &tmp))
		malarm->date = json_object_get_int(tmp);
	if( malarm->date > 127)        //为什么呢？？？
		malarm->date = 127;
		
    if (json_object_object_get_ex(object, "mvol", &tmp))	      
    	malarm->mvol = json_object_get_int(tmp);
	if( malarm->mvol >100)      //为什么呢？
	    malarm->mvol = 100;
			   
    if (json_object_object_get_ex(object, "si", &tmp))		
        malarm->si = json_object_get_int(tmp);
    if( malarm->si > 30)
		malarm->si = 30;
		
    if (json_object_object_get_ex(object, "sr", &tmp))	   	
        malarm->sr = json_object_get_int(tmp);
	if( malarm->sr > 10)
		malarm->sr= 10;
		
    if (json_object_object_get_ex(object, "hour", &tmp))	   	
        malarm->hour = json_object_get_int(tmp);
		
    if (json_object_object_get_ex(object, "minute", &tmp))	   	
        malarm->minute = json_object_get_int(tmp);
		 
    if (json_object_object_get_ex(object, "led", &tmp))
		malarm->led = json_object_get_int(tmp);

    if (json_object_object_get_ex(object, "en", &tmp))
		malarm->enable=  json_object_get_boolean(tmp);

	if (json_object_object_get_ex(object, "re", &tmp))
		malarm->repeat=  json_object_get_boolean(tmp);  

    if (json_object_object_get_ex(object, "name", &tmp))
		strncpy(malarm->name, json_object_get_string(tmp),strlen( json_object_get_string(tmp)));

	if (json_object_object_get_ex(object, "mn", &tmp))
	    strncpy(malarm->mname, json_object_get_string(tmp),strlen( json_object_get_string(tmp)));

    json_object_put(object);   //没初始化也要释放object的内存空间嘛？？？
	return malarm; 
}

//功能：将app传来的数据保存到struct alarm_info结构体里，并返回出来。
struct alarm_info *mozart_get_alarm_info(char *data)
{
	int len = 0;
	struct alarm_info *alarm = NULL;
	json_object *object = NULL;
	json_object *tmp = NULL;
	
	if ((object = json_tokener_parse(data)) == NULL)
		return alarm;
	
    len = strlen(data);
	alarm = calloc(sizeof(struct alarm_info) + len + 1, 1);   //其实没必要+1的；
	strncpy(alarm->prv_info.info, data, len);
    alarm->prv_info.len = len;

	if (json_object_object_get_ex(object, "hour", &tmp))
	       alarm->hour=json_object_get_int(tmp);

	if (json_object_object_get_ex(object, "hour", &tmp))
		alarm->minute=json_object_get_int(tmp);   	

	if (json_object_object_get_ex(object, "date", &tmp))
		alarm->week_active = json_object_get_int(tmp);

	if (json_object_object_get_ex(object, "repeat", &tmp))
		alarm->weekly_repeat =  json_object_get_boolean(tmp);

	if (json_object_object_get_ex(object, "enable", &tmp))
		alarm->enable=  json_object_get_boolean(tmp);

	if (json_object_object_get_ex(object, "id", &tmp))
		alarm->alarm_id = json_object_get_int(tmp);

    if (json_object_object_get_ex(tmp, "name", &tmp))
    {
		strncpy(alarm->name, json_object_get_string(tmp), strlen( json_object_get_string(tmp)));
    }
	  
	alarm->timestamp=0;
    json_object_put(object); 
	return alarm;
}

//功能：关闭闹钟
#if 1
static int mozart_module_alarm_stop(struct alarm_info *a,int id,player_status_t r_status,int r_index,int r_cursor,int r_volume)
{
	printf("\n alarm_stop \n");
	unsigned char DATA[2];
	if(id==-1)
 	{
    	mozart_alarm_delete(a);
 	}
 	mozart_musicplayer_play_index(mozart_musicplayer_handler, r_index);	
 	mozart_musicplayer_set_volume(mozart_musicplayer_handler, r_volume); 
 	mozart_musicplayer_set_seek(mozart_musicplayer_handler, r_cursor); 
	if(r_status!=PLAYER_PLAYING)
    {
 		usleep(100000);
       	mozart_musicplayer_play_pause_pause(mozart_musicplayer_handler);
 	}

	DATA[0] = 0;
	DATA[1] = 0;
	Pack_write(Light_effect,DATA,0);   //关闭tower发送灯光效果
	
	if(mozart_mozart_mode==SLEEP)
	{
#if 0
  		if(conserve_sleep_data()){
			printf("#### Error:【%s, %s, %d】 ####\n",__FILE__, __func__, __LINE__);
		}
#endif
	}
	alarm_start = false;	//新添加于2018.7.16号
  	return 0;
}

//贪睡模式
static int mozart_module_alarm_sleep(struct alarm_info *a,int id,player_status_t r_status,int r_index,int r_cursor,int r_volume)
{
	json_object *object = json_object_new_object();
	struct module_alarm_info *alarm = NULL;
  	struct alarm_info *a_sleep=NULL;
  	int len;
  	int hour,minute,week_active;

 	alarm=mozart_module_alarm_getinfo(a->prv_info.info);
//	printf("\n 【alarm_sleep】：%s \n",alarm);
   
   	if(id==-1)
 	{
    	mozart_alarm_delete(a);
 	}

   	hour=alarm->hour;
   	minute=alarm->minute+alarm->si;
   	week_active=127;
  	if(minute>=60)
  	{
  		minute-=60;
        hour+=1;
		if(hour>=24)
		{
        	hour-=24;
			week_active*=2;
			if(week_active>127)
			week_active-=127;	
		}
  	}
  
   	json_object_object_add(object, "id", json_object_new_int(-1));
   	json_object_object_add(object, "date", json_object_new_int(week_active));
   	json_object_object_add(object, "mvol", json_object_new_int(alarm->mvol));
   	json_object_object_add(object, "si", json_object_new_int(alarm->si));
   	json_object_object_add(object, "sr", json_object_new_int(alarm->sr-1));
   	json_object_object_add(object, "hour", json_object_new_int(hour));
   	json_object_object_add(object, "minute", json_object_new_int(minute));
   	json_object_object_add(object, "led", json_object_new_int(alarm->led));
   	json_object_object_add(object, "en", json_object_new_boolean(alarm->enable));
   	json_object_object_add(object, "re", json_object_new_boolean(alarm->repeat));
   	json_object_object_add(object, "name", json_object_new_string(alarm->name));
   	json_object_object_add(object, "mn", json_object_new_string(alarm->mname));

    len=strlen(json_object_get_string(object));
   	a_sleep = calloc(sizeof(struct alarm_info) + len + 1, 1);
	strncpy(a_sleep->prv_info.info, json_object_get_string(object), len);
    a_sleep->prv_info.len = len;
	   
	a_sleep->alarm_id=-1;
  	a_sleep->hour=hour;
  	a_sleep->minute=minute;
  	a_sleep->week_active=week_active;
  	a_sleep->enable=alarm->enable;	
 	printf("xxxxxxxx  this is a->enable：%d\n",a->enable);
  	a_sleep->weekly_repeat=alarm->repeat;

	printf("【asleep】:hour:%d,minute:%d",a_sleep->hour,a_sleep->minute);
	mozart_alarm_add(a_sleep);
	mozart_musicplayer_play_index(mozart_musicplayer_handler, r_index);	
 	mozart_musicplayer_set_volume(mozart_musicplayer_handler, r_volume); 
 	mozart_musicplayer_set_seek(mozart_musicplayer_handler, r_cursor); 
	if(r_status!=PLAYER_PLAYING)
    {
 		usleep(500000);
       	mozart_musicplayer_play_pause_pause(mozart_musicplayer_handler);
 	}
	alarm_start = false;	//新添加于2018.7.16号
  	sleep_info.snooze++;
 	free(alarm);   //别忘了释放内存
	alarm = NULL;
	free(a_sleep);
	a_sleep = NULL;

	return 0;
}
#endif

#if 0
int alarm_callback(struct alarm_info *a)
{
		alarm_end=0;
  		alarm_pause=0;
		char *url=NULL;
		int r_volume=0;
		int r_index=0;
		int r_cursor=0;
		player_status_t r_status;
		struct module_alarm_info *alarm = NULL;
	//	struct music_info *info = NULL;
	//	struct module_alarm_info *alarm = NULL;
		
		r_status=mozart_player_getstatus(mozart_musicplayer_handler->player_handler); 
		r_volume=mozart_musicplayer_get_volume(mozart_musicplayer_handler);  //获得当前音乐播放的音量
		r_index=mozart_musicplayer_musiclist_get_current_index(mozart_musicplayer_handler);  //获得当前音乐在音乐列表的位置
		r_cursor=mozart_musicplayer_get_pos(mozart_musicplayer_handler);  //获取当前一首音乐播放的位置
		alarm=mozart_module_alarm_getinfo(a->prv_info.info);	//获取闹钟的信息
	
		url = malloc(strlen("/mnt/sdcard/alarm/") + strlen(alarm->mname));
		sprintf(url, "/mnt/sdcard/alarm/%s",alarm->mname);	
		printf("\n--------------------alarm_Callback--------------------------------------\n");   
		printf("alarm callback id: %d, hour: %d, minute: %d, timestamp: %ld info: %s  url:%s\n",
			   a->alarm_id, a->hour, a->minute, a->timestamp, a->prv_info.info,url);
	
		printf("%d,%d,%d,%d\n",r_volume,r_index,r_cursor,r_status);
		mozart_ingenicplayer_notify_alarm(alarm);  //将闹钟信息回复给app
	
	//	info = mozart_musiclist_get_info(0, (char *)alarm->name, (char *)url,NULL,NULL, NULL, NULL);
		
		if(r_status == PLAYER_PLAYING)
		{
			mozart_musicplayer_play_pause_pause(mozart_musicplayer_handler);
		}
		mozart_player_playurl(mozart_musicplayer_handler->player_handler, url); //播放url路径下的音乐？？？ 
	//	mozart_musicplayer_play_music_info(mozart_musicplayer_handler, info);
		
	//	mozart_musicplayer_set_volume(mozart_musicplayer_handler, alarm->mvol); //设置音量	
		do{ 	
			sleep(1);
			printf("\n.........this is enter do while...............");
			if(alarm_end)
			{
				mozart_module_alarm_stop(a,alarm->id,r_status,r_index,r_cursor,r_volume);
				break;
			}
			if(alarm_pause) 
			{
				if(alarm->sr==0)
					mozart_module_alarm_stop(a,alarm->id,r_status,r_index,r_cursor,r_volume);
				else{
					printf("%s %s %d 开始贪睡模式\n",__FILE__, __func__, __LINE__);
					mozart_module_alarm_sleep(a,alarm->id,r_status,r_index,r_cursor,r_volume);	
				}
				break;
			}
	//		if(mozart_player_getstatus(mozart_musicplayer_handler->player_handler) != PLAYER_PLAYING)
	//			break;
		}while(1);
		free(alarm);
		alarm = NULL;
		free(url);
		url = NULL;
		return 0;

}
#endif

#if 1
void *mozart_choice_alarm_sleepORstop(void *arg)
{
	unsigned char DATA[2];
	struct alarm_info *aNew = (struct alarm_info *)arg;
	char *url=NULL;
	int r_volume=0;
	int r_index=0;
	int r_cursor=0;
	player_status_t r_status;
	struct module_alarm_info *alarm = NULL;
	struct music_info *info = NULL;
	//struct module_alarm_info *alarm = NULL;

	r_status=mozart_musicplayer_get_status(mozart_musicplayer_handler); 
    r_volume=mozart_musicplayer_get_volume(mozart_musicplayer_handler);  //获得当前音乐播放的音量
	r_index=mozart_musicplayer_musiclist_get_current_index(mozart_musicplayer_handler);  //获得当前音乐在音乐列表的位置
	r_cursor=mozart_musicplayer_get_pos(mozart_musicplayer_handler);  //获取当前一首音乐播放的位置
	alarm=mozart_module_alarm_getinfo(aNew->prv_info.info);    //获取闹钟的信息

    url = malloc(strlen("/mnt/sdcard/alarm/") + strlen(alarm->mname));
    sprintf(url, "/mnt/sdcard/alarm/%s",alarm->mname);	
	printf("\n--------------------alarm_Callback--------------------------------------\n");   
	printf("alarm callback id: %d, hour: %d, minute: %d, timestamp: %ld info: %s  url:%s\n",
		   aNew->alarm_id, aNew->hour, aNew->minute,aNew->timestamp, aNew->prv_info.info,url);

	mozart_ingenicplayer_notify_alarm(alarm);  //将闹钟信息回复给app

	info = mozart_musiclist_get_info(0, (char *)alarm->name, (char *)url,NULL,NULL,NULL, NULL);
	
	if(r_status == PLAYER_PLAYING)
	{
		mozart_musicplayer_play_pause_pause(mozart_musicplayer_handler);
	}
	//mozart_player_playurl(mozart_musicplayer_handler->player_handler, url); //播放url路径下的音乐？？？ 
	mozart_musicplayer_play_music_info(mozart_musicplayer_handler, info);

	//添加唤醒灯光。（5.29）
	if(alarm->led == 0)
	{
       	DATA[0] = 0;
	  	DATA[1] = 0;
	  	Pack_write(Light_effect,DATA,0);   //给tower发送灯光效果
	}
	else
	{
    	DATA[0] = 1;
		DATA[1] = alarm->led;
		Pack_write(Light_effect,DATA,0);
	}
	
	//mozart_musicplayer_set_volume(mozart_musicplayer_handler, alarm->mvol); //设置音量	
	do{		
		sleep(1);
		printf("\n.........this is enter do while...............");
       	if(alarm_end)     //如果选择停止按钮
       	{
       		mozart_module_alarm_stop(aNew,alarm->id,r_status,r_index,r_cursor,r_volume);
			alarm_end = 0;
            break;
       	}
		if(alarm_pause)     //如果选择贪睡按钮
		{
			if(alarm->sr==0)
		 		mozart_module_alarm_stop(aNew,alarm->id,r_status,r_index,r_cursor,r_volume);
		 	else{
				printf("%s %s %d 开始贪睡模式\n",__FILE__, __func__, __LINE__);
				if(alarm->si == 0){        //如果贪睡的时间间隔为0，则不执行贪睡
					if(mozart_musicplayer_get_status(mozart_musicplayer_handler) == PLAYER_PLAYING)
					{
						mozart_musicplayer_play_pause_pause(mozart_musicplayer_handler);
					}
					break;
				}
				else{
            		mozart_module_alarm_sleep(aNew,alarm->id,r_status,r_index,r_cursor,r_volume);	
				}
			}
			alarm_pause = 0;
			break;
		}
		if( mozart_musicplayer_get_status(mozart_musicplayer_handler) != PLAYER_PLAYING)
			break;
	}while(1);
	free(alarm);
	alarm = NULL;
	free(url);
	url = NULL;
//	free(aNew);
//	aNew = NULL;
	return NULL;
}
#endif

#if 1
int alarm_callback(struct alarm_info *a)
{ 
	struct alarm_info *aNew = (struct alarm_info *)malloc(2048);
	memcpy(aNew, a, 2048);
	printf("........alarm callback id: %d, hour: %d, minute: %d, timestamp: %ld info: %s  ",
			   aNew->alarm_id, aNew->hour, aNew->minute, aNew->timestamp, aNew->prv_info.info);

	
	pthread_t pthread_handle;
    alarm_end=0;
    alarm_pause=0;
	alarm_start = true;
	if (pthread_create(&pthread_handle, NULL, mozart_choice_alarm_sleepORstop, (void *)aNew )){
		printf("create ingenicplayer_thread failed\n");
		return -1;
	}
	pthread_detach(pthread_handle);
//	printf("第二遍........alarm callback id: %d, hour: %d, minute: %d, timestamp: %ld info: %s  ",
//			   aNew->alarm_id, aNew->hour, aNew->minute, aNew->timestamp, aNew->prv_info.info);
	return 0;
}
#endif
