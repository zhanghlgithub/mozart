#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/statfs.h> 
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <curl/curl.h>

#include "ini_interface.h"
#include "updater_interface.h"
#include "json-c/json.h"
#include "utils_interface.h"
#include "linklist_interface.h"
#include "localplayer_interface.h"
#include "lifting_appliance.h"
#include "tips_interface.h"

#include "mozart_config.h"
#include "ingenicplayer.h"
#include "mozart_musicplayer.h"
#include "mozart_app.h"
#include "mozart_key_function.h"
#include "egoserver.h"
#include "battery_capacity.h"

#include "modules/mozart_module_local_music.h"
#include "modules/mozart_module_bt.h"


#define MOZART_INGENICPLAYER_DEBUG 
#ifdef MOZART_INGENICPLAYER_DEBUG
#define pr_debug(fmt, args...)				\
	printf("[INGENICPLAYER] %s %d: "fmt, __func__, __LINE__, ##args)
#else  /* MOZART_INGENICPLAYER_DEBUG */
#define pr_debug(fmt, args...)			\
	do {} while (0)
#endif	/* MOZART_INGENICPLAYER_DEBUG */

#define pr_info(fmt, args...)						\
	printf("[INGENICPLAYER] [INFO] "fmt, ##args)

#define pr_err(fmt, args...)						\
	fprintf(stderr, "[INGENICPLAYER] [ERROR] %s %d: "fmt, __func__, __LINE__, ##args)

#define DEVICENAME "/usr/data/ingenicplayer/devicename"
#define DEVICE_INGENICPLAYER_DIR "/usr/data/ingenicplayer/"

#define DEVICE_NAME_FILENAME "/usr/data/ingenicplayer/devicename"

enum {
	PK_AB_ENTER_MED_MODE = 0,
	PK_AB_START_MED,
	PK_AB_STOP_MED,
	PK_AB_GET_MED_DATA,
	PK_AB_GET_MED_DATA_RES,
	
	PK_AB_ENTER_SLEEP_MODE,
	PK_AB_EXIT_SLEEP_MODE,
	PK_AB_PAUSE_ALARM,
	PK_AB_STOP_ALARM,
	PK_AB_GET_ALARM_LIST,
	PK_BA_ALARM_LIST,
	PK_AB_ADD_ALARM,
	PK_BA_ADD_ALARM_RES,
	PK_AB_DEL_ALARM,
	PK_BA_DEL_ALARM_RES,
	PK_AB_EDIT_ALARM,
	PK_BA_EDIT_ALARM_RES,
	PK_AB_GET_LED_EFFECT_LIST,
	PK_BA_LED_EFFECT_LIST,

	PK_AB_ENTER_FREE_MODE,
	PK_AB_MUSIC_CTL_CMD,
	PK_BA_MUSIC_STATUS,
	PK_AB_SET_MUSIC_VOLUME,
	PK_BA_MUSIC_VOLUME,
	PK_AB_PLAY_MUSIC,
	PK_BA_PLAY_MUSIC_RES,
	PK_AB_GET_MUSIC_LIST,
	PK_BA_MUSIC_LIST,
	PK_AB_GET_MUSIC_FILE_NUM,
	PK_BA_MUSIC_FILE_NUM,
	PK_AB_DEL_MUSIC,
	PK_BA_DEL_MUSIC_RES,
	PK_AB_EDIT_MUSIC,
	PK_BA_EDIT_MUSIC_RES,
	PK_AB_MOVE_MUSIC,
	PK_BA_MOVE_MUSIC_RES,
	PK_AB_UPLOAD_MUSIC,
	PK_BA_UPLOAD_MUSIC_STATUS,
	PK_AB_MUSIC_LOOP,
	PK_BA_MUSIC_LOOP_RES,
	PK_AB_PUT_MUSIC_LIST,

	PK_AB_RAISE,
	PK_BA_RAISE_HEIGHT,
	PK_AB_GET_HEIGHT,

	PK_AB_GET_DEV_CONFIG,
	PK_BA_DEV_CONFIG,
	PK_AB_EDIT_DEV_NAME,
	PK_BA_EDIT_DEV_NAME_RES,
	PK_AB_GET_WORK_STATUS,
	PK_BA_WORK_STATUS,
	PK_AB_GET_MED_CFG,
	PK_BA_MED_CFG,
	PK_AB_EDIT_MED_CFG,
	PK_BA_EDIT_MED_CFG_RES,
	PK_AB_GET_SLEEP_CFG,
	PK_BA_SLEEP_CFG,
	PK_AB_EDIT_SLEEP_CFG,
	PK_BA_EDIT_SLEEP_CFG_RES,
	PK_AB_SET_TIME_CMD,
	PK_BA_TIME_STATUS,
	PK_AB_MOVIE_CTL_CMD,
	PK_BA_MOVIE_STATUS,
	PK_AB_LED_CTL_CMD,
	PK_BA_LED_STATUS,
	PK_AB_WIFI_CTL_CMD,
	PK_BA_WIFI_STATUS,
	PK_AB_CHECK_UPDATE,
	PK_BA_FIRMWARE_UPDATE_STATUS,
	PK_BA_FIRMWARE_NEW_VER,
	PK_AB_RESET_CONFIG,
	PK_AB_SYNC_TIME,
	PK_AB_TOWER_ON,
	PK_AB_TOWER_OFF,
	
       PK_AB_GET_UID,
	PK_BA_UID,
	PK_AB_BIND_UID,
	PK_BA_BIND_UID_RES,
	
	PK_BA_EEG_PARAM,
	PK_BA_BLINK,
	PK_BA_RAW_EEG,
	PK_BA_MOVE_DATA,

	PK_AT_GET_MOVIE_LIST,
	PK_TA_MOVIE_LIST,
	PK_AB_MOVIE_SELECT,
	PK_AB_MOVIE_CTRL,
	PK_AB_MOVIE_MOVIE_STATUS,

	PK_AP_BEIYONG,
	PK_BA_BEIYONG,

	PK_xx,
	
};

static char *ingenicplayer_cmd[] ={
	[PK_AB_ENTER_MED_MODE] = "1",//"0x01",
	[PK_AB_START_MED] = "2",//"0x02",
	[PK_AB_STOP_MED] = "3",//"0x03",
    [PK_AB_GET_MED_DATA]="4",
    [PK_AB_GET_MED_DATA_RES]="4",
	[PK_AB_ENTER_SLEEP_MODE] = "32",//"0x20",
	[PK_AB_EXIT_SLEEP_MODE] = "33",//0x21",
	[PK_AB_PAUSE_ALARM] = "34",//"0x22",
	[PK_AB_STOP_ALARM] = "35",//"0x23",
	[PK_AB_GET_ALARM_LIST] = "36",//"0x24",
	[PK_BA_ALARM_LIST] = "37",//"0x25",
	[PK_AB_ADD_ALARM] = "38",//"0x26",
	[PK_BA_ADD_ALARM_RES] = "39",//"0x27",
	[PK_AB_DEL_ALARM] = "40",//"0x28",
	[PK_BA_DEL_ALARM_RES] = "41",//"0x29",
	[PK_AB_EDIT_ALARM] = "42",//"0x2A",
	[PK_BA_EDIT_ALARM_RES] = "43",//"0x2B",
	[PK_AB_GET_LED_EFFECT_LIST] = "50",//"0x32",
	[PK_BA_LED_EFFECT_LIST] = "51",//"0x33",
	[PK_AB_ENTER_FREE_MODE] = "64",//"0x40",
	[PK_AB_MUSIC_CTL_CMD] = "65",//"0x41",
	[PK_BA_MUSIC_STATUS] = "66",//"0x42",
	[PK_AB_SET_MUSIC_VOLUME] = "67",//"0x43",
	[PK_BA_MUSIC_VOLUME] = "68",//"0x44",
	[PK_AB_PLAY_MUSIC] = "69",//"0x45",
	[PK_BA_PLAY_MUSIC_RES] = "70",//"0x46",
	[PK_AB_GET_MUSIC_LIST] = "71",//"0x47",
	[PK_BA_MUSIC_LIST] = "72",//"0x48",
	[PK_AB_GET_MUSIC_FILE_NUM] = "73",//"0x49",
	[PK_BA_MUSIC_FILE_NUM] = "74",//"0x4A",
	[PK_AB_DEL_MUSIC] = "75",//"0x4B",
	[PK_BA_DEL_MUSIC_RES] = "76",//"0x4C",
	[PK_AB_EDIT_MUSIC] = "77",//"0x4D",
	[PK_BA_EDIT_MUSIC_RES] = "78",//"0x4E",
	[PK_AB_MOVE_MUSIC] = "79",//"0x4F",
	[PK_BA_MOVE_MUSIC_RES] = "80",//"0x50",
	[PK_AB_UPLOAD_MUSIC] = "81",//"0x51",
	[PK_BA_UPLOAD_MUSIC_STATUS] = "82",//"0x52",
	[PK_AB_MUSIC_LOOP] = "83",//"0x53",
	[PK_BA_MUSIC_LOOP_RES] = "84",//"0x54",
	[PK_AB_PUT_MUSIC_LIST] = "85", //"0x55"
	[PK_AB_RAISE] = "96",//"0x60",
	[PK_BA_RAISE_HEIGHT] = "97",//"0x61",
    [PK_AB_GET_HEIGHT] = "98",
	[PK_AB_GET_DEV_CONFIG] = "112",//"0x70",
	[PK_BA_DEV_CONFIG] = "113",//"0x71",
	[PK_AB_EDIT_DEV_NAME] = "114",//"0x72",
	[PK_BA_EDIT_DEV_NAME_RES] = "115",//"0x73",
	[PK_AB_GET_WORK_STATUS] = "116",//"0x74",
	[PK_BA_WORK_STATUS] = "117",//"0x75",
	[PK_AB_GET_MED_CFG] = "118",//"0x76",
	[PK_BA_MED_CFG] = "119",//"0x77",
	[PK_AB_EDIT_MED_CFG] = "120",//"0x78",
	[PK_BA_EDIT_MED_CFG_RES] = "121",//"0x79",
	[PK_AB_GET_SLEEP_CFG] = "122",//"0x7A",
	[PK_BA_SLEEP_CFG] = "123",//"0x7B",
	[PK_AB_EDIT_SLEEP_CFG] = "124",//"0x7C",
	[PK_BA_EDIT_SLEEP_CFG_RES] = "125",//"0x7D",
	[PK_AB_SET_TIME_CMD] = "126",//"0x7E",
	[PK_BA_TIME_STATUS] = "127",//"0x7F",
	[PK_AB_MOVIE_CTL_CMD] = "128",//"0x80",
	[PK_BA_MOVIE_STATUS] = "129",//"0x81",
	[PK_AB_LED_CTL_CMD] = "130",//"0x82",
	[PK_BA_LED_STATUS] = "131",//"0x83",
	[PK_AB_WIFI_CTL_CMD] = "132",//"0x84",
	[PK_BA_WIFI_STATUS] = "133",//"0x85",
	[PK_AB_CHECK_UPDATE] = "134",//"0x86",
	[PK_BA_FIRMWARE_UPDATE_STATUS] = "135",//"0x87",
	[PK_BA_FIRMWARE_NEW_VER] = "136",//"0x88",
	[PK_AB_RESET_CONFIG] ="137",//"0x89",
	[PK_AB_SYNC_TIME] = "138",//"0x8A",
	[PK_AB_TOWER_ON] = "139",//"0x8B",
	[PK_AB_TOWER_OFF] = "140",//"0x8C",
	[PK_AB_GET_UID] = "141",
	[PK_BA_UID] = "142",
	[PK_AB_BIND_UID] = "143",
	[PK_BA_BIND_UID_RES] = "144", 
	[PK_BA_EEG_PARAM] = "160",//"0xA0",
	[PK_BA_BLINK] = "161",//"0xA1",
	[PK_BA_RAW_EEG] = "162",//"0xA2",
	[PK_BA_MOVE_DATA] = "163",//"0xA3",
	[PK_AT_GET_MOVIE_LIST] = "176",//"0xB0",
	[PK_TA_MOVIE_LIST] = "177",//"0xB1",
	[PK_AB_MOVIE_SELECT] = "178",//"0xB2",
	[PK_AB_MOVIE_CTRL] = "179",//"0xB3",
	[PK_AB_MOVIE_MOVIE_STATUS] = "180",//"0xB4",
    [PK_AP_BEIYONG] = "216",   	//0xd8
    [PK_BA_BEIYONG] = "217",	//0xd9
    [PK_xx] = "255",
};
	
struct cmd_info_c {
	struct appserver_cli_info *owner;
	char *command;
	char *data;
};



char *new_mozart_music_list = NULL;

char macaddr[] = "255.255.255.255";
static List app_cmd_list;
static bool die_out = false;
static pthread_t ingenicplayer_thread;
static pthread_mutex_t cmd_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;
static pthread_t heart_rate_thread;



/* TODO: snd_source need lock */
#if 0
static int mozart_module_ingenicplayer_start(void)
{
	if (snd_source != SND_SRC_INGENICPLAYER) {
		if (mozart_module_stop())
			return -1;
		if (mozart_musicplayer_start(mozart_musicplayer_handler))
			return -1;
		snd_source = SND_SRC_INGENICPLAYER;
	} else if (!mozart_musicplayer_is_active(mozart_musicplayer_handler)) {
		if (mozart_musicplayer_start(mozart_musicplayer_handler))
			return -1;
	} else {
		//mozart_musicplayer_musiclist_clean(mozart_musicplayer_handler);
	}

	return 0;
}
#endif

//功能：将回复给app的数据打包成json格式的字符串
static char *reply_json_compose( json_object *object, char *scope)
{
	time_t timep;
  	char date[40]={0};
  	char *reply_json = NULL;

  	json_object *reply_object = json_object_new_object();
	if (!reply_object) {
		printf("%s %d : %s\n", __func__, __LINE__, strerror(errno));
	}
	time(&timep);
//	strncpy(date,asctime(localtime(&timep)),strlen(asctime(localtime(&timep)))-1);
	/********************************************************************************************/
	strncpy(date,asctime(localtime(&timep)),strlen(asctime(localtime(&timep)))+1);

	/********************************************************************************************/
	json_object_object_add(reply_object, "time", json_object_new_string(date));	
 	json_object_object_add(reply_object, "scope", json_object_new_string(scope));
 	json_object_object_add(reply_object, "data",object);	
  	reply_json = strdup(json_object_get_string(reply_object));
	/********************************修改于2018.6.29号****************************************************/	
#if 0
	if(object)
	{
		json_object_put(object);	
		object = NULL;
	}
#endif
	/*****************************************************************************************************/
	if(reply_object)
	{
  		json_object_put(reply_object);	
		reply_object = NULL;
	}
	return reply_json ;
}

static void free_cmd_list(void *cmd_list_info)
{
	struct cmd_info_c *cmd_info = (struct cmd_info_c *)cmd_list_info;
	if (cmd_info) {
		if (cmd_info->owner) {
			free(cmd_info->owner);
			cmd_info->owner = NULL;
		}
		if (cmd_info->command) {
			free(cmd_info->command);
			cmd_info->command = NULL;
		}
		if (cmd_info->data) {
			free(cmd_info->data);
			cmd_info->data = NULL;
		}
		free(cmd_info);
	}
}

//功能：把cmd_json指向的字符串中object关键字下的信息返回出来。返回出来的是字符串
static int get_object_int(char *object, char *cmd_json)
{
	int ret = -1;
	json_object *cmd_object = NULL;
	json_object *tmp = NULL;

	cmd_object = json_tokener_parse(cmd_json);
	if (cmd_object) {
		if (json_object_object_get_ex(cmd_object, object, &tmp))
			ret = json_object_get_int(tmp);
	}
	/*************************修改于2018.6.25********************************************/
	if(cmd_object != NULL)
	{
		json_object_put(cmd_object);
		cmd_object = NULL;
	}
	/***********************************************************************************/
	//json_object_put(cmd_object);
	return ret;
}

//功能：把cmd_json指向的字符串中object关键字下的信息返回出来。返回出来的是字符串
static char *get_object_string(char *object, char *content, char *cmd_json)
{
	char *content_object = NULL;
	json_object *cmd_object = NULL;
	json_object *tmp = NULL;

	cmd_object = json_tokener_parse(cmd_json);
	if (cmd_object) {
		if (json_object_object_get_ex(cmd_object, object, &tmp)) {
			if (content)
				json_object_object_get_ex(tmp, content, &tmp);
			content_object = strdup(json_object_get_string(tmp));
		}
	}
	/*************************修改于2018.6.25********************************************/
	if(cmd_object != NULL)
	{
		json_object_put(cmd_object);
		cmd_object = NULL;
	}
	/***********************************************************************************/
	//json_object_put(cmd_object);
	return content_object;
}

static char *sync_devicename(char *newname)
{
	int namelen = 0;
	char *devicename = NULL;
	FILE *fd = NULL;

	if ((fd = fopen(DEVICENAME, "a+")) == NULL) {
		pr_err("fopen failed %s\n", strerror(errno));
		return NULL;
	}
	if (newname) {
		truncate(DEVICENAME, 0);
		fwrite(newname, sizeof(char), strlen(newname), fd);
	} else {
		fseek(fd, 0, SEEK_END);
		namelen = ftell(fd);
		fseek(fd, 0, SEEK_SET);
		if (namelen) {
			devicename = (char *)calloc(namelen, sizeof(char) + 1);
			if (devicename) {
				fread(devicename, sizeof(char), namelen, fd);
				pr_debug("get devicename %s\n", devicename);
			}
		}
	}
	fclose(fd);

	return devicename;
}
/*
static int ingenicplayer_play_pause(void)
{
	return mozart_musicplayer_play_pause(mozart_musicplayer_handler);
}
*/
//把歌曲的信息打包成json格式
static void get_song_json_object(json_object *object, struct music_info *info)
{
	json_object *reply_object = object;

	if (object == NULL || info == NULL)
		return ;

	//if (info->id)
	//	json_object_object_add(reply_object, "id",
	//			json_object_new_int(info->id));
	if (info->music_name)
		json_object_object_add(reply_object, "name",
			json_object_new_string(info->music_name));
	else
		json_object_object_add(reply_object, "name",
			json_object_new_string("NULL"));
	if (info->music_url)
		json_object_object_add(reply_object, "songurl",
			json_object_new_string(info->music_url));
	else
		json_object_object_add(reply_object, "songurl",
			json_object_new_string("NULL"));
	//if (info->music_picture)
	//	json_object_object_add(reply_object, "picture",
	//			json_object_new_string(info->music_picture));
	if (info->albums_name)
		json_object_object_add(reply_object, "title",
			json_object_new_string(info->albums_name));
	else
		json_object_object_add(reply_object, "title",
		json_object_new_string("NULL"));
	if (info->artists_name)
		json_object_object_add(reply_object, "siger",
				json_object_new_string(info->artists_name));
	else
		json_object_object_add(reply_object, "siger",
				json_object_new_string("NULL"));
	//if (info->data)
	//	json_object_object_add(reply_object, "data",
	//			json_object_new_string(info->data));
}

static void get_mode_status(json_object *object)
{
	json_object *reply_object = object;

	if (mozart_mozart_mode == FREE)
		json_object_object_add(reply_object, "mode", json_object_new_string("1"));
	else if (mozart_mozart_mode == MED)
		json_object_object_add(reply_object, "mode", json_object_new_string("2"));
	else if (mozart_mozart_mode == SLEEP)
		json_object_object_add(reply_object, "mode", json_object_new_string("3"));
	else if (mozart_mozart_mode == CONFIG_WIFI)
		json_object_object_add(reply_object, "mode", json_object_new_string("4"));
	else if (mozart_mozart_mode == CONFIG_BT)
		json_object_object_add(reply_object, "mode", json_object_new_string("5"));
	else if (mozart_mozart_mode == MUSIC_BT)
		json_object_object_add(reply_object, "mode", json_object_new_string("6"));
	else if (mozart_mozart_mode == MUSIC_DLNA)
		json_object_object_add(reply_object, "mode", json_object_new_string("7"));
	else if (mozart_mozart_mode == MUSIC_AIRPLAY)
		json_object_object_add(reply_object, "mode", json_object_new_string("8"));
}

static void get_player_status(json_object *object)
{
	json_object *reply_object = object;
	player_status_t status = mozart_musicplayer_get_status(mozart_musicplayer_handler);

	if (status == PLAYER_PLAYING){
	//	printf("this is status == PLAYER_PLAYING:%d\n",status);
		json_object_object_add(reply_object, "status", json_object_new_string("1"));
	}
	//else if (status == PLAYER_PAUSED || status == PLAYER_TRANSPORT)
	else if (status == PLAYER_PAUSED){
//		printf("this is status == PLAYER_PAUSED%d\n",status);
		json_object_object_add(reply_object, "status", json_object_new_string("3"));
	}
	else if (status == PLAYER_STOPPED){
	//	printf("this is status == PLAYER_STOPPED%d\n",status);
		json_object_object_add(reply_object, "status", json_object_new_string("2"));
	}
}

//(9)当前音乐列表播放设置："loop"，整数类型编号，1-单曲播放、2-单曲循环、3-顺序播放、4-列表循环、5-随机播放）
int mozart_mozart_musicplayer_musiclist_get_play_mode()
{
	enum play_mode loop = -1;
	loop = mozart_musicplayer_musiclist_get_play_mode(mozart_musicplayer_handler);
	if(loop == play_mode_order){
		loop = 4;
	}
	else if(loop == play_mode_single){
		loop = 2;
	}
	else if(loop == play_mode_random){
		loop = 5;
	}
	return loop;
}
//功能：计算助眠剩余时间
static int sleep_time_rest()
{
	int time_rest = 0;
	if(mozart_mozart_mode == SLEEP)
	{
		if(sleep_info.sleeping)
		{
			if(sleep_info.med_time <= mozart_duration)
			{
				time_rest = mozart_duration - sleep_info.med_time; 
			}
			else{
				time_rest = 0;
			}
		}
	}
	return time_rest;
}

/*
*获取歌曲的信息，将歌曲的信息打包成json格式的字符串，包括：播放状态，播放时间，播放进度，歌曲信息，【冥想数据】
*返回值：回复给app的json格式的字符串
*/
static char *ingenicplayer_get_song_info(void)
{
	int time_rest = 0;
	int index = -1;
	int time = -1;
	int duration = 0;	
	enum play_mode mode = -1;
	char *reply_json = NULL;
	json_object *reply_object;
	struct music_info *info = NULL;
//	struct info *x_info=NULL;
    int volume = mozart_musicplayer_get_volume(mozart_musicplayer_handler);

	reply_object = json_object_new_object();
	if (!reply_object)
		return NULL;
	
	get_player_status(reply_object); //此时replay_object指针指向的内容【开始】有当前音乐播放的状态；

	
	mode = mozart_mozart_musicplayer_musiclist_get_play_mode();//获取当前播放的模式
	index = mozart_musicplayer_musiclist_get_current_index(mozart_musicplayer_handler);//获取当前音乐在音乐列表中的位置序号
	info = mozart_musicplayer_musiclist_get_current_info(mozart_musicplayer_handler);//获取当前音乐的信息
//	x_info = get_meddata_form_ble();//获得冥想数据

	if (info) 
	{
		get_song_json_object(reply_object, info);  //此时replay_object指针指向的内容有音乐的信息
		time = mozart_musicplayer_get_duration(mozart_musicplayer_handler);	
		json_object_object_add(reply_object, "time", json_object_new_int(time));
		duration = mozart_musicplayer_get_pos(mozart_musicplayer_handler);
		json_object_object_add(reply_object, "cursor", json_object_new_int(duration));
	}

    json_object_object_add(reply_object, "volume", json_object_new_int(volume));
	json_object_object_add(reply_object, "loop", json_object_new_int(mode));
	json_object_object_add(reply_object, "index", json_object_new_int(index));
	if(mozart_mozart_mode == SLEEP)		//逻辑：在助眠模式下返回剩余助眠时间，不在助眠模式下返回助眠剩余时间为0
	{
		time_rest = sleep_time_rest();
		json_object_object_add(reply_object, "sleepRestTime", json_object_new_int(time_rest));
	}else{
		json_object_object_add(reply_object, "sleepRestTime", json_object_new_int(0));
	}
#if 0
	if(mozart_mozart_mode == MED)
	{
    	json_object_object_add(reply_object, "a", json_object_new_int((int)(x_info->X_brain_attention)));
        json_object_object_add(reply_object, "m", json_object_new_int((int)(x_info->X_brain_concentration)));
        json_object_object_add(reply_object, "p", json_object_new_int((int)(x_info->X_brain_quality)));
        json_object_object_add(reply_object, "delta", json_object_new_int((int)(x_info->brain_parameter.delta[0])*256*256+(int)(x_info->brain_parameter.delta[1])*256+(int)(x_info->brain_parameter.delta[2])));
        json_object_object_add(reply_object, "theta", json_object_new_int((int)(x_info->brain_parameter.theta[0])*256*256+(int)(x_info->brain_parameter.theta[1])*256+(int)(x_info->brain_parameter.theta[2])));
        json_object_object_add(reply_object, "lowalpha", json_object_new_int((int)(x_info->brain_parameter.low_alpha[0])*256*256+(int)(x_info->brain_parameter.low_alpha[1])*256+(int)(x_info->brain_parameter.low_alpha[2])));
        json_object_object_add(reply_object, "hightalpha", json_object_new_int((int)(x_info->brain_parameter.high_alpha[0])*256*256+(int)(x_info->brain_parameter.high_alpha[1])*256+(int)(x_info->brain_parameter.high_alpha[2])));
		json_object_object_add(reply_object, "lowbeta", json_object_new_int((int)(x_info->brain_parameter.low_beta[0])*256*256+(int)(x_info->brain_parameter.low_beta[1])*256+(int)(x_info->brain_parameter.low_beta[2])));
		json_object_object_add(reply_object, "hightbeta", json_object_new_int((int)(x_info->brain_parameter.high_beta[0])*256*256+(int)(x_info->brain_parameter.high_beta[1])*256+(int)(x_info->brain_parameter.high_beta[2])));
		json_object_object_add(reply_object, "lowgamma", json_object_new_int((int)(x_info->brain_parameter.low_gamma[0])*256*256+(int)(x_info->brain_parameter.low_gamma[1])*256+(int)(x_info->brain_parameter.low_gamma[2])));
		json_object_object_add(reply_object, "highgamma", json_object_new_int((int)(x_info->brain_parameter.high_gamma[0])*256*256+(int)(x_info->brain_parameter.high_gamma[1])*256+(int)(x_info->brain_parameter.high_gamma[2])));
	}
#endif

	reply_json = reply_json_compose(reply_object,"66");
	json_object_put(reply_object);

	return reply_json;
}
#if 0
/*
*	获取当前音乐播放的位置，
*	返回值是能回复给app的json格式的字符串，
*/
static char *ingenicplayer_get_position(void)
{
	int i = 0;
	int pos = 0;
	int duration = 0;
	char *reply_json = NULL;
	json_object *reply_object = json_object_new_object();
	if (!reply_object)
		return NULL;
//延时一定的时间来获取。
	for (i = 0; i < 30; i++) {
		pos = mozart_musicplayer_get_pos(mozart_musicplayer_handler);
		if (pos != -1 && pos != 0)
			break;
		usleep(100000);
	}
	duration = mozart_musicplayer_get_duration(mozart_musicplayer_handler);

	json_object_object_add(reply_object, "get_current_position",
			json_object_new_int(pos));
	json_object_object_add(reply_object, "get_duration",
			json_object_new_int(duration));
	reply_json = strdup(json_object_get_string(reply_object));
	json_object_put(reply_object);

	return reply_json;
}


static void ingenicplayer_set_seek(char *cmd_json)
{
	int ret = -1;
	char *reply_json = NULL;

	if ((ret = get_object_int("set_seek_position", cmd_json)) >= 0)
		mozart_musicplayer_set_seek(mozart_musicplayer_handler, ret);
	pr_debug("seek %d\n", ret);

	reply_json = ingenicplayer_get_position();
	if (reply_json) {
		mozart_appserver_notify("get_position", reply_json);
		free(reply_json);
	}

	return;
}

static void play_music_by_name(char *name)
{
int length=0; 
 int i;
 struct music_info *info = NULL;
   length = mozart_musicplayer_musiclist_get_length(mozart_musicplayer_handler);

	   for (i = 0; i < length; i++) {
		
		info = mozart_musicplayer_musiclist_get_index_info(mozart_musicplayer_handler, i);
		if (info != NULL&&name != NULL) 
			{
                       if(strcmp(info->music_name,name)==0)
                 	  {
                       mozart_musicplayer_play_index(mozart_musicplayer_handler,i);
			 break;
                 	 }
		}
	}
	   if(i==length)
	   	mozart_musicplayer_play_index(mozart_musicplayer_handler,0);
}*/
#endif

//根据音乐的路径来播放音乐
static void play_music_by_url(char *url)
{
	int length=0; 
 	int i;
 	struct music_info *info = NULL;
   	length = mozart_musicplayer_musiclist_get_length(mozart_musicplayer_handler);
	for (i = 0; i < length; i++) {
		info = mozart_musicplayer_musiclist_get_index_info(mozart_musicplayer_handler, i);
		if (info != NULL&&url != NULL) 
		{
        	if(strcmp(info->music_url,url)==0)
            {
            	mozart_musicplayer_play_index(mozart_musicplayer_handler,i);
			  	break;
            }
		}
	}
	if(i==length)
	mozart_musicplayer_play_index(mozart_musicplayer_handler,0);
}
//(0x02)开始冥想训练	
static void start_med(char *data)
{
	char *music = NULL;
    music =  get_object_string( "music", NULL,data);
   	play_music_by_url(music);
   //printf("\n %s \n",name);
   	free(music);
   //access(info->music_url,F_OK)==0	   		
}

static char *get_med_data(void)
{   
	int i;
  	struct info *info;
  	char bsn_buf[20] = {};
  	char bver_buf[20] = {};
  	char uid_buf[20] = {};
  	json_object *reply_object = json_object_new_object();
  	json_object *data_object= json_object_new_array();
  	char *reply_json = NULL;
	info = get_meddata_form_ble();
    if(!mozart_ini_getkey("/usr/data/system.ini", "base", "sn", bsn_buf))	
    json_object_object_add(reply_object, "bsn", json_object_new_string(bsn_buf));//BASE序列号
  	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "ver", bver_buf))
    json_object_object_add(reply_object, "bver", json_object_new_string(bver_buf));
   	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "uid", uid_buf))		//BASE固件版本号
    json_object_object_add(reply_object, "uid", json_object_new_string(uid_buf));//绑定的用户ID号
   	json_object_object_add(reply_object, "xsn", json_object_new_string((char *)info->X_number));//X序列号
 	json_object_object_add(reply_object, "xver", json_object_new_string((char *)info->X_binnumber));//X的固件版本号
 	json_object_object_add(reply_object, "tsn", json_object_new_string((char *)info->Tower_number));//TOWER的序列号
 	json_object_object_add(reply_object, "tver", json_object_new_string((char *)info->Tower_binnumber));//TOWER的固件版本号
 	json_object_object_add(reply_object, "tpver", json_object_new_string(""));//TOWER APP版本号
 	json_object_object_add(reply_object, "fname", json_object_new_string(""));//TOWER APP版本号
 	json_object_object_add(reply_object, "ttype", json_object_new_int(1));//训练类型：1-冥想训练
 	json_object_object_add(reply_object, "stime", json_object_new_string(med_info.medstarttime));//开始时间
 	json_object_object_add(reply_object, "etime", json_object_new_string(med_info.medendtime));//结束时间
 	json_object_object_add(reply_object, "mid", json_object_new_int(med_info.mid));//EGO编号的音乐ID
 	json_object_object_add(reply_object, "mid3rd", json_object_new_int(med_info.mid3rd));//第三方平台（喜马拉雅）的音乐ID
 	json_object_object_add(reply_object, "cid", json_object_new_int(med_info.cid));//音乐所属专辑ID，第三方音乐是第三方专辑ID，EGO音乐是EGO课程ID
 	json_object_object_add(reply_object, "mname", json_object_new_string(med_info.mname));//冥想时使用的音乐名称
 	json_object_object_add(reply_object, "mfrom", json_object_new_int(med_info.mfrom));//冥想时使用的音乐名称
 	json_object_object_add(reply_object, "dur", json_object_new_int(med_info.med_time));//数据的时长："dur"，单位为秒，也是data数组的长度
	for(i=0;i<=med_info.med_time;i++)	//以列表形式存储每一秒的专注度、放松度、prrosignal数据
	{   
		json_object *med_object = json_object_new_object();
		json_object_object_add(med_object, "a", json_object_new_int(med_info.zhuanzhu[i]));	
		json_object_object_add(med_object, "m", json_object_new_int(med_info.fangsong[i]));	
		json_object_object_add(med_object, "p", json_object_new_int(0));	
		json_object_array_add(data_object, med_object);
	}
	json_object_object_add(reply_object, "aindex", json_object_new_int(100));//专注度均值??
	json_object_object_add(reply_object, "mindex", json_object_new_int(100));//放松度均值??
	json_object_object_add(reply_object, "score", json_object_new_int(100));//本次训练综合评分??
	json_object_object_add(reply_object, "rank", json_object_new_int(1));//训练结果排名位置??
	json_object_object_add(reply_object, "data",data_object);
    reply_json = reply_json_compose(reply_object,"5");
	printf("\nMED info:%s\n",reply_json);
	
	/*******************************2018.6.26号************************************************/
	if(reply_object != NULL)
	{
		json_object_put(reply_object);
		reply_object = NULL;
	}
	if (data_object != NULL)
	{
		json_object_put(data_object);
		data_object = NULL;
	}

	/****************************************************************************************/
