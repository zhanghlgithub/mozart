#ifndef __MOZART_MODULE_LOCAL_MUSIC_H__
#define __MOZART_MODULE_LOCAL_MUSIC_H__



#include "musiclist_interface.h"

typedef enum mozart_mode{
	FREE=1,
	MED=2,
	SLEEP=3,
	CONFIG_WIFI = 4,
	CONFIG_BT = 5,
	MUSIC_BT = 6,
	MUSIC_DLNA = 7,
	MUSIC_AIRPLAY = 8,
} mozart_mode;

typedef struct Med_Info {
	char name_B[100];   //文件名（路径）
	char name_R[100];
	char name_M[100];
	bool meding;
    int med_time;		//冥想的时长
    int mid;			//EGO编号的音乐ID
	int mid3rd;			//第三方平台音乐的ID
	int cid;
	char mname[100];	//冥想时使用的音乐名称
	int mfrom;			//冥想时音乐的来源
	int zhuanzhu[3600];
	int fangsong[3600];
	char medstarttime[30];   //冥想开始时间
	char medendtime[30];	//冥想结束时间	
}Med_Info;

typedef struct Sleep_Info {
	char name[100];	  //路径的名字
	bool sleeping;   //是否正在助眠
    int med_time;	//助眠的时长
    int mid;		//EGO 编号的音乐 ID
	int mid3rd;		//第三方平台（喜马拉雅）的音乐 ID，针对 EGO 音乐，此项为 0
	int cid;        //音乐所属专辑 ID，第三方音乐是第三方专辑 ID，EGO 音乐是 EGO 课程 ID	
	char mname[100];//助眠时使用的音乐名称
	int mfrom;  	//音乐来源：1-EGO 服务器，2-喜马拉雅："mfrom
	int snooze; 	//唤醒时的贪睡次数
	int alarm_id; 	//闹钟的ID号
	char *alarm;   	//唤醒闹钟的信息
	char sleepstarttime[30];//助眠开始时间？
	char sleependtime[30];  //助眠结束时间？ 
}Sleep_Info;
   
    extern int mozart_duration;
	extern struct Med_Info med_info;    
	extern struct Sleep_Info sleep_info;  
    extern enum mozart_mode mozart_mozart_mode;
	extern enum play_mode mozart_play_mode[4];
	extern int mozart_index;  
	extern void med_info_init(void);
	extern void mozart_start_mode(enum mozart_mode mode);
	extern void mozart_stop_mode(enum mozart_mode mode);
	extern int mozart_module_local_music_start(void);
	extern int mozart_module_local_music_startup(void);



#endif	/* __MOZART_MODULE_LOCAL_MUSIC_H__ */
