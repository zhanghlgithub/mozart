#ifndef __EGOSERVER_H__
#define __EGOSERVER_H__

typedef struct VER {
	char *ver;
	char *time;
	char *url;
	int force;
    char *desc;
}VER;
typedef struct WEATHER {
	int res;  
	char *city;  //城市编号
	int temp;    //温度
	int wind;	//风向
	int weather; //天气
	int day_url;
	int night_url;
	int windLevel;	//风力编码
	char *date;   //实时天气信息
}WEATHER;

extern FILE *fp_B;
extern FILE *fp_R;
extern FILE *fp_M;
extern int wifi_status;
extern struct VER new_ver;
extern struct WEATHER weather;
extern unsigned char weather_data[4];
extern int binduserId(void);
extern void med_data_up(void);
extern void sleep_data_up(void);
extern void  med_file_open(void);
extern void  med_file_close(void);
extern int  conserve_sleep_data(void);
extern int  syncmusicdata(void);
extern void egoserverstart(void);

extern int getweather();  //修改（4.28号）



#endif