//	json_object_put(reply_object);
	return reply_json;
}

//功能：获取当前的闹钟列表
static char *get_alarm_list(void)
{
	char *alarm_list = NULL;
  	char *reply_json = NULL;
  	int len = 0;
  	int x;
	json_object *object = json_object_new_array();
  	json_object *tmp = NULL;
  	json_object *reply= json_object_new_array();
  	json_object *reply_object = json_object_new_object();
  	alarm_list = mozart_alarm_get_list();
	printf("alarm_list :%s\n",alarm_list);
	if(alarm_list != NULL){
  		object=json_tokener_parse(alarm_list);
		len = json_object_array_length(object);
	//printf("len : %d\n",len);
	}
	if(len > 0){
    	for(x=0;x<len;x++)
   		{
    		tmp = json_object_array_get_idx(object, x);
        	if (json_object_object_get_ex(tmp, "alarm_private_info", &tmp))
			if (json_object_object_get_ex(tmp, "private_info", &tmp))
			{
				json_object_array_add(reply,tmp);
			}
   		}
	}
  	json_object_object_add(reply_object, "rows",reply);
    reply_json = reply_json_compose(reply_object,"37");
	
	/*******************************修改于2018.6.25**************************************************************/

	if (reply_object != NULL)
	{
		json_object_put(reply_object);
		reply_object = NULL;
	}
	if(reply != NULL)
	{
		json_object_put(reply);
		reply = NULL;
	}
	if (object != NULL)
	{
		json_object_put(object);
		object = NULL;
	}
	/****************************************************************************************************/
  	//json_object_put(reply_object);
  	//json_object_put(reply);
  	//json_object_put(object);
 	printf("cmd_json:%s\n",reply_json);
    return reply_json;
}

//检测app发来的时间格式是否正确
static char *add_alarm_check(char *data)
{
//	char *alarm_list = NULL;
   	json_object *object = NULL;
   	json_object *tmp = NULL;
   	int hour=24;
   	int minute=60;
// 	int len = 0;
   	json_object *reply_object = json_object_new_object();
   	char *reply_json = NULL;
#if 0	
    alarm_list = mozart_alarm_get_list();
	if(alarm_list != NULL)
	{
		object=json_tokener_parse(alarm_list);
		len = json_object_array_length(object);
	}
	else
	{
		len = 0;
	}
	    
    if(len>=4) 
    {
		json_object_object_add(reply_object, "reason", json_object_new_string("the maximum number of alarm clocks has been reached"));
		reply_json = reply_json_compose(reply_object,"39"); 
	    json_object_put(reply_object);     	
	   	return reply_json;
    }
	//printf("data:%s\n",data);
#endif
    hour=get_object_int( "hour", data);
    minute=get_object_int( "minute", data);
    if(hour>=24||hour<0)
    {
    	json_object_object_add(reply_object, "reason", json_object_new_string("alarm clock time format error"));
	    reply_json = reply_json_compose(reply_object,"39"); 
		/********************************修改于2018.6.29号****************************************************/	
		if(reply_object)
		{
			json_object_put(reply_object);	
			reply_object = NULL;
		}

		/*****************************************************************************************************/
	    //json_object_put(reply_object);
	   	return reply_json;
    }
    if(minute>=60||minute<0)
	{
    	json_object_object_add(reply_object, "reason", json_object_new_string("alarm clock time format error"));
	    reply_json = reply_json_compose(reply_object,"39"); 
		/********************************修改于2018.6.29号****************************************************/	
		if(reply_object)
		{
			json_object_put(reply_object);	
			reply_object = NULL;
		}

		/*****************************************************************************************************/
	    //json_object_put(reply_object);     	
	   	return reply_json;
    }
    if (json_object_object_get_ex(object, "mn", &tmp))
		return NULL;
    return NULL;
}

static void add_alarm(char *data)
{
	int len = 0;
 	struct alarm_info *alarm = NULL;
 	struct module_alarm_info *malarm = NULL;
	len = strlen(data);
  	malarm = mozart_module_alarm_getinfo(data);
 	alarm = calloc(sizeof(struct alarm_info) + len + 1, 1);
  	strncpy(alarm->prv_info.info,data, len);
  	alarm->prv_info.len = len;
 	alarm->alarm_id = 0 ;
 	alarm->hour = malarm->hour;
 	alarm->minute = malarm->minute;
 	alarm->week_active = malarm->date;
 	alarm->weekly_repeat = (int)(malarm->repeat);
 	alarm->enable = (int)(malarm->enable);
 	alarm->timestamp = 0;
 	strncpy(alarm->name,malarm->name, strlen( malarm->name));
/*
	pr_debug(" id: %d, hour: %d, minute: %d, timestamp: %ld , week_active: %d , repeat : %d , enable : %d , info: %s\n",
	alarm->alarm_id, alarm->hour, alarm->minute, alarm->timestamp, alarm->week_active,alarm->weekly_repeat,
	alarm->enable,alarm->prv_info.info);
*/
	if(0 == mozart_alarm_add(alarm)){
		printf("add alarm success\n");
	}else{
		printf("add alarm failed\n");
	}
	free(malarm);
	malarm = NULL;
}
static int get_alarmid(int id)
{
	int x;
    int length=0;
    char *alarm_list = NULL;
    json_object *tmp = NULL;
    json_object *object = json_object_new_array();
    json_object *alarm_object=NULL;

    alarm_list=mozart_alarm_get_list();
    object=json_tokener_parse(alarm_list);
    length = json_object_array_length(object);

	for(x=0;x<length;x++)
   	{
    	alarm_object = json_object_array_get_idx(object, x);
        if (json_object_object_get_ex(alarm_object, "alarm_private_info", &tmp))
		if (json_object_object_get_ex(tmp, "private_info", &tmp))
		if (json_object_object_get_ex(tmp, "id", &tmp))
		{										
			if(id==json_object_get_int(tmp))
			{
            	if (json_object_object_get_ex(alarm_object, "alarm_id", &tmp))
				return json_object_get_int(tmp);		 	
			}
		}       
	}
	/*******************************修改于2018.6.25**************************************************************/
	if (alarm_list != NULL)
	{
		free(alarm_list);
		alarm_list = NULL;
	}
	if (object != NULL)
	{
		json_object_put(object);
		object = NULL;
	}
	/**************************************************************************************************************/
	return -1;
}

static char *del_alarm(char *data)
{
	int id = -1;
   	char *alarm_list = NULL;
   	int length=0;
   	int len=0;
   	int x;
   	char *reply_json = NULL;
   	struct alarm_info *alarm = NULL;
  	json_object *object = json_object_new_array();
  	json_object *tmp = NULL;
  	json_object *alarm_object=NULL;
  	json_object *reply_object = json_object_new_object();
    id =  get_object_int( "id", data);
    id	=  get_alarmid(id);
    alarm_list=mozart_alarm_get_list();
    object=json_tokener_parse(alarm_list);
    length = json_object_array_length(object);
       
   	for(x=0;x<length;x++)
   	{
    	alarm_object = json_object_array_get_idx(object, x);
        if (json_object_object_get_ex(alarm_object, "alarm_id", &tmp))
        if( json_object_get_int(tmp)==id)
        {
        	if (json_object_object_get_ex(alarm_object, "alarm_private_info", &tmp))
		    if (json_object_object_get_ex(tmp, "private_info", &tmp))
				len = strlen(json_object_get_string(tmp));
			alarm = calloc(sizeof(struct alarm_info) + len + 1, 1);

			if (tmp)
		    	strncpy(alarm->prv_info.info, json_object_get_string(tmp), len);
	        alarm->prv_info.len = len;

			if (json_object_object_get_ex(alarm_object, "hour", &tmp))
				alarm->hour = json_object_get_int(tmp);
			if (json_object_object_get_ex(alarm_object, "minute", &tmp))
		        alarm->minute = json_object_get_int(tmp);
	        if (json_object_object_get_ex(alarm_object, "week_active", &tmp))
		        alarm->week_active = json_object_get_int(tmp);
	        if (json_object_object_get_ex(alarm_object, "weekly_repeat", &tmp))
		        alarm->weekly_repeat = json_object_get_int(tmp);
	        if (json_object_object_get_ex(alarm_object, "enable", &tmp))
		        alarm->enable = json_object_get_int(tmp);
	        if (json_object_object_get_ex(alarm_object, "alarm_id", &tmp))
		        alarm->alarm_id = json_object_get_int(tmp);    
	        if (json_object_object_get_ex(alarm_object, "name", &tmp))
		        strncpy(alarm->name, json_object_get_string(tmp),strlen( json_object_get_string(tmp)));

			alarm->timestamp = 0;
                break;
        }
   	}
   
    json_object_put(object);
    if(x==length)        
    {
    	json_object_object_add(reply_object, "reason", json_object_new_string("The alarm clock has been deleted"));
	    reply_json = reply_json_compose(reply_object,"41"); 
		/********************************修改于2018.6.29号****************************************************/	
		if(reply_object)
		{
			json_object_put(reply_object);	
			reply_object = NULL;
		}

		/*****************************************************************************************************/
	    //json_object_put(reply_object);
	   	return reply_json;
    }
    else
    {
    	mozart_alarm_delete(alarm);
	  	json_object_put(reply_object);
        return NULL;
    }
}

static char *edit_alarm(char *data)
{
	int len = 0;
 	int x;
 	char *alarm_list = NULL;
 	char *reply_json = NULL;
 	struct alarm_info *alarm = NULL;
 	struct module_alarm_info *malarm = NULL;
  	json_object *object = json_object_new_array();
  	json_object *tmp = NULL;
  	json_object *alarm_object=NULL;
  	json_object *reply_object = json_object_new_object();
	len = strlen(data);
 
	malarm = mozart_module_alarm_getinfo(data);
 	alarm = calloc(sizeof(struct alarm_info) + len + 1, 1);
 
	strncpy(alarm->prv_info.info,data, len);
  	alarm->prv_info.len = len;
 	alarm->alarm_id = get_alarmid(malarm->id);   //闹钟编号
 	alarm->hour = malarm->hour;       //小时
 	alarm->minute = malarm->minute;   //分钟 
 	alarm->week_active = malarm->date;   //每周那天响应
 	alarm->weekly_repeat = (int)(malarm->repeat);    //每周那天重复
 	alarm->enable = (int)(malarm->enable);     //闹钟是否管用
 	printf("alarm->enable:%d\n",alarm->enable);
 	alarm->timestamp = 0;
 	strncpy(alarm->name,malarm->name, strlen(malarm->name));
    if(alarm->hour<24&&alarm->hour>=0&&alarm->minute<60&&alarm->minute>=0)
	{
		printf("【%s】 [%s] %d\n",__FILE__, __func__, __LINE__);
	//	mozart_alarm_delete(alarm);
	//	mozart_alarm_add(alarm);
		mozart_alarm_update(alarm);  //更新闹钟列表
    }
    alarm_list = mozart_alarm_get_list();
    object=json_tokener_parse(alarm_list);
    len = json_object_array_length(object);
    for(x=0;x<len;x++)
    {
    	alarm_object = json_object_array_get_idx(object, x);
		if (json_object_object_get_ex(alarm_object, "alarm_id", &tmp))
        if( json_object_get_int(tmp)==alarm->alarm_id)
        {
        //   printf("\n%d  %d\n",json_object_get_int(tmp),alarm->alarm_id);
        	if (json_object_object_get_ex(alarm_object, "alarm_private_info", &tmp))
		    if (json_object_object_get_ex(tmp, "private_info", &tmp))
			{
		    	reply_object=json_object_get(tmp);
			}
        }
   	}

	if(alarm->hour>=24)
    {
    	json_object_object_add(reply_object, "res", json_object_new_int(0));
        json_object_object_add(reply_object, "reason", json_object_new_string("alarm clock time format error"));
    }
	else if(alarm->minute>=60)
	{
    	json_object_object_add(reply_object, "res", json_object_new_int(0));
		json_object_object_add(reply_object, "reason", json_object_new_string("alarm clock time format error"));
    }
	else 
	{
		json_object_object_add(reply_object, "res", json_object_new_int(1));
   	}
		   
  	reply_json = reply_json_compose(reply_object,"43");
	/********************************修改于2018.6.29号****************************************************/	
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}

	/*****************************************************************************************************/
  	//json_object_put(reply_object);
  	return reply_json;
 
}

static char *ingenicplayer_light_list(int i)
{
	int x;
  	char *reply_json = NULL;
  	json_object *reply= json_object_new_array();
  	json_object *object[i];
  	json_object *reply_object = json_object_new_object();
    for(x=1;x<=i;x++)
  	{
  		object[x-1] = json_object_new_object();
        json_object_object_add(object[x-1], "id", json_object_new_int(x));
        json_object_object_add(object[x-1], "name", json_object_new_string(light_list[x]));
        json_object_array_add(reply, object[x-1]);
	}
  
  	json_object_object_add(reply_object, "list", reply);	
  	reply_json = reply_json_compose(reply_object,"51");
  	for(x=0;x<i;x++)
  		json_object_put(object[x]);
	/********************************修改于2018.6.29号****************************************************/	
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}

	/*****************************************************************************************************/
  	//json_object_put(reply_object);
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply)
	{
		json_object_put(reply);	
		reply = NULL;
	}
	
	/*****************************************************************************************************/

	//json_object_put(reply);
    return reply_json;
 }

static char *ingenicplayer_gif_list(int i)
{
	int x;
  	char *reply_json = NULL;
  	json_object *reply= json_object_new_array();
  	json_object *object[i];
  	json_object *reply_object = json_object_new_object();
    for(x=1;x<=i;x++)
  	{
  		object[x-1] = json_object_new_object();
        json_object_object_add(object[x-1], "id", json_object_new_int(x));
        json_object_object_add(object[x-1], "name", json_object_new_string(gif_list[x]));
        json_object_array_add(reply, object[x-1]);
	}
  
  	json_object_object_add(reply_object, "list", reply);	
  	reply_json = reply_json_compose(reply_object,"177");
  	for(x=0;x<i;x++)
  	json_object_put(object[x]); 
	
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	
	/*****************************************************************************************************/
	//json_object_put(reply_object);
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply)
	{
		json_object_put(reply); 
		reply = NULL;
	}
		
	/*****************************************************************************************************/
  	//json_object_put(reply); 
  	return reply_json;
 }
//音乐播放控制指令，ctrl是整型，1：播放；2：暂停；3：上一首；4：下一首
static int music_ctl_cmd(char *data)
{
	int ret =-1;
   	int ctrl=0;
   	time_t timep;
    ctrl = get_object_int("ctrl",data);
    switch(ctrl){
    	case 1:
        	ret=mozart_musicplayer_play_pause_play(mozart_musicplayer_handler);
         	break;    
     	case 2:
        	ret=mozart_musicplayer_play_pause_pause(mozart_musicplayer_handler);
	  		break;	
     	case 3:
        	ret=mozart_musicplayer_prev_music(mozart_musicplayer_handler);
		 	if(mozart_mozart_mode == MED)
			{ 
				if(med_info.meding)
			 	{
			 		med_info.meding = false;
                    time(&timep);
		            sprintf(med_info.medendtime, "%d-%d-%d %d:%d:%d",localtime(&timep)->tm_year+1900,localtime(&timep)->tm_mon+1,
	 	            localtime(&timep)->tm_mday,localtime(&timep)->tm_hour,
	 	            localtime(&timep)->tm_min,localtime(&timep)->tm_sec);
				    med_file_close();	
			 	}
				med_info.med_time=0;  
		        med_info.meding = true; 
			}
	  		break;
     	case 4:
        	ret=mozart_musicplayer_next_music(mozart_musicplayer_handler);
		 	if(mozart_mozart_mode == MED)	//为什么这里要考虑MED的
			{ 
				if(med_info.meding)
			 	{
			 		med_info.meding = false;
                    time(&timep);
		            sprintf(med_info.medendtime, "%d-%d-%d %d:%d:%d",localtime(&timep)->tm_year+1900,localtime(&timep)->tm_mon+1,
	 	            localtime(&timep)->tm_mday,localtime(&timep)->tm_hour,
	 	            localtime(&timep)->tm_min,localtime(&timep)->tm_sec);//记录一下冥想结束的时间
				    med_file_close();	
			 	}
				med_info.med_time=0;  
		        med_info.meding = true; 
			}
	  		break;
     	default:	  
        	ret=-1;
   	}
   return ret;
}

//设置当前的音量
static void set_music_volume(char *data)
{
	int volume=0;
    volume =  get_object_int( "volume", data);
  	mozart_musicplayer_set_volume(mozart_musicplayer_handler, volume);
}

//功能：根据路径来播放音乐
static char *play_music(char *data)
{
	int index=0;
   	char *reason= NULL;
   	char *music = NULL;  	//music用来记录音乐文件的全路径名称
   	json_object *reply_object = json_object_new_object();
   	struct music_info *info = NULL;
   	char *reply_json = NULL;
    index = get_object_int( "index", data);
	music =  get_object_string( "music", NULL,data);
	info = mozart_musicplayer_musiclist_get_index_info(mozart_musicplayer_handler,index);
    if(music!=NULL && strcmp(music,info->music_url)==0) 
    {
    	mozart_musicplayer_play_index(mozart_musicplayer_handler,index);//播放音乐列表位于第index处的文件
    }
	else
	{
		reason=malloc(200);
        sprintf(reason, "The music %s was not found",music);	
	    json_object_object_add(reply_object, "reason", json_object_new_string(reason));
	    reply_json = reply_json_compose(reply_object,"70");
		free(reason);	
	}
	
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/
	
	//json_object_put(reply_object);	
    return reply_json ;
}

/*功能：APP发送（0x47）获取4个文件夹下的音乐列表；
** 		base回复APP（0x48），具体见JSON报文格式；
**		注：内存释放有问题！！！
*/
static char *get_music_list(char *data)
{
	int i,j;
    int len=0;
	int lengh=0; 
	int idex=0;
	int music_url_lengh=0;
	//音乐所在的音乐列表，1:free,2:med,3:sleep,4:alarm.
    int list=0;   
	int total=0;  
	int music_num=0;
	char *dir = "";   
  	char *music = NULL;
	char *url = NULL;
	char *url_cp = NULL;
	const char *music_name = NULL;
	const char *music_url = NULL;	
    json_object *object, *tmp,*song;
	json_object *song_object = NULL;
	json_object *song_new=NULL;
	json_object *reply_object= json_object_new_object();
	json_object *reply= json_object_new_array();
	char *reply_json = NULL;
    object = json_tokener_parse(data);	//将字符串转化成json格式
    if (json_object_object_get_ex(object, "list", &tmp))
	    list = json_object_get_int(tmp);
    if (json_object_object_get_ex(object, "dir", &tmp))
    {
    	len = strlen(json_object_get_string(tmp));
		dir =  calloc(len,1);	     //内存没释放吧？？？？
        strncpy(dir,json_object_get_string(tmp),len);
    }
    json_object_object_add(reply_object, "list", json_object_new_int(list)); 
	json_object_object_add(reply_object, "dir", json_object_new_string(dir));
    if(list == 1)
    {
    	url = malloc(strlen("/mnt/sdcard/free/") + strlen(dir) + 1);
        sprintf(url, "/mnt/sdcard/free/%s",dir);//此时url为free模式下的音乐列表的全路径	  
    }
	else if(list == 2)
	{
    	url = malloc(strlen("/mnt/sdcard/med/") + strlen(dir) + 1);
        sprintf(url, "/mnt/sdcard/med/%s",dir);
	}
	else if(list == 3)
	{
    	url = malloc(strlen("/mnt/sdcard/sleep/") + strlen(dir) + 1);
        sprintf(url, "/mnt/sdcard/sleep/%s",dir);
	}
	else if(list == 4)
	{
    	url = malloc(strlen("/mnt/sdcard/alarm/") + strlen(dir) + 1);
        sprintf(url, "/mnt/sdcard/alarm/%s",dir);
	}
	lengh = strlen(url);  
    url_cp =  calloc(lengh,1);
	//music = mozart_localplayer_get_musiclist(); //注：music是json string format格式（获取的是SD卡内所有的音乐列表）
	if (new_mozart_music_list == NULL)
	{
		return NULL;
	}
	music = calloc(strlen(new_mozart_music_list)+1,1);
	memset(music, 0, strlen(new_mozart_music_list)+1);
	strcpy(music,new_mozart_music_list);

	if (music == NULL) 
	{
		printf("localplayer's music list is empty\n");
	}
	else
	{
    	song_object = json_tokener_parse(music);
        free(music);
	    music_num = json_object_array_length(song_object); //数组的长度就是音乐的数量
		for (i = 0; i < music_num; i++) 
		{
			song = json_object_array_get_idx(song_object, i);
			if (json_object_object_get_ex(song, "name", &tmp))
			music_name = json_object_get_string(tmp);  //第i位置处的音乐的名字
			if (json_object_object_get_ex(song, "url", &tmp))	 
            {
            	music_url = json_object_get_string(tmp);  //第i位置处的音乐的全部路径
				music_url_lengh = strlen(music_url);
				strncpy(url_cp,music_url,lengh);				 
                //printf("\n%s\n",music_url);
				//printf("\n%s\n",url_cp);
            }
			if((list==1)&&(*(music_url+12)=='f'))
		    	idex++; 
			else if((list==2)&&(*(music_url+12)=='m'))
		        idex++;
            else if((list==3)&&(*(music_url+12)=='s'))
		        idex++;
            else if((list==4)&&(*(music_url+12)=='a'))
		        idex++;
			if(strcmp(url,url_cp)==0) 	//保证在指定的文件夹下(添加进去)
			{
            	for(j=lengh+1;j<=music_url_lengh;j++)
                {
                	//printf(" %c ",*(music_url+j));
					if(*(music_url+j)=='/')	
                    	break;
                }
				if(j>=music_url_lengh+1)
				{
					total++;
                    song_new= json_object_new_object();
					//printf("\n%d  %s\n",i,music_name);
					json_object_object_add(song_new, "idex", json_object_new_int(idex-1));	
					json_object_object_add(song_new, "name", json_object_new_string(music_name));
					json_object_object_add(song_new, "url", json_object_new_string(music_url));
					json_object_object_add(song_new, "time", json_object_new_int(0));
					//printf("\n %s\n", strdup(json_object_get_string(song_new)));
                    json_object_array_add(reply,song_new);
				}
  		   	} 
		}
	}

	json_object_object_add(reply_object, "list", json_object_new_int(list));
	json_object_object_add(reply_object, "total", json_object_new_int(total));	//满足条件的总的数据数量。	
	json_object_object_add(reply_object, "pageid", json_object_new_int(0));
	json_object_object_add(reply_object, "pagesize", json_object_new_int(0));
	json_object_object_add(reply_object, "rows",reply);
	reply_json = reply_json_compose(reply_object,"72");	
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/
	//json_object_put(reply_object);
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply)
	{
		json_object_put(reply);	
		reply = NULL;
	}
	/*****************************************************************************************************/
	//json_object_put(reply);
	if(song_new != NULL)
    	json_object_put(song_new);
	/*******************************修改于2018.6.25**************************************************************/
	if (object != NULL)
	{
		json_object_put(object);
		object = NULL;
	}
	if (song_object != NULL)
	{
		json_object_put(song_object);
		song_object = NULL;
	}
	
	/************************************************************************************************************/

	return reply_json;
}

/*功能：获取指定音乐文件夹下子音乐文件夹的数量
**是否包含子文件中的文件数量："sub"，布尔类型，1 表示包含，0 表示不包含。
*/
static char *get_music_file_num(char *data)
{
	int i,j;
    int len=0;
	int lengh=0; 
	int music_url_lengh=0;
    int list=0;
	int count=0;   
	int music_num=0;
	char *dir = NULL;   
	bool sub =false;
  	char *music = NULL;
	char *url = NULL;
	char *url_cp = NULL;
	const char *music_url = NULL;	
    json_object *object, *tmp,*song;
	json_object *song_object = NULL;
	json_object *reply_object;
	char *reply_json = NULL;
    object = json_tokener_parse(data) ;
	if (json_object_object_get_ex(object, "list", &tmp))
    	list = json_object_get_int(tmp);
    if (json_object_object_get_ex(object, "dir", &tmp))
    {
    	len = strlen(json_object_get_string(tmp));
		dir =  calloc(len,1);	  
        strncpy(dir,json_object_get_string(tmp),len);
    }
    if (json_object_object_get_ex(object, "sub", &tmp))
    sub =   json_object_get_boolean(tmp);
   	reply_object = json_object_new_object();
    json_object_object_add(reply_object, "list", json_object_new_int(list));
	json_object_object_add(reply_object, "dir", json_object_new_string(dir));
	json_object_object_add(reply_object, "sub", json_object_new_boolean(sub));
    if(list == 1)
    {
    	url = malloc(strlen("/mnt/sdcard/free/") + strlen(dir) + 1);
        sprintf(url, "/mnt/sdcard/free/%s",dir);		  
    }
	else if(list == 2)
	{
    	url = malloc(strlen("/mnt/sdcard/med/") + strlen(dir) + 1);
        sprintf(url, "/mnt/sdcard/med/%s",dir);
	}
	else if(list == 3)
	{
    	url = malloc(strlen("/mnt/sdcard/sleep/") + strlen(dir) + 1);
        sprintf(url, "/mnt/sdcard/sleep/%s",dir);
	}
	else if(list == 4)
	{
    	url = malloc(strlen("/mnt/sdcard/alarm/") + strlen(dir) + 1);
        sprintf(url, "/mnt/sdcard/alarm/%s",dir);
	}
	lengh = strlen(url);  
   	url_cp =  calloc(lengh,1);
   	//printf("url==%s\n",url);
//	music = mozart_localplayer_get_musiclist();
	if (new_mozart_music_list == NULL)
	{
		return NULL;
	}

	music = calloc(strlen(new_mozart_music_list)+1,1);
	memset(music, 0, strlen(new_mozart_music_list)+1);
	strcpy(music,new_mozart_music_list);
	
	if (music == NULL) 
	{
		printf("localplayer's music list is empty\n");
    }
	else
	{
	    song_object = json_tokener_parse(music);
        free(music);
	    music_num = json_object_array_length(song_object);
		for (i = 0; i < music_num; i++) 
		{
			song = json_object_array_get_idx(song_object, i);
            if (json_object_object_get_ex(song, "url", &tmp))	 
            {
            	music_url = json_object_get_string(tmp);
				music_url_lengh = strlen(music_url);
				strncpy(url_cp,music_url,lengh);				 
                //printf("\n%s\n",music_url);
				//printf("\n%s\n",url_cp);
            }
			if(strcmp(url,url_cp)==0)
			{
            	if(sub==0)
                {
                	for(j=lengh+1;j<=music_url_lengh;j++)
                    {
                    	//         	printf(" %c ",*(music_url+j));
						if(*(music_url+j)=='/')	
                        break;
                    }
				    if(j>=music_url_lengh+1)
						count++;
                }
               	else
                	count++;
			} 
		}
	}
	json_object_object_add(reply_object, "count", json_object_new_int(count));	
	reply_json = reply_json_compose(reply_object,"74");
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/
	//json_object_put(reply_object);
	/*******************************修改于2018.6.25**************************************************************/
	if (object != NULL)
	{
		json_object_put(object);
		object = NULL;
	}
	if (song_object != NULL)
	{
		json_object_put(song_object);
		song_object = NULL;
	}
	
	/************************************************************************************************************/
	return reply_json;
}


//设置音乐的播放模式
static char* music_loop(char *data)
{
	enum play_mode loop;
	char *reply_json = NULL;
	json_object *reply_object = json_object_new_object();  //对json初始化
	loop=  get_object_int("loop", data);
	if(loop == 2)   //单曲循环
	{
		loop = 1;	
	}else if(loop == 4){  //列表循环
		loop = 0;
	}else if(loop == 5){   //随机播放
		loop = 2;
	}
    pr_debug("loop==%d\n",loop);
	mozart_play_mode[mozart_mozart_mode] = loop;    //这是什么意思？？？
	mozart_musicplayer_musiclist_set_play_mode(mozart_musicplayer_handler, loop);

	loop = mozart_mozart_musicplayer_musiclist_get_play_mode();  //获取当前的播放模式。返回值2,4,5
	json_object_object_add(reply_object, "loop", json_object_new_int(loop));
	reply_json = reply_json_compose(reply_object, "84");
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/
	//json_object_put(reply_object);
	
	return reply_json;
}

/*
**功能：APP发送过来最新的播放列表，base做更新替换。（0x55）
*/
static char *put_music_list(char *data)
{
	int i;
	int inlist = -1;
	int len =0;
	int index;
	const char *name = NULL;
	const char *songurl = NULL;
	const char *title = NULL;
	const char *siger = NULL;
	json_object *object=NULL;
	json_object *songs=NULL;
	json_object *tmp=NULL;
	json_object *song=NULL;
  	struct music_info *info = NULL;
	int loop = -1;	//添加于2018.7.9号用于测试
	
	loop = mozart_musicplayer_musiclist_get_play_mode(mozart_musicplayer_handler);	//添加于2018.7.9号用于测试
	printf("\n  loop:%d \n",loop);

	index = mozart_musicplayer_musiclist_get_current_index(mozart_musicplayer_handler);
	object=json_tokener_parse(data);

	if (json_object_object_get_ex(object, "inlist", &tmp))
   	inlist = json_object_get_int(tmp);  //1 表示当前正在播放的音乐在播放列表里，0 表示不在列表里
	
	if (json_object_object_get_ex(object, "songs", &tmp))   //songs：是播放列表数组
    songs = json_tokener_parse(json_object_get_string(tmp));

	if(songs != NULL){
	  	mozart_musicplayer_musiclist_clean(mozart_musicplayer_handler);
	  	len= json_object_array_length(songs); 
		printf("...................len:%d...............................",len);
	  	for (i = 0; i < len; i++) {
			song = json_object_array_get_idx(songs, i);

			if (json_object_object_get_ex(song, "name", &tmp))
				name = json_object_get_string(tmp);
			if (json_object_object_get_ex(song, "songurl", &tmp))
				songurl= json_object_get_string(tmp);
			if (json_object_object_get_ex(song, "title", &tmp))
				title = json_object_get_string(tmp);				
			if (json_object_object_get_ex(song, "siger", &tmp))
				siger = json_object_get_string(tmp);
			
			info = mozart_musiclist_get_info(0, (char *)name, (char *)songurl, NULL,
							(char *)title, (char *)siger, NULL);
			
			/**************************2018.7.7号修改*************************************************/
			#if 0
			if(strcmp(title,"NULL"))
			{
				info = mozart_musiclist_get_info(0, (char *)name, (char *)songurl, NULL,
								(char *)title, (char *)siger, NULL);
				printf("........%d...........title:%s,char:%s\n",__LINE__,info->albums_name,(char *)title);
			}
			else
			{
				info = mozart_musiclist_get_info(0, (char *)name, (char *)songurl, NULL,
								NULL, (char *)siger, NULL);	
				printf("........%d...........title:%s,char:%s\n",__LINE__,info->albums_name,(char *)title);
			}
			info = mozart_musiclist_get_info(0, (char *)name, (char *)songurl, NULL,
								NULL, (char *)siger, NULL);	
			/**********************************************************************************/
			
		//	printf("........%d...........title:%s,name:%s,songurl:%s,siger:%s\n",__LINE__,(char *)title, (char *)name,(char *)songurl,(char *)siger);
			info = mozart_musiclist_get_info(0, (char *)name, (char *)songurl, NULL,
								NULL, NULL, NULL);	
			#endif
			if (info)
				mozart_musicplayer_musiclist_insert(mozart_musicplayer_handler, info);
		}
		
		mozart_musicplayer_musiclist_set_play_mode(mozart_musicplayer_handler, loop);	//添加于2018.7.9号
	}
	
 	if(len==0)
 	{
    	mozart_module_stop();  //关闭mozart_musicplayer_handler
		mozart_musicplayer_start(mozart_musicplayer_handler);
 	}
 	else if(inlist==0 && (PLAYER_PLAYING == mozart_musicplayer_get_status(mozart_musicplayer_handler)))
 	{
 		if(len==index)
	 		mozart_musicplayer_play_index(mozart_musicplayer_handler,index-1);
		else
	    	mozart_musicplayer_play_index(mozart_musicplayer_handler,index);
 	}
	loop = mozart_mozart_musicplayer_musiclist_get_play_mode();	//添加于2018.7.9号用于测试
	printf("\n  loop:%d \n",loop);
	//json_object_put(object);
	/*******************************修改于2018.6.25**************************************************************/
	if (object != NULL)
	{
		json_object_put(object);
		object = NULL;
	}
	if (songs != NULL)
	{
		json_object_put(songs);
		songs = NULL;
	}
	/************************************************************************************************************/
    return NULL;
}

//接受APP发送的当前磁悬浮球上升的高度的指令
static void raise(char *data)
{
	int high;
    high=  get_object_int("height", data);
	high=high/20;
	// lifting_appliance_led(high);
    if(high!=0&&high<4)
	high+=2;
	/*注：lifting_appliance_go_high是lifting里面的一个全局变量，在这个文件中改变这个数的值，
			要学会这种方法,顾虑：是不是代码不好维护？
*/
	lifting_appliance_go_high=high;
}

/*
**功能：设置当前升降高度（0x62）并返回当前的高度信息（0x61）
*/
static char *get_height(int i)
{
	int x;
    int y;	 
    json_object *reply_object = json_object_new_object();
    char *reply_json = NULL;
    x=lifting_appliance_high;
	if(x==1||x==2)
		x=0;
	if(x!=0)
    	x-=2;
    y=lifting_appliance_go_high;
	if(y==1||y==2)
		y=0;
	if(y!=0)
    	y-=2;	 
	json_object_object_add(reply_object, "height", json_object_new_int(x)); 
    json_object_object_add(reply_object, "toheight", json_object_new_int(y));  	  
    json_object_object_add(reply_object, "event", json_object_new_int(i)); 
    json_object_object_add(reply_object, "tower", json_object_new_int(1)); 
	reply_json = reply_json_compose(reply_object,"97");
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/
	//json_object_put(reply_object);
    return reply_json;
}
//获得base设备的名字
static int get_device_name(json_object *object)
{
	int tmp = 0;
	int length = 0;
	char *name = NULL;
	FILE *fp = NULL;
	json_object *newobject = object;

	if ((fp = fopen(DEVICE_NAME_FILENAME, "r")) == NULL) {
		printf("fopen %s\n", strerror(errno));
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	if ((length = ftell(fp)) == -1) {
		printf("ftell %s\n", strerror(errno));
		fclose(fp);
		return -1;
	}
	fseek(fp, 0, SEEK_SET);

	if ((name = calloc(length + 1, sizeof(char))) != NULL) {
		tmp = fread(name, sizeof(char), length, fp);
		if (tmp == length) {
				json_object_object_add(newobject, "name",
						json_object_new_string(name));		
		}
		free(name);
	}
	fclose(fp);
	return 0;
}
/*
*功能：(0x71)硬件配置信息。包含 X、Base 和 Tower 的硬件配置信息。
*/
static char *get_dev_config(void)
{
	char sn_buf[20] = {};
	char blever_buf[20] = {};
	char ver_buf[20] = {};
	char os_buf[20] = {};
	char ip_buf[20] = {};
	char pin_buf[20] = {};
	char ssid_buf[20] = {};
	char zone_buf[8] = {};
	char tformat_buf[8] = {};
	struct info *info;
	struct statfs diskInfo; 
	statfs("/mnt/sdcard/", &diskInfo); 
	unsigned long long blocksize = diskInfo.f_bsize;    
	unsigned long long totalsize = blocksize * diskInfo.f_blocks;   
	unsigned long long freeDisk = diskInfo.f_bfree * blocksize; 
	json_object *x_object= json_object_new_object();
	json_object *base_object= json_object_new_object();
	json_object *tower_object= json_object_new_object();
	json_object *reply_object= json_object_new_object();
	char *reply_json = NULL;
	info = get_meddata_form_ble();   //从蓝牙那块获得冥想的数据（头戴（x）的信息）存放到info
	
	/******************************X 头带*************************************************/
	json_object_object_add(x_object, "type", json_object_new_int(1));//设备类型
 	json_object_object_add(x_object, "status", json_object_new_int(get_cuband_state()));//设备与Base之间BLE的连接状态
	//注：缺少name，问初试状态下x的名字           									
 	json_object_object_add(x_object, "model", json_object_new_string((char *)info->X_model));//设备型号
 	json_object_object_add(x_object, "sn", json_object_new_string((char *)info->X_number));//产品序列号
	//注：缺少电池状态，"charge"，布尔类型，1-正在充电，0-未充电
	json_object_object_add(x_object, "charge", json_object_new_int((int)info->X_charge));	//电池状态(新添加)
	json_object_object_add(x_object, "battery", json_object_new_int((int)info->X_power));//电池状态
 	json_object_object_add(x_object, "ble", json_object_new_string((char *)info->X_BLEnumber));//BLE设备号
 	json_object_object_add(x_object, "ver", json_object_new_string((char *)info->X_binnumber));//固件版本号
 	json_object_object_add(x_object, "update", json_object_new_string(""));   //新添加
	//注：缺少X固件新版本号："vernew"，字符串，已下载的新固件版本，没有新版本则为空，不可以修改
	
	/******************************Base*************************************************/
	json_object_object_add(base_object, "type", json_object_new_int(2));//设备类型
	get_device_name(base_object);	//设备名称
 	json_object_object_add(base_object, "model", json_object_new_string("EGO-B1"));//设备型号
  	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "sn", sn_buf))	
  		json_object_object_add(base_object, "sn", json_object_new_string(sn_buf));//产品序列号
  	json_object_object_add(base_object, "charge", json_object_new_int(0));		//电池状态
  	json_object_object_add(base_object, "battery", json_object_new_int(capacity));//电池状态
 	json_object_object_add(base_object, "ble", json_object_new_string(macaddr));//BLE设备号
  	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "blever", blever_buf))
  		json_object_object_add(base_object, "blever", json_object_new_string(blever_buf));//BLE固件版本号
  	json_object_object_add(base_object, "blenew", json_object_new_string(""));	//BLE固件新版本号
   	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "ver", ver_buf))
  		json_object_object_add(base_object, "ver", json_object_new_string(ver_buf));//固件版本号
   	if(new_ver.ver!=NULL)
  		json_object_object_add(base_object, "vernew", json_object_new_string(new_ver.ver));//固件新版本号
  	else
  		json_object_object_add(base_object, "vernew", json_object_new_string(""));
   	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "os", os_buf))
  		json_object_object_add(base_object, "os", json_object_new_string(os_buf));//操作系统版本号
  	json_object_object_add(base_object, "osnew", json_object_new_string(""));	//操作系统新版本号
  	json_object_object_add(base_object, "wifi", json_object_new_int(wifi_status));//WIFI连接状态
   	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "ip", ip_buf))
   		json_object_object_add(base_object, "ip", json_object_new_string(ip_buf));//WLAN IP地址
  	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "ssid", ssid_buf))
  		json_object_object_add(base_object, "ssid", json_object_new_string(ssid_buf));//当前连接的WIFI SSID
  	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "passwd", pin_buf))
  		json_object_object_add(base_object, "pin", json_object_new_string(pin_buf));//当前连接的WIFI密码
  	json_object_object_add(base_object, "mac", json_object_new_string(macaddr));//MAC地址
  	if(!mozart_ini_getkey("/usr/data/system.ini", "time", "zone", zone_buf))
  		json_object_object_add(base_object, "zone", json_object_new_string(zone_buf));	//时间格式设置
  	if(!mozart_ini_getkey("/usr/data/system.ini", "time", "tformat", tformat_buf))	
  		json_object_object_add(base_object, "tformat", json_object_new_int(atoi(tformat_buf)));//时间格式设置
  	json_object_object_add(base_object, "sd", json_object_new_int((int)(totalsize>>30)+1));//SD卡容量
  	json_object_object_add(base_object, "free", json_object_new_int((int)(freeDisk>>20)));//SD卡剩余空间
	
	/******************************Tower*************************************************/
	
	json_object_object_add(tower_object, "type", json_object_new_int(3));//设备类型
	json_object_object_add(tower_object, "status", json_object_new_int(get_tower_state()));//设备与Base之间BLE的连接状态
	//注：缺少设备名称，"name"，字符串，设备的别名，用户可以修改自定义
	json_object_object_add(tower_object, "model", json_object_new_string((char *)info->Tower_type));	//设备型号
 	json_object_object_add(tower_object, "sn", json_object_new_string((char *)info->Tower_number));		//产品序列号
	//注：缺少电池状态，"charge"，布尔类型，1-正在充电，0-未充电
	json_object_object_add(tower_object, "charge", json_object_new_int((int)info->Tower_if_power));		//tower是否在充电
	json_object_object_add(tower_object, "battery", json_object_new_int((int)info->Tower_power));		//电池电量
 	json_object_object_add(tower_object, "ble", json_object_new_string((char *)info->Tower_BLEnumber));//BLE设备号
 	//注：缺少BLE固件版本号:"blever",,操作系统版本号："os",MAC地址："mac"，wlan的ip地址：ip；
	//注：添加缺少的（4.26号）
	json_object_object_add(tower_object, "blever", json_object_new_string((char *)info->Tower_blever));
	json_object_object_add(tower_object, "os", json_object_new_string((char *)info->Tower_os));
	json_object_object_add(tower_object, "mac", json_object_new_string((char *)info->Tower_MAC));
	json_object_object_add(tower_object, "ip", json_object_new_string((char *)info->Tower_wlanIp));
	printf("info->Tower_binnumber:%s...........\n",(char *)info->Tower_binnumber);
	json_object_object_add(tower_object, "ver", json_object_new_string((char *)g_Tower_binnumber));//固件版本号(修改于2018.7.31号)
	json_object_object_add(tower_object, "blenew", json_object_new_string(""));	//BLE固件新版本号："blenew"
	//json_object_object_add(tower_object, "ver", json_object_new_string((char *)info->Tower_binnumber));//固件版本号
	json_object_object_add(tower_object, "vernew", json_object_new_string(""));   //固件新版本号:"vernew"
	json_object_object_add(tower_object, "osnew", json_object_new_string(""));	  //操作系统新版本号："osnew"
 	json_object_object_add(tower_object, "wifi", json_object_new_int((int)info->Tower_WIFI_state[0]));//WIFI连接状态
 	json_object_object_add(tower_object, "ssid", json_object_new_string((char *)info->Tower_WIFI_Name));//设置的连接的WIFI SSID
 	json_object_object_add(tower_object, "pin", json_object_new_string((char *)info->Tower_WIFI_Password));//设置的连接的WIFI密码
 	json_object_object_add(tower_object, "sd", json_object_new_int((int)info->Tower_sd_capacity));//SD卡容量
 	json_object_object_add(tower_object, "free", json_object_new_int((int)info->Tower_sd_rest));//SD卡容量

 	json_object_object_add(reply_object, "x",x_object);
 	json_object_object_add(reply_object, "base",base_object);
 	json_object_object_add(reply_object, "tower",tower_object);
    reply_json = reply_json_compose(reply_object,"113");   //base回复给APP（0x71）
    /********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/
	//json_object_put(reply_object);
	return reply_json;

}

/*修改设备名称，设备类型："type"，1-X、2-Base、3-Tower（0x72）,只修改了base的名称
**返回值是修改的结果，这个是要传给APP的(0x73)。
*/
static char *edit_dev_name(char *data)
{
	int type=-1;
  	char *ble = NULL;
  	char *name = NULL;
  	FILE *fp = NULL;
  	char *reply_json = NULL;
  	json_object *reply_object = json_object_new_object();
    type= get_object_int( "type", data);
 	ble =  get_object_string( "ble", NULL,data); 
	name =  get_object_string( "name", NULL,data); 
 	if(type == 1)
		printf("%s",ble);
	else if(type == 2)
	{
		if ((fp = fopen("/usr/data/ingenicplayer/devicename", "w+")) == NULL) {
	 		json_object_object_add(reply_object, "res", json_object_new_int(0));
			json_object_object_add(reply_object, "reason", json_object_new_string("fopen devicename errno"));   
	    }
		else
		{
        	fwrite(name,strlen(name), sizeof(char), fp);
			fclose(fp);
		    json_object_object_add(reply_object, "res", json_object_new_int(1));
			json_object_object_add(reply_object, "reason", json_object_new_string(""));  
		}
	}
	else if(type == 3)
	printf("%s",ble);
	json_object_object_add(reply_object, "type", json_object_new_int(type));
	json_object_object_add(reply_object, "ble", json_object_new_string(ble)); 
	json_object_object_add(reply_object, "name", json_object_new_string(name)); 
	reply_json = reply_json_compose(reply_object,"115");  //（0x73）
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/
	//json_object_put(reply_object);
	return reply_json;
}


/*功能：（app0x74）获取base的当前工作状态，并把当前的工作状态保存到reply_object里面，返回出来！
*		reply_object的具体内容看json报文格式的0x75指令
*/
static char *get_work_status(void)
{
	int i = 0;
	int index = -1;
	int time = -1;
	int duration = 0;
	int length = -1;
	enum play_mode loop = -1;
	char *reply_json = NULL;
	json_object *reply_object;
	json_object *songs= json_object_new_array();
    int volume = mozart_musicplayer_get_volume(mozart_musicplayer_handler);
	struct music_info *info = NULL;

	reply_object = json_object_new_object();
	if (!reply_object)
		return NULL;

    get_mode_status(reply_object);  //获取当前工作模式
	get_player_status(reply_object);//音乐当前的播放状态

	loop = mozart_mozart_musicplayer_musiclist_get_play_mode();
	//当前播放的歌曲在播放列表中的位置
	index = mozart_musicplayer_musiclist_get_current_index(mozart_musicplayer_handler);

	info = mozart_musicplayer_musiclist_get_current_info(mozart_musicplayer_handler);

	if (info) {
		/*疑问：为什么需要添加此信息*/
		get_song_json_object(reply_object, info);		
		time = mozart_musicplayer_get_duration(mozart_musicplayer_handler);	
		json_object_object_add(reply_object, "time", json_object_new_int(time));
		duration = mozart_musicplayer_get_pos(mozart_musicplayer_handler);
		json_object_object_add(reply_object, "cursor", json_object_new_int(duration));
	}

    json_object_object_add(reply_object, "volume", json_object_new_int(volume));
	json_object_object_add(reply_object, "loop", json_object_new_int(loop));
	json_object_object_add(reply_object, "index", json_object_new_int(index));
	json_object_object_add(reply_object, "height", json_object_new_int(lifting_appliance_high));//升降机构当前高度
	json_object_object_add(reply_object, "validtime", json_object_new_int(med_info.med_time));
	
	
	length = mozart_musicplayer_musiclist_get_length(mozart_musicplayer_handler);
	for (i = 0; i < length; i++) {
		json_object *songs_object = json_object_new_object();
		
		info = mozart_musicplayer_musiclist_get_index_info(mozart_musicplayer_handler, i);
		if (info != NULL) {
			get_song_json_object(songs_object, info);
			json_object_array_add(songs, songs_object);
		}
		/*需要释放songs_object：json_object_put(songs_object)*/
	}

	json_object_object_add(reply_object, "songs", songs);
    reply_json = reply_json_compose(reply_object,"117");
//	printf("*****0x75****%s\n",reply_json);	   

//	printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);
//	printf("\n*****0x75****【%s】 %s %d reply_object:%s  \n",__FILE__, __func__, __LINE__,json_object_get_string(reply_object));
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/
	//json_object_put(reply_object);

	return reply_json;
}

//功能：从初始化文件夹中获取冥想的参数配置
static char *get_med_cfg(void)
{
	char city_buf[100] = {};
  	char volume_buf[8] = {};
  	char music_buf[100] = {};
  	char name_buf[100] = {};
  	char movie_buf[100] = {};
  	json_object *reply_object = json_object_new_object();
  	char *reply_json = NULL;
	if(!mozart_ini_getkey("/usr/data/system.ini", "med", "volume", volume_buf))
    json_object_object_add(reply_object, "volume", json_object_new_int(atoi(volume_buf)));//冥想音乐播放的默认音量

  	if(!mozart_ini_getkey("/usr/data/system.ini", "med", "music", music_buf))	//默认播放的冥想音乐
    json_object_object_add(reply_object, "music", json_object_new_string(music_buf));

    if(!mozart_ini_getkey("/usr/data/system.ini", "med", "name", name_buf))		//
    json_object_object_add(reply_object, "name", json_object_new_string(name_buf));

  	if(!mozart_ini_getkey("/usr/data/system.ini", "med", "movie", movie_buf))	//缺省播放的全息动画文件："movie"，Tower动画文件全路径名称，为空则不播放动画
    json_object_object_add(reply_object, "movie", json_object_new_string(movie_buf));
  
  	if(!mozart_ini_getkey("/usr/data/system.ini", "med", "city", city_buf))		//所在城市编号
    json_object_object_add(reply_object, "city", json_object_new_string(city_buf));
    reply_json = reply_json_compose(reply_object,"119");
	   
	//json_object_put(reply_object);
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/

	return reply_json;
}

//功能：(0x78)修改冥想配置信息。
static char *edit_med_cfg(char *data)
{
	time_t timep;
  	int volume=-1;
  	char volume_buf[8]={};
  	char gvolume_buf[8]={};
  	char *music=NULL;
  	char gmusic_buf[100] = {};
  	char *movie=NULL;
  	char gname_buf[100] = {};
  	char *name=NULL;
  	char gmovie_buf[100] = {};
  	char *city=NULL;
  	char gcity_buf[100] = {};
  	json_object *reply_object = json_object_new_object();
  	char *reply_json = NULL;
	city =  get_object_string( "city", NULL,data);
	volume =  get_object_int( "volume", data);
    music =  get_object_string( "music", NULL,data);
	movie =  get_object_string( "movie", NULL,data);
	name = get_object_string("name",NULL,data);
	
	sprintf(volume_buf,"%d",volume);   
	// printf("\n %d %s %s",volume,music,movie);
	if(volume<0||volume>100)
  	{
    	json_object_object_add(reply_object, "res", json_object_new_int(0));
	 	json_object_object_add(reply_object, "reason", json_object_new_string("Wrong configuration parameter"));
  	}
  	else
  	{
  		if(!mozart_ini_getkey("/usr/data/system.ini", "med", "volume", gvolume_buf))
	    if(volume!=atoi(gvolume_buf))
	 	{
	 		if(mozart_mozart_mode==MED)
            mozart_musicplayer_set_volume(mozart_musicplayer_handler, volume);
            mozart_ini_setkey("/usr/data/system.ini", "med", "volume", volume_buf);	
	 	}
		if(music!=NULL)
	    if(!mozart_ini_getkey("/usr/data/system.ini", "med", "music", gmusic_buf))
		if(strcmp(gmusic_buf,music)!=0)  	//如果音乐路劲和上一次的不同，则更改，否则不更改。
		{
  	    	mozart_ini_setkey("/usr/data/system.ini", "med", "music", music);
		    if(mozart_mozart_mode==MED)	
		   {
                 play_music_by_url(music);
				 if(med_info.meding)		
			 	{
           		     med_info.meding = false;
                     time(&timep);
		        	 sprintf(med_info.medendtime, "%d-%d-%d %d:%d:%d",localtime(&timep)->tm_year+1900,localtime(&timep)->tm_mon+1,
	 	             localtime(&timep)->tm_mday,localtime(&timep)->tm_hour,
	 	             localtime(&timep)->tm_min,localtime(&timep)->tm_sec);
				     med_file_close();	
				}		
				med_info.med_time=0;  
		        med_info.meding = true; 
			}
		}
		if(name!=NULL)
	 	if(!mozart_ini_getkey("/usr/data/system.ini", "med", "name", gname_buf))
		if(strcmp(gname_buf,name)!=0)  	
		{
  	    	mozart_ini_setkey("/usr/data/system.ini", "med", "name", name);				
		}
		 
		if(movie!=NULL)
	 	if(!mozart_ini_getkey("/usr/data/system.ini", "med", "movie", gmovie_buf))
		if(strcmp(gmovie_buf,movie)!=0)  	
		{
        	mozart_ini_setkey("/usr/data/system.ini", "med", "movie", movie);
			Pack_write(Select_gif,(unsigned char *)movie,strlen(movie)-1); 		  
		}
		 
	 	if(city!=NULL)   //需要发送给tower
	 	if(!mozart_ini_getkey("/usr/data/system.ini", "med", "city", gcity_buf))
		if(strcmp(gcity_buf,city)!=0)  	
		{
        	mozart_ini_setkey("/usr/data/system.ini", "med", "city", city);
			getweather();   //用户改变city后会改变一下城市的信息
		}	 
		json_object_object_add(reply_object, "res", json_object_new_int(1));
	 	json_object_object_add(reply_object, "reason", json_object_new_string(""));
  	}
	if(city!=NULL)
		json_object_object_add(reply_object, "city", json_object_new_string(city));
	  	json_object_object_add(reply_object, "volume", json_object_new_int(volume));
	if(music!=NULL)
		json_object_object_add(reply_object, "music", json_object_new_string(music));
	if(name!=NULL)
			json_object_object_add(reply_object, "name", json_object_new_string(name));
	if(movie!=NULL)
		json_object_object_add(reply_object, "movie", json_object_new_string(movie));

	reply_json = reply_json_compose(reply_object,"121");
	//json_object_put(reply_object);
	
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/
	sleep(4);
	return reply_json;
}

static char *get_sleep_cfg(void)
{
	char loop_buf[8] = {};
	char volume_buf[8] = {};
  	char led_buf[8] = {};
  	char duration_buf[8] = {};
  	char alarm_buf[8] = {};
  	char name_buf[100] = {};
  	char music_buf[100] = {};
  	char movie_buf[100] = {};
  	json_object *reply_object = json_object_new_object();
  	char *reply_json = NULL;
	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "loop", loop_buf))
    	json_object_object_add(reply_object, "loop", json_object_new_int(atoi(loop_buf)));//助眠音乐列表播放设置
	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "volume", volume_buf))
    	json_object_object_add(reply_object, "volume", json_object_new_int(atoi(volume_buf)));//助眠音乐播放的默认音量
    if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "led", led_buf))
    	json_object_object_add(reply_object, "led", json_object_new_int(atoi(led_buf)));//助眠灯光的灯光亮度颜色模式编号
    if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "duration", duration_buf))	
    	json_object_object_add(reply_object, "duration", json_object_new_int(atoi(duration_buf)));//助眠音乐持续时间
	//唤醒闹钟编号："alarm"，整数类型，0表示不使用唤醒闹钟，大于0的编号则是指定的唤醒闹钟的编号
	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "alarm", alarm_buf))		
    	json_object_object_add(reply_object, "alarm", json_object_new_int(atoi(alarm_buf)));
	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "name", name_buf))
    	json_object_object_add(reply_object, "name", json_object_new_string(name_buf));
	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "music", music_buf))
    	json_object_object_add(reply_object, "music", json_object_new_string(music_buf));////缺省播放的助眠音乐,音乐路劲的全路径
	//缺省播放的全息动画文件："movie"，Tower动画文件全路径名称
	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "movie", movie_buf))
    	json_object_object_add(reply_object, "movie", json_object_new_string(movie_buf));
    reply_json = reply_json_compose(reply_object,"123");
	//json_object_put(reply_object);
		/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/
	return reply_json;
}

static char *edit_sleep_cfg(char *data)
{
	int loop=-1; 
  	int volume=-1;
  	int led=-1;
  	int duration=-1;
  	int alarm=-1;
  	char loop_buf[8] = {};
  	char gloop_buf[8] = {};
  	char volume_buf[8] = {};
  	char gvolume_buf[8] = {};
  	char led_buf[8] = {};
  	char gled_buf[8] = {};
  	char duration_buf[8] = {};
  	char alarm_buf[8] = {};
  	char *music=NULL;
  	char gmusic_buf[100] = {};
  	char *name=NULL;
  	char gname_buf[100] = {};
  	char *movie=NULL;
  	char gmovie_buf[100] = {};
  	unsigned char DATA[2];
  	json_object *reply_object = json_object_new_object();
  	char *reply_json = NULL;

	loop =  get_object_int( "loop", data);
	volume =  get_object_int( "volume", data);
	led =  get_object_int( "led", data);
	duration=  get_object_int( "duration", data);	//助眠音乐持续的时间
	alarm =  get_object_int( "alarm", data);
    music =  get_object_string( "music", NULL,data);
	name =  get_object_string( "name", NULL,data);
	movie =  get_object_string( "movie", NULL,data);
	sprintf(loop_buf,"%d",loop);
	sprintf(volume_buf,"%d",volume);
	sprintf(led_buf,"%d",led);
	sprintf(duration_buf,"%d",duration);
	sprintf(alarm_buf,"%d",alarm);
	if(loop<1||loop>5||volume<0||volume>100||led<0||duration<0||alarm<0)
  	{
    	json_object_object_add(reply_object, "res", json_object_new_int(0));
	 	json_object_object_add(reply_object, "reason", json_object_new_string("Wrong configuration parameter"));
  	}
  	else if(0)
  	{

  	}
  	else
  	{
  		if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "loop", gloop_buf))
		if(loop!=atoi(gloop_buf))
		{
        	mozart_ini_setkey("/usr/data/system.ini", "sleep", "loop",loop_buf); //把app设置的循环存到文件中去
			if(mozart_mozart_mode==SLEEP)			  
	        	mozart_musicplayer_musiclist_set_play_mode(mozart_musicplayer_handler, loop);
		}
		if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "volume", gvolume_buf))
		if(volume!=atoi(gvolume_buf))
		{
  	    	mozart_ini_setkey("/usr/data/system.ini", "sleep", "volume", volume_buf);
			if(mozart_mozart_mode==SLEEP)		
	        mozart_musicplayer_set_volume(mozart_musicplayer_handler, volume);
		}
       	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "led", gled_buf))
		if(led!=atoi(gled_buf))
		{
  	    	mozart_ini_setkey("/usr/data/system.ini", "sleep", "led", led_buf);
			if(mozart_mozart_mode==SLEEP)	
			{
	        	if(led==0)
		        {
		        	printf("给tower发送灯光指令。。。。。。。。。。\n");
                	DATA[0] = 0;
		            DATA[1] = 0;
		            Pack_write(Light_effect,DATA,0);
		        }
	            else
		        {
		        	printf("给tower发送灯光指令。。。。。。。。。。\n");
                	DATA[0] = 1;
		            DATA[1] = atoi(led_buf);
		            Pack_write(Light_effect,DATA,0);
		        }
			}
		}

		mozart_ini_setkey("/usr/data/system.ini", "sleep", "duration", duration_buf); //将助眠时长写入配置文件
		mozart_duration = duration*60;    //助眠时长换算成秒数。
	
		mozart_ini_setkey("/usr/data/system.ini", "sleep", "alarm", alarm_buf);
       	sleep_info.alarm_id=alarm;
	
	 	if(music!=NULL)
	   	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "music", gmusic_buf))
		if(strcmp(gmusic_buf,music)!=0)  	
	 	{
  	    	mozart_ini_setkey("/usr/data/system.ini", "sleep", "music", music);
		  	if(mozart_mozart_mode==SLEEP)	 
            play_music_by_url(music);
	 	}

		if(name!=NULL)
	   	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "name", gname_buf))
		if(strcmp(gname_buf,name)!=0)  	
	 	{
  	    	mozart_ini_setkey("/usr/data/system.ini", "sleep", "name", name);
	 	}
		 
	 	if(movie!=NULL)
	   	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "movie", gmovie_buf))
	   	if(strcmp(gmovie_buf,movie)!=0)  	
	 	{
  	    	mozart_ini_setkey("/usr/data/system.ini", "sleep", "movie", movie);
		 	if(mozart_mozart_mode==SLEEP)	 
		 	{
		 		printf("给tower发送动画指令。。。。。。。。。。\n");
				Pack_write(Select_gif,(unsigned char *)movie,strlen(movie));
			}
	 	}
        
  	 	json_object_object_add(reply_object, "res", json_object_new_int(1));
	 	json_object_object_add(reply_object, "reason", json_object_new_string(""));
  	}
  
 	json_object_object_add(reply_object, "loop", json_object_new_int(loop)); 
 	json_object_object_add(reply_object, "volume", json_object_new_int(volume));  
 	json_object_object_add(reply_object, "led", json_object_new_int(led));
 	json_object_object_add(reply_object, "duration", json_object_new_int(duration));
	json_object_object_add(reply_object, "alarm", json_object_new_int(alarm));
	if(music!=NULL)
	 	json_object_object_add(reply_object, "music", json_object_new_string((char *)music));
	if(name!=NULL)
	 	json_object_object_add(reply_object, "name", json_object_new_string((char *)name));
 	if(movie!=NULL)
	 	json_object_object_add(reply_object, "movie", json_object_new_string((char *)movie));
 	reply_json = reply_json_compose(reply_object,"125");
	   
	//json_object_put(reply_object);
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/

	return reply_json;
}

static char *set_time_cmd(char *data)
{
	char *zone;
   //char *tformat;	 	 
	int tformat;
	char tformat_buf[8] = {};
	
    json_object *reply_object = json_object_new_object();
    char *reply_json = NULL;

    zone =  get_object_string( "zone", NULL,data);
  //  tformat =  get_object_string( "tformat", NULL,data);

  	tformat =  get_object_int( "tformat",data);
  	printf("..........>>>>>>>>>>>>%d>>>>>>>>>>>\n",tformat);
  	sprintf(tformat_buf,"%d",tformat);	

	mozart_ini_setkey("/usr/data/system.ini", "time", "zone", zone);	


	mozart_ini_setkey("/usr/data/system.ini", "time", "tformat", tformat_buf);	

    json_object_object_add(reply_object, "zone", json_object_new_string(zone)); 
//  json_object_object_add(reply_object, "tformat", json_object_new_string(tformat)); 
	json_object_object_add(reply_object, "tformat", json_object_new_int(tformat)); 
    reply_json = reply_json_compose(reply_object,"127");
  
    //json_object_put(reply_object);
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/

    return reply_json;
}

 static int movie_ctl_cmd(char *data)
{
	int ctrl=-1;
	unsigned char DATA[1];   
	   
    ctrl = get_object_int("ctrl", data);
	if(ctrl == 1)   
    DATA[0]=0x00; 
	if(ctrl == 2)
    DATA[0]=0x01; 
	Pack_write(Control_gif,DATA,0);
	Pack_write(REQUEST_Tower_gif_state,NULL,0);

    return 0;     	
}
 
static int led_ctl_cmd(char *data)
{
	bool x= false;
  	unsigned char DATA[4];   
  	json_object *object=NULL;
  	json_object *tmp=NULL;
  	json_object *tmp2=NULL;
  	object=json_tokener_parse(data);

  	if (json_object_object_get_ex(object, "mode", &tmp))
  	{
     	if (json_object_object_get_ex(tmp, "cmd", &tmp2))
		{
			//mozart_ini_setkey("/usr/data/system.ini", "sleep", "cmd", (char *)json_object_get_string(tmp2));	
        	DATA[0] = json_object_get_int(tmp2);
     	}
		if (json_object_object_get_ex(tmp, "id", &tmp2))
        {
	        //mozart_ini_setkey("/usr/data/system.ini", "sleep", "id", (char *)json_object_get_string(tmp2));
        	DATA[1] = json_object_get_int(tmp2);   
		} 
	   	Pack_write(Light_effect,DATA,0);
  	}
  
    if (json_object_object_get_ex(object, "rgb", &tmp))
   	{
   		x = true;	
        if (json_object_object_get_ex(tmp, "r", &tmp2))
        DATA[0] = json_object_get_int(tmp2) ;
	   	if (json_object_object_get_ex(tmp, "g", &tmp2))
        DATA[1] = json_object_get_int(tmp2) ;
        if (json_object_object_get_ex(tmp, "b", &tmp2))
        DATA[2] = json_object_get_int(tmp2) ;      
   	}
   	else
   	{
    	DATA[0] = 0;
	   	DATA[1] = 0;
        DATA[2] = 0; 
   	}
	 
   	if (json_object_object_get_ex(object, "bright", &tmp))
   	{
   		x = true;
        if (json_object_object_get_ex(tmp, "level", &tmp2))
        DATA[3] = json_object_get_int(tmp2) ;		   
   	}
   	else
   	{
    	DATA[3] = 0;
   	}
   	if(x)
    	Pack_write(Light_intensity,DATA,4);
   //    Pack_write(REQUEST_Tower_lighting_state,NULL,0);  
	return 0;     	
}

static int wifi_ctl_cmd(char *data)
{
	int wificmd=-1;
	   
    wificmd = get_object_int("wificmd", data);
	if(wificmd == 0)   
          Pack_write(Tower_off,NULL,0);
	if(wificmd == 1)
          Pack_write(Tower_on,NULL,0);

	Pack_write(REQUEST_Tower_WIFI_state,NULL,0);
    return 0;     	
}

static int check_update(char *data)
{
	int type=-1;
	nvinfo_t *nvinfo = NULL;	  
    type = get_object_int("type", data);
	
	if(type == 1 || type == 0)
	{
		if (capacity >= 80)
		{
			if ((nvinfo = mozart_updater_chkver()) != NULL) {
				free(nvinfo);
				printf("\n=================update======================\n");
				mozart_system("updater -p");
			}
		}
  	}
	if(type == 2 || type == 0)
	Pack_write(Check_update,NULL,0);

	if(type == 3 || type == 0)   
          ;
    	return 0;     	
}

static int reset_config(char *data)
{
	int type=-1;
	type = get_object_int("type", data);
	if(type== 1||type == 3)   
	{
    	system("rm -r /mnt/sdcard/data");
	    system("rm -r /mnt/sdcard/data1");
	    system("rm -r /usr/data/system.ini");
	    system("rm -r /usr/data/wpa_supplicant.conf");
	    system("cp  /usr/data/system_beifen.ini  /usr/data/system.ini");
	  //  system("cp  /usr/data/wpa_supplicant_beifen.conf  /usr/data/wpa_supplicant.conf");	
        system("mkdir -p /mnt/sdcard/data");
	    system("mkdir -p /mnt/sdcard/data1");
        mozart_mozart_mode=0;
	    mozart_start_mode(FREE);	
	}
	if(type == 2||type == 3 )
    	Pack_write(Reset,NULL,0);
	 return 0;     	
}

static int sync_time(char *data)
{
	char *clock = NULL;
   	unsigned char DATA[8];
   	char *zone;
	char zone1[2];
	char zone2[2];
	int zone1_int;
	int zone2_int;
   	int format;
   	int seconds;
   	struct tm time_tm;  
   	struct timeval time_tv; 
 	//  struct timezone time_zo;
	clock =  get_object_string( "clock", NULL,data); 
	printf("\n Clock:%s \n",clock);
   	zone = get_object_string("zone", NULL, data);
	strncpy(zone1,zone,2);
	strncpy(zone2,zone+2,2);
	zone1_int = atoi(zone1);
	zone2_int = atoi(zone2);
   	format = get_object_int("format", data);
   	sscanf(clock, "%d-%d-%d %d:%d:%d", &time_tm.tm_year, &time_tm.tm_mon, &time_tm.tm_mday, &time_tm.tm_hour, &time_tm.tm_min, &time_tm.tm_sec);  
    time_tm.tm_year -= 1900;  
    time_tm.tm_mon -= 1;  
    time_tm.tm_wday = 0;  
    time_tm.tm_yday = 0;  
    time_tm.tm_isdst = 0;  
    time_tv.tv_sec = mktime(&time_tm);  
    time_tv.tv_usec = 0;
  	//  time_zo.tv_minuteswest=7;
   	// time_zo.tv_dsttime=0;
    settimeofday(&time_tv, NULL);  //根据app传来的设置当前时间   
    usleep(100000);
    seconds = time((time_t*)NULL);   
	DATA[3]  = seconds %256;
    seconds /=256;
    DATA[2]  = seconds %256;
    seconds /=256;
    DATA[1]  = seconds %256;
    seconds /=256;   
    DATA[0] = seconds;
    DATA[4] =	format;
    DATA[5] =	zone1_int;
	DATA[6] =   zone2_int;
	DATA[7] = '\0';
//	printf("DATA[7]:%s\n",(char *)DATA);
//	mozart_ini_setkey("/usr/data/system.ini", "time", "data", (char *)DATA);  //保存时区信息
    Pack_write(Sync_time,DATA,7);
	return 0;
}


int up_tower_sysc_time()
{
	int seconds;
	unsigned char DATA[7];
	char zone_buf[8] = {};
	char zone_buf_1[4] = {};
	char zone_buf_2[4] = {};
	char tformat_buf[8] = {};
	seconds = time((time_t*)NULL);
	DATA[3]  = seconds %256;
	seconds /=256;
	DATA[2]  = seconds %256;
	seconds /=256;
	DATA[1]  = seconds %256;
	seconds /=256;   
	DATA[0] = seconds;
	if(!mozart_ini_getkey("/usr/data/system.ini", "time", "zone", zone_buf))
	strncpy(zone_buf_1,zone_buf,2);
	strncpy(zone_buf_2,zone_buf+2,2);
	DATA[5] = atoi(zone_buf_1);
	DATA[6] = atoi(zone_buf_2);
	if(!mozart_ini_getkey("/usr/data/system.ini", "time", "tformat", tformat_buf))
	DATA[4] =	atoi(tformat_buf);
    Pack_write(Sync_time,DATA,7);
	return 0;
}


static char *get_uid(void)
{
	char uid_buf[20] = {};
  	json_object *reply_object = json_object_new_object();
  	char *reply_json = NULL;
	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "uid", uid_buf))
    json_object_object_add(reply_object, "uid", json_object_new_string(uid_buf));
	reply_json = reply_json_compose(reply_object,"142");
	//json_object_put(reply_object);
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/
	return reply_json;
}

static char *bind_uid(char *data)
{
	int res=0;
  	char *uid=NULL;
  	char uid_buf[20] = {};
  	json_object *reply_object = json_object_new_object();
  	char *reply_json = NULL;
   	uid =  get_object_string( "uid", NULL,data);

  	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "uid", uid_buf))
   		if(strcmp(uid_buf,uid)!=0)  
  	 	{
        	mozart_ini_setkey("/usr/data/system.ini", "base", "uid", uid);
            res=binduserId();
  	 	}
    json_object_object_add(reply_object, "uid", json_object_new_string(uid));
    json_object_object_add(reply_object, "res", json_object_new_int(res));
  	reply_json = reply_json_compose(reply_object,"144");
	//json_object_put(reply_object);
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/

	return reply_json;
}

static int get_movie_list(char *data)
{
	unsigned char da[1];
  	da[0] = get_object_int("data", data);
  	Pack_write(Get_gif_list,da,0);

	return 0;
}

static int movie_select(char *data)
{
	char *name = NULL;
 	name =  get_object_string( "name", NULL,data); 
	Pack_write(Select_gif,(unsigned char *)name,strlen(name)-1);
	return 0;
}

static int movie_ctrl(char *data)
{
	unsigned char da[1];
  	da[0] = get_object_int("data", data);
	Pack_write(Control_gif,da,1);

	return 0;
}

static char	*beiyong(char *data)
{
//	json_object *reply_object = json_object_new_object();
// 	char *reply_json = NULL;
	unsigned char da[15]={0};
  	da[0] = get_object_int("parama", data);
  	da[1] = get_object_int("paramb", data);
  	da[2] = get_object_int("paramc", data);
  	da[3] = get_object_int("paramd", data);
  	da[4] = get_object_int("parame", data);
  	da[5] = get_object_int("paramf", data);
  	da[6] = get_object_int("paramg", data);
  	da[7] = get_object_int("paramh", data);
  	da[8] = get_object_int("parami", data);
  	da[9] = get_object_int("paramj", data);
  	da[10] = get_object_int("paramk", data);
  	da[11] = get_object_int("paraml", data);
  	da[12] = get_object_int("paramm", data);
	da[13] = get_object_int("paramn", data);	//修改于4.28号
	da[14] = get_object_int("paramo", data);
	Pack_write(Beiyong,da,15);
#if 0
	//新添加协议0xd9,回复给APP（4.27号）
    json_object_object_add(reply_object, "parama", json_object_new_int((int)da[0]));
	json_object_object_add(reply_object, "paramb", json_object_new_int((int)da[1]));
	json_object_object_add(reply_object, "paramc", json_object_new_int((int)da[2]));
	json_object_object_add(reply_object, "paramd", json_object_new_int((int)da[3]));
	json_object_object_add(reply_object, "parame", json_object_new_int((int)da[4]));
	json_object_object_add(reply_object, "paramf", json_object_new_int((int)da[5]));
	json_object_object_add(reply_object, "paramg", json_object_new_int((int)da[6]));
	json_object_object_add(reply_object, "paramh", json_object_new_int((int)da[7]));
	json_object_object_add(reply_object, "parami", json_object_new_int((int)da[8]));
	json_object_object_add(reply_object, "paramg", json_object_new_int((int)da[9]));
	json_object_object_add(reply_object, "paramk", json_object_new_int((int)da[10]));
	json_object_object_add(reply_object, "paraml", json_object_new_int((int)da[11]));
	json_object_object_add(reply_object, "paramm", json_object_new_int((int)da[12]));
	reply_json = reply_json_compose(reply_object,"217");
	json_object_put(reply_object);
#endif
	return 0;
}

int reply_beiyong(unsigned char *date)
{
	unsigned char da[15] = {0};
	memcpy(da,date,15);
	json_object *reply_object = json_object_new_object();
	char *reply_json = NULL;
	json_object_object_add(reply_object, "parama", json_object_new_int((int)da[0]));
	json_object_object_add(reply_object, "paramb", json_object_new_int((int)da[1]));
	json_object_object_add(reply_object, "paramc", json_object_new_int((int)da[2]));
	json_object_object_add(reply_object, "paramd", json_object_new_int((int)da[3]));
	json_object_object_add(reply_object, "parame", json_object_new_int((int)da[4]));
	json_object_object_add(reply_object, "paramf", json_object_new_int((int)da[5]));
	json_object_object_add(reply_object, "paramg", json_object_new_int((int)da[6]));
	json_object_object_add(reply_object, "paramh", json_object_new_int((int)da[7]));
	json_object_object_add(reply_object, "parami", json_object_new_int((int)da[8]));
	json_object_object_add(reply_object, "paramj", json_object_new_int((int)da[9]));
	json_object_object_add(reply_object, "paramk", json_object_new_int((int)da[10]));
	json_object_object_add(reply_object, "paraml", json_object_new_int((int)da[11]));
	json_object_object_add(reply_object, "paramm", json_object_new_int((int)da[12]));
	json_object_object_add(reply_object, "paramn", json_object_new_int((int)da[13]));
	json_object_object_add(reply_object, "paramo", json_object_new_int((int)da[14]));
	reply_json = reply_json_compose(reply_object,"217");   //reply_json_compose里释放了reply_object对象
	/********************************修改于2018.6.29号****************************************************/ 
	if(reply_object)
	{
		json_object_put(reply_object);	
		reply_object = NULL;
	}
	/*****************************************************************************************************/
	if (reply_json) {
		mozart_appserver_notify("usercommand", reply_json);
		pr_debug("reply_json:%s\n", reply_json);
		free(reply_json);
	}
	return 0;
}

#if 1
/********************************************************************************************************************************/
static size_t receive_data(void *buffer, size_t size, size_t nmemb, FILE *file)
{
//	printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);
	size_t r_size = fwrite(buffer, size, nmemb, file);
    return r_size;
}

int progress_func(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUpload)  
{  
	Reply_Download *reply_app_info = (Reply_Download *)ptr;
	json_object *reply_object = NULL;
	char *reply_json = NULL;
//	printf("..........%d.........local:%s,list:%d,size:%d\n",__LINE__,reply_app_info->local,reply_app_info->list,reply_app_info->size);
    if(TotalToDownload != 0)
    {
//    	process = NowDownloaded / TotalToDownload;	//注意0不能为分母  

		if (NowDownloaded != 0)
		{
			reply_object = json_object_new_object();
			json_object_object_add(reply_object, "list", json_object_new_int(reply_app_info->list));
			json_object_object_add(reply_object, "local", json_object_new_string(reply_app_info->local));
			json_object_object_add(reply_object, "remote", json_object_new_string(""));
			json_object_object_add(reply_object, "size", json_object_new_int(reply_app_info->size));
			json_object_object_add(reply_object, "finish", json_object_new_int((int)NowDownloaded));
			//printf("\n【%s】 %s %d reply_object:%s  def:%s\n\n",__FILE__, __func__, __LINE__,json_object_get_string(reply_object),"def");
			reply_json = reply_json_compose(reply_object,"82");			//注：reply_object已经被释放了
			//不明白为什么？？？？？printf("\n【%s】 %s %d reply_object:%s  def:%s\n\n",__FILE__, __func__, __LINE__,json_object_get_string(reply_object),"def");
			#if 0
			if(reply_object != NULL)
			{				
				//printf("\n【%s】 %s %d reply_object:%s  abc:%s\n\n",__FILE__, __func__, __LINE__,json_object_get_string(reply_object),"abc");
				printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);
				json_object_put(reply_object);
				reply_object = NULL;
			}	
			#endif
			if (reply_json) {
				mozart_appserver_notify("usercommand", reply_json);
				pr_debug("reply_json:%s\n", reply_json);
				free(reply_json);
				reply_json = NULL;
			}
		}
    }
	
    return 0;  
}  

//功能：上传音乐
void *handle_download_music(void *arg)
{	
	char *data = (char *)arg;
	printf("【%s】 %s %d data:%s\n",__FILE__, __func__, __LINE__,data);

	Reply_Download *reply_app_info = (Reply_Download *)malloc(sizeof(Reply_Download));
	CURL *curl;
	curl = curl_easy_init();  
	static CURLcode res = -1;
	char new_name[512] = {};
	int list = 0;
	int len_u = 0;
	int len_l = 0;
	char *url = NULL;	//app传来的待下载的网址
	char *local = NULL;	//存放的路径
	char *local_tmp = NULL;	//存放音乐的全路径
	int size = 0;

	json_object *add_new_song = NULL;
	json_object	*add_new_song_tmp=json_object_new_object();
	json_object *object = NULL, *tmp = NULL;
	object = json_tokener_parse(data);					/*data == null 会导致程序崩溃*/

	if (json_object_object_get_ex(object, "list", &tmp))
    	list = json_object_get_int(tmp);
	if (json_object_object_get_ex(object, "size", &tmp))
    	size = json_object_get_int(tmp);
	if (json_object_object_get_ex(object, "url", &tmp))
	{
		len_u = strlen(json_object_get_string(tmp))+1;
		url = calloc(len_u,1);
		memset(url, 0, len_u);
		strncpy(url,json_object_get_string(tmp),len_u);
	}
	if (json_object_object_get_ex(object, "local", &tmp))
	{
		len_l = strlen(json_object_get_string(tmp))+1;
		printf("【%s】 %s %d len_l:%d\n",__FILE__, __func__, __LINE__,len_l);
		local = calloc(len_l,1);
		memset(local, 0, len_l);
		strncpy(local,json_object_get_string(tmp),len_l);
	}
	if(list == 1){
		local_tmp = malloc(strlen("/mnt/sdcard/free/") + strlen(local) + 1);
		sprintf(local_tmp, "/mnt/sdcard/free/%s",local);
	}
	if(list == 2){
		local_tmp = malloc(strlen("/mnt/sdcard/med/") + strlen(local) + 1);
		sprintf(local_tmp, "/mnt/sdcard/med/%s",local);
	}
	if(list == 3){
		local_tmp = malloc(strlen("/mnt/sdcard/sleep/") + strlen(local) + 1);
		sprintf(local_tmp, "/mnt/sdcard/sleep/%s",local);
	}
	if(list == 4){
		local_tmp = malloc(strlen("/mnt/sdcard/alarm/") + strlen(local) + 1);
		sprintf(local_tmp, "/mnt/sdcard/alarm/%s",local);
	}

	while(1)
	{
		printf("【%s】 %s %d local:%s,local_tmp:%s,len_l:%d,len_u:%d \n",__FILE__, __func__, __LINE__,local,local_tmp,len_l,len_u);
		if(*(local+len_l) == '.'){
			printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);
			if(len_l < 511)
				strncpy(new_name,local,len_l);		//将文件的名字拷贝赋给loadSong.name
			printf("%s %s %d new_name:%s。。。。。。。。。。。。。。。。。。\n",__FILE__, __func__, __LINE__,new_name);	
			break;
		}
		len_l--;			
	}

	printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);

/********************************************新添加于2018.6.27*******************************************************************/

	reply_app_info->list = list;
	
	reply_app_info->local = (char *)malloc(strlen(local)+1);
	memset(reply_app_info->local,0,strlen(local)+1);
	strncpy(reply_app_info->local,local,strlen(local)+1);
	
	reply_app_info->size = size;

	if(url)  
	{

		FILE *outfile = fopen(local_tmp,"w+"); 		//打开文件（没有当前文件就创建）
		curl_easy_setopt(curl, CURLOPT_URL, url);  
		//curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, downProcess); 	//设置从downProcess开始下载。
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile); 
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receive_data);  	//将数据写入文件的回调函数
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);  
	    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);  	//下载的响应进度回调函数(一秒回调一次)
	   	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA,reply_app_info); 
	    res = curl_easy_perform(curl);   

		if(res == 0)
		{
			printf("...................下载完成....................................\n");
			add_new_song = json_tokener_parse(new_mozart_music_list);
			free(new_mozart_music_list);
			/************************new_name不带.MP3*************************************************/
			json_object_object_add(add_new_song_tmp, "name", json_object_new_string(new_name));
			json_object_object_add(add_new_song_tmp, "url", json_object_new_string(local_tmp));
			json_object_array_add(add_new_song,add_new_song_tmp);
			new_mozart_music_list = strdup(json_object_get_string(add_new_song));
			
			if(reply_app_info->local != NULL)
			{
				free(reply_app_info->local);
				reply_app_info->local = NULL;
			}
			if(reply_app_info != NULL)
			{
				free(reply_app_info);
				reply_app_info = NULL;
			}
		}
		else
		{
			printf("下载失败......................\n");
		}
	    fclose(outfile);  

/******************************************************************************************************************************/
	}


	free(url);
	free(local);
	free(local_tmp);
	if(object != NULL)
	{
		json_object_put(object);
		object = NULL;
	}
	if (add_new_song_tmp != NULL)
	{
		json_object_put(add_new_song_tmp);
		add_new_song_tmp = NULL;
	}
	if (add_new_song != NULL)
	{
		json_object_put(add_new_song);
		add_new_song = NULL;
	}
	if(data != NULL)
	{
		free(data);
		data = NULL;
	}
	
  	return 0;  
}

int upload_music(char *data)
{
	int len = strlen(data)+1;
	printf(">>>>>>>>>>>len:%d>>>>>>>>>>>>>>>>>>>>\n",len);
	char *date_buf = (char *)malloc(len);
	memset(date_buf,0,len);
	memcpy(date_buf,data,len);
	pthread_t pthread_download;
	if (pthread_create(&pthread_download, NULL, handle_download_music, (void *)date_buf)){
		printf("create handle_download_music failed\n");
		return -1;
	}
	pthread_detach(pthread_download);
	
	return 0;
}



//功能：删除指定音乐并回复（0x4B）
static char *delect_music(char *data)
{
	int list = 0;
	int len = 0;
	int ret = -1;
	int music_num = 0;
	char *url = NULL;
	int i = 0;
	char *dir = NULL;
	json_object *object, *tmp, *reply_object;
	json_object *song_object = NULL;
	json_object *del_song;
	char *del_url = NULL;
	char *reply_json = NULL;
	reply_object = json_object_new_object();
	object = json_tokener_parse(data);	
	json_object *new_song_object = json_object_new_array();
		
	if (json_object_object_get_ex(object, "list", &tmp))
    	list = json_object_get_int(tmp);
	if (json_object_object_get_ex(object, "dir", &tmp))
	{
		len = strlen(json_object_get_string(tmp));
		dir = calloc(len+1,1);
		strncpy(dir,json_object_get_string(tmp),len+1);
	}
	 if(list == 1)
    {
    	url = malloc(strlen("/mnt/sdcard/free/") + strlen(dir) + 1);
        sprintf(url, "/mnt/sdcard/free/%s",dir);//此时url为free模式下的音乐列表的全路径	  
    }
	else if(list == 2)
	{
    	url = malloc(strlen("/mnt/sdcard/med/") + strlen(dir) + 1);
        sprintf(url, "/mnt/sdcard/med/%s",dir);
	}
	else if(list == 3)
	{
    	url = malloc(strlen("/mnt/sdcard/sleep/") + strlen(dir) + 1);
        sprintf(url, "/mnt/sdcard/sleep/%s",dir);
	}
	else if(list == 4)
	{
    	url = malloc(strlen("/mnt/sdcard/alarm/") + strlen(dir) + 1);
        sprintf(url, "/mnt/sdcard/alarm/%s",dir);
	}
	if(url != NULL)
	{
		ret = unlink(url);
		if(ret == 0){
			/*同时删除自己保存的音乐列表*/
			song_object = json_tokener_parse(new_mozart_music_list);
			printf("【%s】 %s %d song_object:%s\n",__FILE__, __func__, __LINE__,json_object_get_string(song_object));
			free(new_mozart_music_list);
	    	music_num = json_object_array_length(song_object); //数组的长度就是音乐的数量
			for (i = 0; i < music_num; i++) 
			{
				del_song = json_object_array_get_idx(song_object, i);
				if (json_object_object_get_ex(del_song, "url", &tmp))
				{
					len = strlen(json_object_get_string(tmp));
					del_url = calloc(len+1,1);
					strncpy(del_url,json_object_get_string(tmp),len+1);	
				}
				if(del_url != NULL)
				{
					if(strcmp(del_url,url) != 0)
					{
						json_object_array_add(new_song_object,del_song);
					}
				}
				if(del_url != NULL)
				{
					free(del_url);
					del_url = NULL;
				}
			}
			new_mozart_music_list = strdup(json_object_get_string(new_song_object));
			json_object_object_add(reply_object, "res", json_object_new_int(1));	
			json_object_object_add(reply_object, "reason", json_object_new_string(""));
		}
		else{
			/****删除失败***/
			json_object_object_add(reply_object, "res", json_object_new_int(0));
			json_object_object_add(reply_object, "reason", json_object_new_string("delete target file failed"));
		}
	}
	else{
		/*未找到此音乐文件*/
		json_object_object_add(reply_object, "dir", json_object_new_int(0));
		json_object_object_add(reply_object, "reason", json_object_new_string("find target file failed"));	
	}
	printf("【%s】 %s %d new_mozart_music_list:%s\n",__FILE__, __func__, __LINE__,new_mozart_music_list);
	json_object_object_add(reply_object, "list", json_object_new_int(list));
	json_object_object_add(reply_object, "dir", json_object_new_string(dir));
	reply_json = reply_json_compose(reply_object,"76");
	
	/*****************************************************************************************************/

	if(object != NULL)
	{
		json_object_put(object);
		object = NULL;
	}
	if (reply_object != NULL)
	{
		json_object_put(reply_object);
		reply_object = NULL;
	}
	if(new_song_object != NULL)
	{
		json_object_put(new_song_object);
		new_song_object = NULL;
	}
	if (song_object != NULL)
	{
		json_object_put(song_object);
		song_object = NULL;
	}
	/****************************************************************************************************/
	return reply_json;
}
#endif

static int ingenicplayer_handle_command(struct cmd_info_c *cmd_info)
{
	int index = -1;
	//int volume = 0;
	//char *name = NULL;
	char *data=NULL;
	char *command = NULL ;
	char *reply_json = NULL;
	char *reply2_json = NULL;
	json_object *object = NULL;
    json_object *tmp = NULL;
    time_t timep;
	
    reply_time = 0;
	object = json_tokener_parse(cmd_info->data) ;
	if (json_object_object_get_ex(object, "data", &tmp))
	{
		data =  calloc(strlen(json_object_get_string(tmp))+1,1); //开辟内存空间，大小为strlen（）的值
		strncpy(data,json_object_get_string(tmp),strlen(json_object_get_string(tmp))+1);
	}
	if(mozart_mozart_mode != CONFIG_WIFI){
		for (index = 0; index < sizeof(ingenicplayer_cmd)/sizeof(ingenicplayer_cmd[0]); index++) {
			if (strcmp(ingenicplayer_cmd[index], cmd_info->command) == 0)
			{
				command = strdup("usercommand");
			  	break;
			}
		}
	}
	printf("\n -------------------------------------------------------------------\n");
//	time(&timep);
//	pr_debug("start time1:%s",asctime(localtime(&timep)));

  	switch(index){
    	case PK_AB_ENTER_MED_MODE:   //进入冥想模式
			mozart_start_mode(MED);
         // usleep(50000);
		//	reply_json = get_work_status();
			break;
	 	case PK_AB_START_MED:     //开始冥想训练  （0x02）
			start_med(data)	; 	
			med_info.med_time=0;  
			med_info.meding = true;
	 		reply_json = get_work_status();
			break;
	 	case PK_AB_STOP_MED:      //停止冥想训练（0x03）
			if(med_info.meding)
			{ 
				med_info.meding = false;
	            time(&timep);		//获得当前时间
	            sprintf(med_info.medendtime, "%d-%d-%d %d:%d:%d",localtime(&timep)->tm_year+1900,localtime(&timep)->tm_mon+1,
	            localtime(&timep)->tm_mday,localtime(&timep)->tm_hour,localtime(&timep)->tm_min,localtime(&timep)->tm_sec);
		   		med_file_close();		 
		   	}
		 	if (mozart_musicplayer_get_status(mozart_musicplayer_handler) != PLAYER_STOPPED)
		           mozart_musicplayer_stop_playback(mozart_musicplayer_handler);  
			reply_json = get_work_status();
			reply2_json = get_med_data();
			mozart_appserver_notify("usercommand", reply2_json);  //每次冥想训练结束把冥想数据发送给APP
			/*提示音：继续冥想请按下冥想键，结束冥想请关闭脑波仪*/
			mozart_play_key("choice_if_continue_med");			
	       	break; 
	 	case PK_AB_GET_MED_DATA:   //返回最近一次单条冥想数据（0x05）
              reply_json = get_med_data();
	    	  break;
	 	case PK_AB_ENTER_SLEEP_MODE:   //进入睡眠模式
	 		system("echo 0 > /sys/class/leds/led-sleep/brightness");  
			mozart_start_mode(SLEEP);
          //  usleep(50000);
		//	reply_json = get_work_status();
		//	printf("%s %s %d 开始贪睡模式\n",__FILE__, __func__, __LINE__);
		//	upload_music(my_test_data);
			break;
	 	case PK_AB_EXIT_SLEEP_MODE:     //退出睡眠模式
	 		system("echo 1 > /sys/class/leds/led-sleep/brightness");
			mozart_start_mode(FREE);
			reply_json = get_work_status();
	       	break;
	 	case PK_AB_PAUSE_ALARM:         //暂停闹钟（贪睡）
	 		alarm_pause=1;
            reply_json = get_work_status();
			break;	 	
	 	case PK_AB_STOP_ALARM:         //停止闹钟（0x23）
	 		alarm_end=1;
            reply_json = get_work_status();
			break;
	 	case PK_AB_GET_ALARM_LIST:     //获取闹钟列表(0x24)
        	reply_json = get_alarm_list();
            break;
        case PK_AB_ADD_ALARM:          //添加闹钟（0x26）
			if((reply_json=add_alarm_check(data))==NULL)
        	{
            	printf("start add alarm\n");
            	add_alarm(data);
	            reply_json = get_alarm_list();
				if(reply_json == NULL)
				{
					printf("获取闹钟列表失败");
				}
			}
			break;
	 	case PK_AB_DEL_ALARM:         //删除闹钟
        	if((reply_json=del_alarm(data))==NULL)
            {
	        	reply_json = get_alarm_list();
            }
			break;
	 	case PK_AB_EDIT_ALARM:        //编辑闹钟
        	reply_json = edit_alarm(data);
			break;
	 	case PK_AB_GET_LED_EFFECT_LIST:   //获取灯光效果列表  （PS：暂时先放一放）
	 		Pack_write(Get_light_list,NULL,0);
            //  command = strdup("51");
            break;       
	 	case PK_AB_ENTER_FREE_MODE:   //进入自由（音乐）模式(0x40)
	 		system("echo 1 > /sys/class/leds/led-sleep/brightness"); 
			mozart_start_mode(FREE);
            usleep(50000);
			reply_json = get_work_status();
			break;
	 	case PK_AB_MUSIC_CTL_CMD:    //进入音乐播放控制（PS涉及到了MED模式0x41）
        	music_ctl_cmd(data);
	      	usleep(50000);		 
	      	reply_json =  get_work_status();	//打开于2018.7.18号
			break;
	 	case PK_AB_SET_MUSIC_VOLUME:   //设置音乐的音量（0x43）
        	set_music_volume(data);	  
			break;
	 	case PK_AB_PLAY_MUSIC:       //开始播放音乐（0x45）
        	if((reply_json=play_music(data))==NULL)
            {
          //  	reply_json =  get_work_status();
            }
			break;
	 	case PK_AB_GET_MUSIC_LIST:   	//获取音乐列表(0x47)
        	reply_json = get_music_list(data);	
			break;
	 	case PK_AB_GET_MUSIC_FILE_NUM:	//（0x47）
        	reply_json = get_music_file_num(data);
			break;
	 	case PK_AB_DEL_MUSIC:    //删除音乐
	 		reply_json = delect_music(data);
			break;
	 	case PK_AB_EDIT_MUSIC:    //编辑音乐
			break;
	 	case PK_AB_MOVE_MUSIC:    //移动音乐
	       	break;
	 	case PK_AB_UPLOAD_MUSIC:   //下载音乐(0x51，待完成)
			upload_music(data);
			break;
	  	case PK_AB_MUSIC_LOOP:     //设置音乐列表的播放模式
            reply_json = music_loop(data);
			break;
	    // 	break;
	  	case PK_AB_PUT_MUSIC_LIST:   //发送播放列表到base（0x56）
        	if((reply_json=put_music_list(data))==NULL)
            {
            	reply_json =  get_work_status();
            }
			break;	 
	  	case PK_AB_RAISE:      //设定当前升降的高度
        	raise(data);
	       	reply_json = get_height(20); 		  
		 	break;	  
	  	case PK_AB_GET_HEIGHT:   //获得当前的高度并返回给app（0x61）
            reply_json = get_height(20);
		   	break;
	  	case PK_AB_GET_DEV_CONFIG:  //获得硬件配置信息（0x70）
            reply_json = get_dev_config();            
	       	break;		
	  	case PK_AB_EDIT_DEV_NAME:     //编辑设备的名称(有问题BLE的修改意义不大)
            reply_json = edit_dev_name(data);
	       	break;
	   	case PK_AB_GET_WORK_STATUS:   //获取当前工作状态（0x74，返回给app0x75）
            reply_json = get_work_status();			 
			break;
	   	case PK_AB_GET_MED_CFG:    //获取冥想配置信息（0x76）
            reply_json = get_med_cfg(); //返回给app0x77
			break;
	   	case PK_AB_EDIT_MED_CFG:  //修改冥想配置信息（0x78）
            reply_json = edit_med_cfg(data);	//(0x79)
	        break;
	   	case PK_AB_GET_SLEEP_CFG:  //获取助眠配置信息（0x7A）
            reply_json = get_sleep_cfg(); //助眠配置信息（0x7B,返回给APP）
			break;	
	   case PK_AB_EDIT_SLEEP_CFG:   //修改助眠配置信息
            reply_json = edit_sleep_cfg(data);
			break;
	   case PK_AB_SET_TIME_CMD:    //修改时间配置指令
            reply_json = set_time_cmd(data);
			break;
	   case PK_AB_MOVIE_CTL_CMD:   //动画播放控制指令
	       	movie_ctl_cmd(data);
			break;
	   case PK_AB_LED_CTL_CMD:     //灯光控制指令
            led_ctl_cmd(data);
			break;
	   case PK_AB_WIFI_CTL_CMD:   //wifi控制命令：打开或关闭tower wifi
            wifi_ctl_cmd(data);
			break;	
 	   case PK_AB_CHECK_UPDATE:   //检查升级包指令
			check_update(data);
			break;
	   case PK_AB_RESET_CONFIG:   //恢复出厂设置
            reset_config(data);  
	   	    break;		
	   case PK_AB_SYNC_TIME:	//同步时钟
            sync_time(data);     
			break;	
	   case PK_AB_TOWER_ON:		//打开tower
            Pack_write(Power_on,NULL,0);
	  	    break;		
	   case PK_AB_TOWER_OFF:     //关闭tower
            Pack_write(Power_off,NULL,0);
            break;
       case PK_AB_GET_UID:      //请求绑定用户的ID
            reply_json = get_uid();
	   	    break;
	   case PK_AB_BIND_UID:    //绑定用户ID
			reply_json = bind_uid(data); 
	       	break;
	   case PK_AT_GET_MOVIE_LIST:	//通过base获取tower动画列表
			get_movie_list(data);
	       	break;
	   case PK_AB_MOVIE_SELECT:    //通过base向tower发送选定指定动画文件指令
            movie_select(data);
			break;	
	   case PK_AB_MOVIE_CTRL:    //通过base向tower发送动画播放控制指令
            movie_ctrl(data);
	   	    break;	
	   case PK_AP_BEIYONG:      //备用
      	    beiyong(data);
	       	break;		
	   case PK_xx:
		 	reply_time = 0;		 	
	  	    break;	
	   default:
			pr_err("Unknow ingenic  cmd %d %s\n", index, cmd_info->command);
			break;
  	}
	if(reply_json!=NULL)
        pr_debug("reply_json:%s\n", reply_json);
    if(reply2_json!=NULL)
		pr_debug("reply2_json:%s\n", reply2_json);
	if (reply_json && command && reply_json!=NULL)
		mozart_appserver_response(command, reply_json, cmd_info->owner);  //给APP做回复的

	free(reply_json);
    free(reply2_json);
	free(command);
	free(data);
	json_object_put(object);   //释放object

//	free_cmd_list(cmd_info);	//屏蔽于2018.7.9号

	return index;
}

int mozart_ingenicplayer_notify_volume(int volume)
{
	json_object *reply_object = NULL;
	json_object *object = NULL;

	if ((reply_object = json_object_new_object()) == NULL)
		return -1;
	if ((object = json_object_new_object()) == NULL)
		return -1;
	
	json_object_object_add(reply_object, "scope", json_object_new_string("68"));
	json_object_object_add(object, "volume", json_object_new_int(volume));
    json_object_object_add(reply_object, "data",object);

	pr_debug("notify : 68 %s\n",json_object_get_string(reply_object));
	mozart_appserver_notify("usercommand", (char *)json_object_get_string(reply_object));
	json_object_put(object);
	json_object_put(reply_object);

	return 0;
}

int mozart_ingenicplayer_notify_play_mode(enum play_mode loop)
{
	json_object *reply_object = NULL;
	json_object *object = NULL;

	if ((reply_object = json_object_new_object()) == NULL)
		return -1;
	if ((object = json_object_new_object()) == NULL)
		return -1;

    //pr_debug("loop==%d\n",loop);
	json_object_object_add(reply_object, "scope", json_object_new_string("84"));
	json_object_object_add(object, "loop", json_object_new_int(loop));
    json_object_object_add(reply_object, "data",object);

	//pr_debug("notify : 84%s\n",json_object_get_string(reply_object));
	mozart_appserver_notify("usercommand", (char *)json_object_get_string(reply_object));
	json_object_put(object);
	json_object_put(reply_object);

	return 0;
}

int mozart_ingenicplayer_notify_alarm(struct module_alarm_info *alarm)
{
	json_object *reply_object = NULL;
	json_object *object = NULL;

	if ((reply_object = json_object_new_object()) == NULL)
		return -1;
	if ((object = json_object_new_object()) == NULL)
		return -1;

	json_object_object_add(reply_object, "scope", json_object_new_string("52"));
	
	json_object_object_add(object, "id", json_object_new_int(alarm->id));
	json_object_object_add(object, "date", json_object_new_int(alarm->date));
	json_object_object_add(object, "mvol", json_object_new_int(alarm->mvol));
	json_object_object_add(object, "si", json_object_new_int(alarm->si));
	json_object_object_add(object, "sr", json_object_new_int(alarm->sr));
	json_object_object_add(object, "hour", json_object_new_int(alarm->hour));
	json_object_object_add(object, "minute", json_object_new_int(alarm->minute));
	json_object_object_add(object, "led", json_object_new_int(alarm->led));

	json_object_object_add(object, "en", json_object_new_boolean(alarm->enable));
	json_object_object_add(object, "re", json_object_new_boolean(alarm->repeat));

	json_object_object_add(object, "name", json_object_new_string(alarm->name));
	json_object_object_add(object, "mn", json_object_new_string(alarm->mname));
	
    json_object_object_add(reply_object, "data",object);

	mozart_appserver_notify("usercommand", (char *)json_object_get_string(reply_object));
	json_object_put(object);
	json_object_put(reply_object);

	return 0;
}

int mozart_ingenicplayer_light_list(int i)
{
	char *reply_json = NULL;
	reply_json = ingenicplayer_light_list(i);
	if (reply_json) {
		mozart_appserver_notify("usercommand", reply_json);
		pr_debug("reply_json:%s\n", reply_json);
		free(reply_json);
	}
	return 0;
}

int mozart_ingenicplayer_gif_state(unsigned char *data)
{
	json_object *reply_object = NULL;
	json_object *object = NULL;

	if ((reply_object = json_object_new_object()) == NULL)
		return -1;
	if ((object = json_object_new_object()) == NULL)
		return -1;

	json_object_object_add(reply_object, "scope", json_object_new_string("129"));
	   
	json_object_object_add(object, "status", json_object_new_int((int)data[0]));
	json_object_object_add(object, "cursor", json_object_new_int(data[1]*16+data[2]));
	json_object_object_add(object, "time", json_object_new_int(data[3]*16+data[4]));
	json_object_object_add(object, "name", json_object_new_string((char *)&data[5]));
	
    json_object_object_add(reply_object, "data",object);

	mozart_appserver_notify("usercommand", (char *)json_object_get_string(reply_object));
	json_object_put(object);
	json_object_put(reply_object);

	return 0;
}

/******************************************新添加于2018.6.25号*******************************************************************************/
static char *mozart_ingenicplayer_brain_data()
{
	char *reply_json = NULL;
	json_object *object = json_object_new_object();
	struct info *x_info;

	x_info = get_meddata_form_ble();
	json_object_object_add(object, "a", json_object_new_int((int)(x_info->X_brain_attention)));
    json_object_object_add(object, "m", json_object_new_int((int)(x_info->X_brain_concentration)));
    json_object_object_add(object, "p", json_object_new_int((int)(x_info->X_brain_quality)));
    json_object_object_add(object, "delta", json_object_new_int((int)(x_info->brain_parameter.delta[0])*256*256+(int)(x_info->brain_parameter.delta[1])*256+(int)(x_info->brain_parameter.delta[2])));
    json_object_object_add(object, "theta", json_object_new_int((int)(x_info->brain_parameter.theta[0])*256*256+(int)(x_info->brain_parameter.theta[1])*256+(int)(x_info->brain_parameter.theta[2])));
    json_object_object_add(object, "lowalpha", json_object_new_int((int)(x_info->brain_parameter.low_alpha[0])*256*256+(int)(x_info->brain_parameter.low_alpha[1])*256+(int)(x_info->brain_parameter.low_alpha[2])));
    json_object_object_add(object, "hightalpha", json_object_new_int((int)(x_info->brain_parameter.high_alpha[0])*256*256+(int)(x_info->brain_parameter.high_alpha[1])*256+(int)(x_info->brain_parameter.high_alpha[2])));
	json_object_object_add(object, "lowbeta", json_object_new_int((int)(x_info->brain_parameter.low_beta[0])*256*256+(int)(x_info->brain_parameter.low_beta[1])*256+(int)(x_info->brain_parameter.low_beta[2])));
	json_object_object_add(object, "hightbeta", json_object_new_int((int)(x_info->brain_parameter.high_beta[0])*256*256+(int)(x_info->brain_parameter.high_beta[1])*256+(int)(x_info->brain_parameter.high_beta[2])));
	json_object_object_add(object, "lowgamma", json_object_new_int((int)(x_info->brain_parameter.low_gamma[0])*256*256+(int)(x_info->brain_parameter.low_gamma[1])*256+(int)(x_info->brain_parameter.low_gamma[2])));
	json_object_object_add(object, "highgamma", json_object_new_int((int)(x_info->brain_parameter.high_gamma[0])*256*256+(int)(x_info->brain_parameter.high_gamma[1])*256+(int)(x_info->brain_parameter.high_gamma[2])));
	
	reply_json = reply_json_compose(object, "160");
	
	
	if(object != NULL)
	{
		printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);
		json_object_put(object);
		object = NULL;
	}
	return reply_json;
	
}

int mozart_ingenicplayer_notify_brain_data()
{
	char *reply_json = NULL;

	reply_json = mozart_ingenicplayer_brain_data();
	printf("\n reply_object:%s \n",reply_json);
	if(reply_json)
	{
		mozart_appserver_notify("usercommand", reply_json);
		free(reply_json);
	}
	return 0;	
}

/*******************************************************************************************************************************/

int mozart_ingenicplayer_light_state(unsigned char *data)
{
	json_object *reply_object = NULL;
	json_object *object = NULL;
	json_object *mode_object = NULL;
	json_object *rgb_object = NULL;
	json_object *bright_object = NULL;

	if ((reply_object = json_object_new_object()) == NULL)
		return -1;
	if ((object = json_object_new_object()) == NULL)
		return -1;
	if ((mode_object = json_object_new_object()) == NULL)
		return -1;
	if ((rgb_object = json_object_new_object()) == NULL)
		return -1;
	if ((bright_object = json_object_new_object()) == NULL)
		return -1;

	json_object_object_add(reply_object, "scope", json_object_new_string("131"));
	json_object_object_add(mode_object, "cmd", json_object_new_int((int)data[0]));
	json_object_object_add(mode_object, "id", json_object_new_int((int)data[1]));
	json_object_object_add(object, "mode",mode_object);
	json_object_object_add(rgb_object, "r", json_object_new_int(0));
	json_object_object_add(rgb_object, "g", json_object_new_int(0));
	json_object_object_add(rgb_object, "b", json_object_new_int(0));
	json_object_object_add(object, "rgb",rgb_object);

	json_object_object_add(bright_object, "level", json_object_new_int((int)data[2]));
	json_object_object_add(object, "bright",bright_object);
	
    json_object_object_add(reply_object, "data",object);

	mozart_appserver_notify("usercommand", (char *)json_object_get_string(reply_object));
	json_object_put(object);
	json_object_put(reply_object);
	json_object_put(mode_object);
	json_object_put(rgb_object);
	json_object_put(bright_object);

	return 0;
}


int mozart_ingenicplayer_wifi_state(unsigned char *data)
{
	json_object *reply_object = NULL;
	json_object *object = NULL;
	char ip[20]={};

	if ((reply_object = json_object_new_object()) == NULL)
		return -1;
	if ((object = json_object_new_object()) == NULL)
		return -1;

	json_object_object_add(reply_object, "scope", json_object_new_string("133"));
	   
	json_object_object_add(object, "wifi", json_object_new_int(data[0]));
	sprintf(ip, "%d.%d.%d.%d",data[1],data[2],data[3],data[4]);
	json_object_object_add(object, "ip", json_object_new_string(ip));
	json_object_object_add(object, "progress", json_object_new_int(data[5]*256+data[6]));
	
    json_object_object_add(reply_object, "data",object);

	mozart_appserver_notify("usercommand", (char *)json_object_get_string(reply_object));
	json_object_put(object);
	json_object_put(reply_object);

	return 0;
}

int mozart_ingenicplayer_upgrade_state(unsigned char *data)
{
	json_object *reply_object = NULL;
	json_object *object = NULL;

	if ((reply_object = json_object_new_object()) == NULL)
		return -1;
	if ((object = json_object_new_object()) == NULL)
		return -1;

	json_object_object_add(reply_object, "scope", json_object_new_string("135"));
	   
	json_object_object_add(object, "type", json_object_new_int(2));
	json_object_object_add(object, "newver", json_object_new_string("null"));
	json_object_object_add(object, "flag", json_object_new_int(data[0]));
	json_object_object_add(object, "progress", json_object_new_int(data[1]));
	
    json_object_object_add(reply_object, "data",object);

	mozart_appserver_notify("usercommand", (char *)json_object_get_string(reply_object));
	json_object_put(object);
	json_object_put(reply_object);

	return 0;
}

int mozart_ingenicplayer_gif_list(int i)
{
	char *reply_json = NULL;
	reply_json = ingenicplayer_gif_list(i);
	if (reply_json) {
		mozart_appserver_notify("usercommand", reply_json);
		pr_debug("reply_json:%s\n", reply_json);
		free(reply_json);
	}

	return 0;
}

#if 0
//通知app当前音乐播放的位置？？？	
int mozart_ingenicplayer_notify_pos(void)
{
	char *reply_json = NULL;

	reply_json = ingenicplayer_get_position(); 
	if (reply_json) {
		mozart_appserver_notify("usercommand", reply_json);
		free(reply_json);
	}
	return 0;
}
#endif

//功能：给APP回复音乐的播放状态，包括：播放状态，播放时间，播放进度，歌曲信息，【冥想数据】
int mozart_ingenicplayer_notify_song_info(void)
{
	char *reply_json = NULL;

	reply_json = ingenicplayer_get_song_info();
	if (reply_json) 
	{
	//	pr_debug("notify : 66 %s\n",reply_json);
		mozart_appserver_notify("usercommand", reply_json);  //将消息reply_json发送给app
		free(reply_json);
	}
	return 0;
}

//通知app base在什么模式下音乐播放的状态
int mozart_ingenicplayer_notify_work_status(void)
{
	char *reply_json = NULL;

	reply_json = get_work_status();
	if (reply_json) {
		mozart_appserver_notify("usercommand", reply_json);
		free(reply_json);
	}
	return 0;
}

int mozart_ingenicplayer_notify_get_hight(int i)
{
	char *reply_json = NULL;

	reply_json = get_height(i);
	if (reply_json) {
		mozart_appserver_notify("usercommand", reply_json);
		free(reply_json);
	}
	return 0;
}

int mozart_ingenicplayer_get_med_data(void)
{
	char *reply_json = NULL;

	reply_json = get_med_data();
	if (reply_json) {
		mozart_appserver_notify("usercommand", reply_json);
		free(reply_json);
	}

	return 0;
}
int mozart_ingenicplayer_response_cmd(char *command, char *data, struct appserver_cli_info *owner)
{
	struct cmd_info_c *cmd_info = NULL;

	cmd_info = (struct cmd_info_c *)calloc(sizeof(struct cmd_info_c), sizeof(char));

	cmd_info->owner = (struct appserver_cli_info *)calloc(
			sizeof(struct appserver_cli_info), sizeof(char));
	if (!cmd_info->owner) {
		pr_err("calloc %s\n", strerror(errno));
		free(cmd_info);
		return 0;
	}
	memcpy(cmd_info->owner, owner, sizeof(struct appserver_cli_info));
	cmd_info->command = strdup(command);
	cmd_info->data = strdup(data);

	pthread_mutex_lock(&cmd_mutex);
	list_insert_at_tail(&app_cmd_list, (struct cmd_info_c *)cmd_info);		//将app发送来的指令插入到链表的尾部
	pthread_cond_signal(&cond_var);
	pthread_mutex_unlock(&cmd_mutex);

	return 0;
}

//开启与APP的通信的功能
static void *ingenicplayer_func(void *arg)
{
	char devicename[64] = {};
	char sn_buf[64] = {};
	struct stat devicename_stat;  // 申请一个stat结构体的数据类型变量。
	
	memset(macaddr, 0, sizeof(macaddr));
	get_mac_addr("wlan0", macaddr, "");
	/*将base的序列号保存到本地（5.14号）*/
	strcpy(sn_buf,"B");
	strcat(sn_buf,macaddr);
	mozart_ini_setkey("/usr/data/system.ini", "base", "sn", sn_buf);	
	
	system("mkdir -p /usr/data/ingenicplayer");

	stat(DEVICENAME, &devicename_stat);  //将DEVICENAME路径下的文件的基本信息存储到devicename_stat结构体中
	if (devicename_stat.st_size == 0) {  //该文件为空
		sprintf(devicename, "EGO-%s", macaddr + 4);  //将wlan0的地址加4存到devicename
		pr_debug("devicename %s\n", devicename);
		sync_devicename(devicename);  //将devicename的内容写到DEVICENAME路径下的文件里
	}

	pthread_mutex_lock(&cmd_mutex);    //加锁
	while (die_out) {   //此时die_out = true
		if (!is_empty(&app_cmd_list)) {
			struct cmd_info_c *cmd_info;
			cmd_info = list_delete_at_index(&app_cmd_list, 0);	//逻辑：从链表的头开始，一个个的删除控制指令，并返回删除的内容。

			if (cmd_info) {
				pthread_mutex_unlock(&cmd_mutex);
				if((mozart_mozart_mode != CONFIG_WIFI)&&(mozart_mozart_mode != CONFIG_BT))	//新添加于2018.7.9号
				{
					printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);
					ingenicplayer_handle_command(cmd_info);
				}
				free_cmd_list(cmd_info);	//新添加于2018.7.9号
				pthread_mutex_lock(&cmd_mutex);
			}
		} else {
			pthread_cond_wait(&cond_var, &cmd_mutex);
		}
	}

	list_destroy(&app_cmd_list, free_cmd_list);
	pthread_mutex_unlock(&cmd_mutex);

#ifdef SUPPORT_SONG_SUPPLYER    //打开了。
	if (snd_source == SND_SRC_CLOUD)
		mozart_musicplayer_stop(mozart_musicplayer_handler);
#endif

	return NULL;
}

//给APP回复心跳包（255）
static void *heart_rate(void *arg)
{
	json_object *object = NULL;
    char *reply_json = NULL;
	
    while(1)
    {
    	reply_json = reply_json_compose(object,"255");
	    //pr_debug("heart rate time:%s",asctime(localtime(&timep)));
        mozart_appserver_notify("usercommand", reply_json);
		sleep(15);	 
    }
	return NULL;
}

//心跳的线程
static void ingenicplayer_heart_rate(void)
{
	if(0 != pthread_create(&heart_rate_thread, NULL, heart_rate, NULL))
		printf("Can't create music status reply!\n");
		
	pthread_detach(heart_rate_thread);
}

//开启与app通信的功能
int mozart_ingenicplayer_startup(void)
{
	pthread_mutex_lock(&cmd_mutex);
	if (die_out) {
		pthread_mutex_unlock(&cmd_mutex);
		return 0;
	}

	die_out = true;
	list_init(&app_cmd_list);

	if (pthread_create(&ingenicplayer_thread, NULL, ingenicplayer_func, NULL)) {	/* 线程一 */
		pr_err("create ingenicplayer_thread failed\n");
		return -1;
	}
	pthread_detach(ingenicplayer_thread);
	pthread_mutex_unlock(&cmd_mutex);

    ingenicplayer_heart_rate();  //线程2...给app回复心跳
		
	return 0;
}

int mozart_ingenicplayer_shutdown(void)
{
	pthread_mutex_lock(&cmd_mutex);
	if (die_out) {
		die_out = false;
		pthread_cond_signal(&cond_var);
	}
	pthread_mutex_unlock(&cmd_mutex);
	return 0;
}
