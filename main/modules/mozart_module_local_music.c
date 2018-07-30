#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <limits.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/time.h>


#include "ini_interface.h"
#include "utils_interface.h"
#include "json-c/json.h"
#include "tips_interface.h"
#include "localplayer_interface.h"

#include "mozart_config.h"
#include "mozart_musicplayer.h"
#include "mozart_key_function.h"
#include "mozart_ui.h"
#include "modules/mozart_module_local_music.h"
#include "modules/mozart_module_bt.h"
#include "egoserver.h"
#include "ingenicplayer.h"
#include "lifting_appliance.h"


#if (SUPPORT_LOCALPLAYER == 1)

#define states_change_lock(lock)						\
	do {                                                            \
		int i = 0;                                              \
									\
		while (pthread_mutex_trylock(lock)) {                   \
			if (i++ >= 100) {                               \
				printf("#### {%s, %s, %d} dead lock (last:)####\n", \
				       __FILE__, __func__, __LINE__); \
				i = 0;                                  \
			}                                               \
			usleep(100 * 1000);                             \
		}                                                       			\
	} while (0)

#define states_change_unlock(lock)					\
	do {								\
		pthread_mutex_unlock(lock);				\
	} while (0)



enum mozart_mode mozart_mozart_mode =0;

enum play_mode mozart_play_mode[4]={play_mode_order,play_mode_order,play_mode_single,play_mode_order};

#define MAXLINE 1024  

int mozart_duration=0;   //助眠音乐持续的时间，单位为秒
struct Med_Info med_info; 
struct Sleep_Info sleep_info; 

static pthread_mutex_t work_states_mutex = PTHREAD_MUTEX_INITIALIZER;

/*********************************************************************************/
#define PER_MAX_IO_BYTES 2048
size_t g_musicBytes_F = 0;
size_t g_musicBytes_M = 0;
size_t g_musicBytes_S = 0;

#define FREE_MUSIC_PATHNAME "/usr/data/free.txt"
#define MED_MUSIC_PATHNAME "/usr/data/med.txt"
#define SLEEP_MUSIC_PATHNAME "/usr/data/sleep.txt"

#define FREE_MUSIC_LEN_PATHNAME "/usr/data/free_len.txt"
#define MED_MUSIC_LEN_PATHNAME "/usr/data/med_len.txt"
#define SLEEP_MUSIC_LEN_PATHNAME "/usr/data/sleep_len.txt"




void sleep_info_init()
{
	time_t timep;	
	struct music_info *info = NULL;
	char macaddr[] = "255.255.255.255";
	char alarm_buf[8] = {};
		
	memset(sleep_info.sleepstarttime, 0, 30);
	memset(sleep_info.mname, 0, 100);
	memset(sleep_info.sleependtime, 0, 30);
	memset(sleep_info.name, 0, 100);
	sleep_info.med_time=0;
	sleep_info.snooze=0;
	sleep_info.alarm_id=0;
    sleep_info.sleeping = true;
		 
	info = mozart_musicplayer_musiclist_get_current_info(mozart_musicplayer_handler);
	printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);
	
	if(info!=NULL)
    	strncpy(sleep_info.mname,info->music_name,strlen(info->music_name));
	
	time(&timep);
	sprintf(sleep_info.sleepstarttime, "%d-%d-%d %d:%d:%d",
			localtime(&timep)->tm_year+1900,localtime(&timep)->tm_mon+1,
	 	    localtime(&timep)->tm_mday,localtime(&timep)->tm_hour,
	 	    localtime(&timep)->tm_min,localtime(&timep)->tm_sec);

	memset(macaddr, 0, sizeof(macaddr));
	get_mac_addr("wlan0", macaddr, "");
	//修改于2018.7.25文件名应该是_S结尾
	sprintf(sleep_info.name, "/mnt/sdcard/data1/%s_%d%02d%02d_%02d%02d%02d_S.dat", 
						macaddr,localtime(&timep)->tm_year+1900,localtime(&timep)->tm_mon+1,
	 	                localtime(&timep)->tm_mday,localtime(&timep)->tm_hour,
	 	                localtime(&timep)->tm_min,localtime(&timep)->tm_sec);
	printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);

	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "alarm", alarm_buf))
  	sleep_info.alarm_id=atoi(alarm_buf);
}

void med_info_init()
{
	time_t timep;	
	struct music_info *info = NULL;
	char macaddr[] = "255.255.255.255";

	memset(med_info.zhuanzhu, 0, 3600);
	memset(med_info.fangsong, 0, 3600);
	memset(med_info.medstarttime, 0, 30);
	memset(med_info.mname, 0, 100);
	memset(med_info.medendtime, 0, 30);
	memset(med_info.name_B, 0, 100);
	memset(med_info.name_R, 0, 100);
	memset(med_info.name_M, 0, 100);

	info = mozart_musicplayer_musiclist_get_current_info(mozart_musicplayer_handler);
	if(info!=NULL)
    	strncpy(med_info.mname,info->music_name,strlen(info->music_name));
	
	time(&timep);
	sprintf(med_info.medstarttime, "%d-%d-%d %d:%d:%d",
								localtime(&timep)->tm_year+1900,localtime(&timep)->tm_mon+1,
	 	                        localtime(&timep)->tm_mday,localtime(&timep)->tm_hour,
	 	                        localtime(&timep)->tm_min,localtime(&timep)->tm_sec);

    memset(macaddr, 0, sizeof(macaddr));
	get_mac_addr("wlan0", macaddr, "");
	sprintf(med_info.name_B, "/mnt/sdcard/data/%s_%d%02d%02d_%02d%02d%02d_B.dat", 
						macaddr,localtime(&timep)->tm_year+1900,localtime(&timep)->tm_mon+1,
	 	                localtime(&timep)->tm_mday,localtime(&timep)->tm_hour,
	 	                localtime(&timep)->tm_min,localtime(&timep)->tm_sec);
	sprintf(med_info.name_R, "/mnt/sdcard/data/%s_%d%02d%02d_%02d%02d%02d_R.dat", 
						macaddr,localtime(&timep)->tm_year+1900,localtime(&timep)->tm_mon+1,
	 	                localtime(&timep)->tm_mday,localtime(&timep)->tm_hour,
	 	                localtime(&timep)->tm_min,localtime(&timep)->tm_sec);
	sprintf(med_info.name_M, "/mnt/sdcard/data/%s_%d%02d%02d_%02d%02d%02d_M.dat", 
	 						macaddr,localtime(&timep)->tm_year+1900,localtime(&timep)->tm_mon+1,
	 	                    localtime(&timep)->tm_mday,localtime(&timep)->tm_hour,
	 	                    localtime(&timep)->tm_min,localtime(&timep)->tm_sec);

	med_file_open();
}

static int update_list_from_sdcard(enum mozart_mode mode)
{
	printf("\n	   (update_list_from_sdcard)  \n");

	int ret, i, music_num, id = 0;
	char *music = NULL;
	const char *music_name = NULL;
	const char *music_url = NULL;
	json_object *object, *song, *tmp;
	struct music_info *info = NULL; 

	
    if(tfcard_status != 1){					//判断一下SD卡是否插入
		printf("tfcard is empty\n");
		//逻辑关系是什么？
		if (!mozart_musicplayer_is_active(mozart_musicplayer_handler))
		{
			
			if (mozart_musicplayer_start(mozart_musicplayer_handler))
			{
				
				return -1;
			}
		}
    }
	
	mozart_musicplayer_musiclist_clean(mozart_musicplayer_handler);
	

//	music = mozart_localplayer_get_musiclist();		//获取（SD卡）本地播放列表，返回json格式的音乐数组
	if (new_mozart_music_list == NULL)
	{
		return -1;
	}
	music = calloc(strlen(new_mozart_music_list)+1,1);
	memset(music, 0, strlen(new_mozart_music_list)+1);
	strcpy(music,new_mozart_music_list);


	if (music == NULL) {
		printf("localplayer's music list is empty\n");
		return -1;
	}
	
	object = json_tokener_parse(music);
	if (object == NULL) {
		printf("%s: json parse failed\n", __func__);
		free(music);		//为什么用free
		return -1;
	}
	music_num = json_object_array_length(object);	//获取列表音乐的数量
	
	for (i = 0; i < music_num; i++) {
		song = json_object_array_get_idx(object, i);
		if (json_object_object_get_ex(song, "name", &tmp))
			music_name = json_object_get_string(tmp);
		if (json_object_object_get_ex(song, "url", &tmp))
			music_url = json_object_get_string(tmp);

		if(mode==FREE)
        {
			if(*(music_url+12)!='f')
				continue;
        }
		else if(mode==MED)
		{
			if(*(music_url+12)!='m')
		    	continue;
		}
		else if(mode==SLEEP)
		{
			if(*(music_url+12)!='s')
		    	continue;
		}
		else continue;

		info = mozart_musiclist_get_info(id, (char *)music_name, (char *)music_url, NULL,
						 NULL, NULL, NULL);
		if (info)
		{
			mozart_musicplayer_musiclist_insert(mozart_musicplayer_handler, info);	//逻辑：将此时的音乐信息添加到播放器中
		}
	}

	mozart_musicplayer_musiclist_set_play_mode(mozart_musicplayer_handler, mozart_play_mode[mode]);		//设置播放模式

	if(object != NULL)
	{
		json_object_put(object);
		object = NULL;
	}
	if(music != NULL)
	{
		free(music);
		music = NULL;
	}
	return ret;
}

//功能：把歌曲的信息存放到object指向的内存空间去
static void get_song_json_object(json_object *object, struct music_info *info)
{
	json_object *reply_object = object;

	if (object == NULL || info == NULL)
		return ;

	if (info->id)
		json_object_object_add(reply_object, "song_id",
				json_object_new_int(info->id));
	if (info->music_name)
		json_object_object_add(reply_object, "song_name",
				json_object_new_string(info->music_name));
	if (info->music_url)
		json_object_object_add(reply_object, "songurl",
				json_object_new_string(info->music_url));
	if (info->music_picture)
		json_object_object_add(reply_object, "picture",
				json_object_new_string(info->music_picture));
	if (info->albums_name)
		json_object_object_add(reply_object, "albumsname",
				json_object_new_string(info->albums_name));
	if (info->artists_name)
		json_object_object_add(reply_object, "artists_name",
				json_object_new_string(info->artists_name));
	if (info->data)
		json_object_object_add(reply_object, "data",
				json_object_new_string(info->data));
}

/************************************2018.7.8号:将历史列表保存到SD卡的文件中***************************/
int openFile(const char *pathname, int flags)
{
    int fd = open(pathname, flags|O_CREAT, 0664);
    if (-1 == fd)
    {
        printf("open erron\n");
        return -1;
    }
    return fd;
}

ssize_t writeDataToFile(int fd, void *buf, size_t count)
{
    ssize_t iWrited = -1;
    if (NULL != buf && count > 0)
    {
        int ret = -1;
        while (count)
        {
            if (PER_MAX_IO_BYTES < count)
            {
                ret = write(fd, (char *)buf+iWrited, PER_MAX_IO_BYTES);
            }
            else
            {
                ret = write(fd, (char *)buf+iWrited, count);
            }
            if (-1 == ret || 0 == ret)
            {
                break;
            }
            iWrited += ret;
            count -= ret;
        }
    }

    return iWrited;
}

ssize_t readDataFromFile(int fd, void *buf, size_t count)
{
    ssize_t iReaded = -1;
    if (NULL != buf && count > 0)
    {
        int ret = -1;
        while (count)
        {
            if (PER_MAX_IO_BYTES < count)
            {
                ret = read(fd, (char *)buf+iReaded, PER_MAX_IO_BYTES);
            }
            else
            {
                ret = read(fd, (char *)buf+iReaded, count);
            }
            if (0 == ret || -1 == ret)
            {
                break;
            }
            iReaded += ret;
            count -= ret;
        }
    }

    return iReaded;
}

int writeLenToFile(const char *pathname,int len)
{
	FILE *fp;
	fp = fopen(pathname,"w");
	if(fp)
	{
		fprintf(fp,"%d",len);
		fclose(fp);
	}
	else
	{
		printf("erron\n");
		return -1;
	}
	return 0;
}

int readLenFromFile(const char *pathname)
{
	int len = -1;
	FILE *fp;
	fp = fopen(pathname,"r");
	if(fp)
	{
		fscanf(fp,"%d",&len);
		fclose(fp);
	}
	else
	{
		printf("erron\n");
		return -1;
	}
	return len;
}


/******************************************************************************************************/
/*保存历史播放列表，便于进行模式切换时智能的切换到对应的播放队列*/
static void preserve_list(enum mozart_mode mode)
{
	int i = 0;
	int length = -1;
	char *reply_json = NULL;
	json_object *reply_object;
	json_object *songs;
	struct music_info *info = NULL;
	int fd = -1;
	int ret = 0;

	reply_object = json_object_new_object();
	if (!reply_object)
		goto object_err;
	songs = json_object_new_array();
	if (!songs)
		goto object_err;

	json_object_object_add(reply_object, "songs", songs);

	length = mozart_musicplayer_musiclist_get_length(mozart_musicplayer_handler);
	if(length==-1)
		goto object_err;
	
	for (i = 0; i < length; i++) {
		json_object *songs_object = json_object_new_object();
		if (!songs_object)
			goto object_err;

		info = mozart_musicplayer_musiclist_get_index_info(mozart_musicplayer_handler, i);
		if (info != NULL) {
			get_song_json_object(songs_object, info);
			json_object_array_add(songs, songs_object);
			/*需要手动释放info嘛	？？？*/
		}
	}
	reply_json = (char *)calloc(strlen(json_object_get_string(reply_object))
			+ 1, sizeof(char));
	if (!reply_json) {
		goto object_err;
	}
	strcpy(reply_json, (const char *)json_object_get_string(reply_object));
	
	/**********************************2018.7.8号*************************************************/
	
	//printf("***************LINE:%d**********strlen:%d**************保存到文件系统的歌单:%s\n",__LINE__,strlen(reply_json),reply_json);
 	switch(mode)
	{
    	case(FREE):
			g_musicBytes_F = strlen(reply_json) +1;
			writeLenToFile(FREE_MUSIC_LEN_PATHNAME,strlen(reply_json) +1);
			fd = openFile(FREE_MUSIC_PATHNAME, O_WRONLY);
			ret = writeDataToFile(fd, reply_json, g_musicBytes_F);
			if(-1 == ret)
			{
				printf("writeDataToFile erron\n");
				return;
			}
        	//mozart_ini_setkey("/usr/data/system.ini", "list", "free", reply_json);
        break;   
        case(MED):
			g_musicBytes_M = strlen(reply_json) +1;
			writeLenToFile(MED_MUSIC_LEN_PATHNAME,strlen(reply_json) +1);
			fd = openFile(MED_MUSIC_PATHNAME, O_WRONLY);
			writeDataToFile(fd, reply_json, g_musicBytes_M);
			if(-1 == ret)
			{
				printf("writeDataToFile erron\n");
				return;
			}
            //mozart_ini_setkey("/usr/data/system.ini", "list", "med", reply_json);
        break;   
        case(SLEEP):
			g_musicBytes_S = strlen(reply_json) +1;
			writeLenToFile(SLEEP_MUSIC_LEN_PATHNAME,strlen(reply_json) +1);
			fd = openFile(SLEEP_MUSIC_PATHNAME, O_WRONLY);
			writeDataToFile(fd, reply_json, g_musicBytes_S);
			if(-1 == ret)
			{
				printf("writeDataToFile erron\n");
				return;
			}
            //mozart_ini_setkey("/usr/data/system.ini", "list", "sleep", reply_json);
        break;   
		case(CONFIG_WIFI):
			break;
		case(CONFIG_BT):
			break;
		case(MUSIC_BT):
			break;
		case(MUSIC_DLNA):
			break;
		case(MUSIC_AIRPLAY):
			break;
	}
	free(reply_json);
	close(fd);
	/*********************************************************************************************/
object_err:
	json_object_put(reply_object);
	return;
}

//功能：从历史播放列表中更新播放列表
static void update_list_from_recordlist(char *value)
{
	//printf("\n	   (update_list_from_recordlist)   value:%s \n",value);

	int i, music_num = 0;
  	int id=0;
  	const char *music_name = NULL;
  	const char *music_url = NULL;
  	const char *music_picture = NULL;
  	const char *albums_name = NULL;
  	const char *artists_name = NULL;
  	const char *data = NULL;
  	json_object *object=NULL, *song=NULL, *tmp=NULL;
	json_object *object_tmp=NULL;
  	struct music_info *info = NULL;
	char *json_song = NULL;
 
  	mozart_musicplayer_musiclist_clean(mozart_musicplayer_handler);	
	printf("\n	   (update_list_from_recordlist)  1\n");

	/*********************************修改于2018.7.7**********************************************************/
	
	object_tmp = json_tokener_parse(value); 	
	
	printf("\n............................LINE:%d.....................................\n",__LINE__);
	if (json_object_object_get_ex(object_tmp, "songs", &tmp));	
	{
	//	printf("--------%d:-----------从历史列表中拿到的歌曲信息:%s\n",__LINE__,json_object_get_string(tmp));
		if(json_object_get_string(tmp) != NULL)
		{
			json_song = malloc(strlen(json_object_get_string(tmp))+1);
			memset(json_song,0,strlen(json_object_get_string(tmp))+1);
			memcpy(json_song,json_object_get_string(tmp),strlen(json_object_get_string(tmp))+1);
			//printf("--------%d:-----------从历史列表中拿到的歌曲信息:%s\n",__LINE__,json_song);
			object = json_tokener_parse(json_song);	

		}
		printf("\n............................LINE:%d.....................................\n",__LINE__);
	}
	/********************************************************************************************************/

	//object = json_tokener_parse(value);		//逻辑：将value字符串转化成json格式赋给object					“未释放指针”
  	//if (json_object_object_get_ex(object, "songs", &tmp))	//逻辑：将object中的songs属性的内容赋给tmp。
    //object = json_tokener_parse(json_object_get_string(tmp));	
  
  	music_num = json_object_array_length(object);
	
  	for (i = 0; i < music_num; i++) {
		song = json_object_array_get_idx(object, i);

		if (json_object_object_get_ex(song, "song_id", &tmp))
			id = json_object_get_int(tmp);
		if (json_object_object_get_ex(song, "song_name", &tmp))
			music_name = json_object_get_string(tmp);
		if (json_object_object_get_ex(song, "songurl", &tmp))
			music_url = json_object_get_string(tmp);
		if (json_object_object_get_ex(song, "picture", &tmp))
			music_picture = json_object_get_string(tmp);
		if (json_object_object_get_ex(song, "albumsname", &tmp))
			albums_name = json_object_get_string(tmp);
		if (json_object_object_get_ex(song, "artists_name", &tmp))
			artists_name = json_object_get_string(tmp);
		if (json_object_object_get_ex(song, "data", &tmp))
			data = json_object_get_string(tmp);

		/*******************************************2018.7.7号************************************************/
		//printf("........%d...........id:%d,music_name:%s,music_url:%s,music_picture:%s,albums_name:%s,artists_name:%s,data:%s\n",__LINE__,id,
		//						(char *)music_name, (char *)music_url,(char *)music_picture,(char *)albums_name,(char *)artists_name,(char*)data);
				
		/*************************************************************************************************/
		info = mozart_musiclist_get_info(id, (char *)music_name, (char *)music_url, (char *)music_picture,
						(char *)albums_name, (char *)artists_name, (char *)data);
		if (info)
			mozart_musicplayer_musiclist_insert(mozart_musicplayer_handler, info);
	}
	/*********************************修改于2018.6.25**********************************************************/
	if(object_tmp != NULL)
	{
		json_object_put(object_tmp);
		object_tmp = NULL;
	}
	if(object != NULL)
	{
		json_object_put(object);
		object = NULL;
	}
	if(json_song != NULL)
	{
		free(json_song);
		json_song = NULL;
	}
	/********************************************************************************************************/

	return;
}

//初始化自由模式的配置
static void init_free_config(int i)
{
	printf(".....................i:%d.............\n",i);
	unsigned char DATA[2];
  	char music_buf[8] = {};
  	char loop_buf[8] = {};
  	char volume_buf[8] = {};
  	char cursor_buf[8]={};
  	char led_buf[8] = {};

	if(!mozart_ini_getkey("/usr/data/system.ini", "free", "music", music_buf))
 		mozart_musicplayer_play_index(mozart_musicplayer_handler, atoi(music_buf));	

  	if(!mozart_ini_getkey("/usr/data/system.ini", "free", "volume", volume_buf))
  		mozart_musicplayer_set_volume(mozart_musicplayer_handler, atoi(volume_buf));

  	if(!mozart_ini_getkey("/usr/data/system.ini", "free", "loop", loop_buf))
  		mozart_musicplayer_musiclist_set_play_mode(mozart_musicplayer_handler, atoi(loop_buf));
  
  	if(i!= 0)
  	{
    	if(!mozart_ini_getkey("/usr/data/system.ini", "free", "cursor", cursor_buf))
        mozart_musicplayer_set_seek(mozart_musicplayer_handler, atoi(cursor_buf)); 
  	}
#if 0
 	if(i==0)
 	{
       	mozart_musicplayer_play_pause_pause(mozart_musicplayer_handler);
 	}
#endif

	//新添加：进入自由模式，如果有音乐正在播放，则停止当前播放
	printf("\n################################################play_pause#####################\n");
	mozart_musicplayer_play_pause_pause(mozart_musicplayer_handler);


#if 1
	if(Pack_write(Weather,(unsigned char *)weather_data,4)) //上传天气信息
	{
		return ;
	}
#endif
    if(!mozart_ini_getkey("/usr/data/system.ini", "free", "led", led_buf))    //上传灯光效果信息
    {
		if(atoi(led_buf)==0)
		{
        	DATA[0] = 0;      //DATA[0] = 0表示灯灭
		  	DATA[1] = 0;
		  	Pack_write(Light_effect,DATA,0);
		}
		else
		{
        	DATA[0] = 1;   		//DATA[0] = 1表示灯亮
		  	DATA[1] = atoi(led_buf);
		  	Pack_write(Light_effect,DATA,0);
		}
    }
	up_tower_sysc_time();   //同步时间
}

static void init_med_config(void)
{
	char volume_buf[8] = {};
  	char movie_buf[100] = {};

	if(!mozart_ini_getkey("/usr/data/system.ini", "med", "volume", volume_buf))
  		mozart_musicplayer_set_volume(mozart_musicplayer_handler, atoi(volume_buf));

	if(!mozart_ini_getkey("/usr/data/system.ini", "med", "movie", movie_buf))
    	Pack_write(Select_gif,(unsigned char *)movie_buf,strlen(movie_buf)-1);
}

/*********初始化睡眠配置*************/
static void init_sleep_config(void)
{
	int length=0; 
  	int i;
  	unsigned char DATA[2];
  	struct music_info *info = NULL;
  	char loop_buf[8] = {};
  	char volume_buf[8] = {};
  	char led_buf[8] = {};
  	char duration_buf[8] = {};
  	char music_buf[100] = {};
  	char movie_buf[100] = {};
	printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);

  	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "music", music_buf))		//根据路径播放音乐
  	{
    	length = mozart_musicplayer_musiclist_get_length(mozart_musicplayer_handler);
        if(length!=0)
	   	for (i = 0; i < length; i++) {
			
				/*****************************************info 应该free掉 *********************************************************/
				info = mozart_musicplayer_musiclist_get_index_info(mozart_musicplayer_handler, i);
				if (info != NULL&&music_buf != NULL) 
				{
                	if(strcmp(info->music_url,music_buf)==0)
                 	{
                    	mozart_musicplayer_play_index(mozart_musicplayer_handler,i);
			 			break;
                 	}
				}
		}
	   if(length!=0&&i==length)  //如果播放到尾就从头开始播放
	   		mozart_musicplayer_play_index(mozart_musicplayer_handler,0);
  	}
  printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);
	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "loop", loop_buf))  //歌曲播放的默认模式
  		mozart_musicplayer_musiclist_set_play_mode(mozart_musicplayer_handler, atoi(loop_buf));

  	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "volume", volume_buf))   //歌曲播放的默认音量
    	mozart_musicplayer_set_volume(mozart_musicplayer_handler, atoi(volume_buf));
	//助眠灯光的灯光亮度颜色模式编号："led"，整数类型，0表示不使用助眠灯光，非0则是唤醒灯光的编号，编号从1开始
  	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "led", led_buf))		
 	{
		if(atoi(led_buf)==0)
		{
        	DATA[0] = 0;
		  	DATA[1] = 0;
		  	Pack_write(Light_effect,DATA,0);   //给tower发送灯光效果
		}
		else
		{
    		DATA[0] = 1;
			DATA[1] = atoi(led_buf);
			Pack_write(Light_effect,DATA,0);
		}
    }
	printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);

	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "duration", duration_buf))
       mozart_duration = atoi(duration_buf)*60;  //助眠音乐持续时间
       
	printf("【%s】 %s %d \n",__FILE__, __func__, __LINE__);   
	if(!mozart_ini_getkey("/usr/data/system.ini", "sleep", "movie", movie_buf)){
		if(movie_buf == NULL){
			strcpy(movie_buf,"0");
			Pack_write(Select_gif,(unsigned char *)movie_buf,strlen(movie_buf)-1);  //给tower发送选择指定动画文件指令
		}
		else{
    		Pack_write(Select_gif,(unsigned char *)movie_buf,strlen(movie_buf)-1);  //给tower发送选择指定动画文件指令
		}
	}
}
//进入某种工作模式
void mozart_start_mode(enum mozart_mode mode)
{
	//char value[1000000]={};
	char *value_free = NULL;
	char *value_med = NULL;
	char *value_sleep = NULL;
	unsigned char da[2];
	int fd = -1;
	int len = -1;
	
	states_change_lock(&work_states_mutex);

  	printf("\nmode:%d  rmode:%d\n",mode,mozart_mozart_mode);  //mozart_mozart_mode是本地正在进行的功能模式
   	switch(mode){
  		case(FREE):
	        da[0]=0x01;
			da[1]=0x00;
			fd = openFile(FREE_MUSIC_PATHNAME,O_RDONLY);
			len = readLenFromFile(FREE_MUSIC_LEN_PATHNAME);
			value_free = (char *)malloc(sizeof(char)*len+1);
			memset(value_free,0,sizeof(char)*len+1);
			printf(".........................buf_len:%d\n",len);
	        switch(mozart_mozart_mode)
			{		
	        	case(FREE): //如果本地是FREE模式，就意味着从FREE进入FREE模式，什么都不做直接退出
					Pack_write(Base_station,da,2);
					break;
		       	case(MED):  //如果本地是MED模式，则停止MED模式，
					mozart_stop_mode(MED);
					mozart_mozart_mode=FREE;
	                if(-1 != readDataFromFile(fd, value_free, len))
	                {                                                   	 
	                	update_list_from_recordlist(value_free);  //加载历史列表从保存的文件夹中
	                }
					else
					{
						update_list_from_sdcard(mode);   //如果保存的文件夹内容为空，则从SD卡中加载列表
					}
	                init_free_config(1);
					Pack_write(Base_station,da,2);  //上传base工作状态数据给tower
		            break;
		       	case(SLEEP):
					mozart_stop_mode(SLEEP);
					mozart_mozart_mode=FREE;   
	               	if(-1 != readDataFromFile(fd, value_free, len))
	                {
	                	update_list_from_recordlist(value_free);
	                }
					else
					{
						update_list_from_sdcard(mode);
					}
					
					init_free_config(2);
					Pack_write(Base_station,da,2);  //上传base工作状态数据给tower
		            break;
				case(CONFIG_WIFI):
					mozart_stop_mode(CONFIG_WIFI);
					mozart_mozart_mode=FREE;
					if(-1 != readDataFromFile(fd, value_free, len))
	                {                                                   	 
	                	update_list_from_recordlist(value_free);  //加载历史列表从保存的文件夹中
	                }
					else
					{
						update_list_from_sdcard(mode);   //如果保存的文件夹内容为空，则从SD卡中加载列表
					}
					init_free_config(4);
					Pack_write(Base_station,da,2);
					break;
				case(CONFIG_BT):
					mozart_stop_mode(CONFIG_BT);
					mozart_mozart_mode=FREE;
					if(-1 != readDataFromFile(fd, value_free, len))
	                {                                                   	 
	                	update_list_from_recordlist(value_free);  //加载历史列表从保存的文件夹中
	                }
					else
					{
						update_list_from_sdcard(mode);   //如果保存的文件夹内容为空，则从SD卡中加载列表
					}
					
					init_free_config(5);
					Pack_write(Base_station,da,2);
					break;
				case(MUSIC_BT):
					mozart_stop_mode(MUSIC_BT);
					mozart_mozart_mode=FREE;
					if(-1 != readDataFromFile(fd, value_free, len))
	                {                                                   	 
	                	update_list_from_recordlist(value_free);  //加载历史列表从保存的文件夹中
	                }
					else
					{
						update_list_from_sdcard(mode);   //如果保存的文件夹内容为空，则从SD卡中加载列表
					}
					
					init_free_config(6);
					Pack_write(Base_station,da,2);
					break;
				case(MUSIC_DLNA):
					mozart_stop_mode(MUSIC_DLNA);
					mozart_mozart_mode=FREE;
					if(-1 != readDataFromFile(fd, value_free, len))
	                {                                                   	 
	                	update_list_from_recordlist(value_free);  //加载历史列表从保存的文件夹中
	                }
					else
					{
						update_list_from_sdcard(mode);   //如果保存的文件夹内容为空，则从SD卡中加载列表
					}
					
					init_free_config(7);
					Pack_write(Base_station,da,2);
					break;
				case(MUSIC_AIRPLAY):
					mozart_stop_mode(MUSIC_AIRPLAY);
					mozart_mozart_mode=FREE;
					if(-1 != readDataFromFile(fd, value_free, len))
	                {                                                   	 
	                	update_list_from_recordlist(value_free);  //加载历史列表从保存的文件夹中
	                }
					else
					{
						update_list_from_sdcard(mode);   //如果保存的文件夹内容为空，则从SD卡中加载列表
					}
					
					init_free_config(8);
					Pack_write(Base_station,da,2);
					break;
				default:
					mozart_mozart_mode=FREE;
					if(-1 != readDataFromFile(fd, value_free,len))
	                {
	                	update_list_from_recordlist(value_free);
					}
					else
					{
						update_list_from_sdcard(mode);
					}
					
					init_free_config(0);
					Pack_write(Base_station,da,2);  //上传base工作状态数据给tower
					break;		  
	        }
			mozart_ingenicplayer_notify_work_status();
	  	break;	  
	  	case(MED):
	  		da[0]=0x02;
		    da[1]=0x00;
			fd = openFile(MED_MUSIC_PATHNAME,O_RDONLY);
			len = readLenFromFile(MED_MUSIC_LEN_PATHNAME);
			printf(".........................buf_len:%d\n",len);
			value_med = (char *)malloc(sizeof(char)*len+1);
			memset(value_med,0,sizeof(char)*len+1);
            switch(mozart_mozart_mode){
            	case(FREE):
					mozart_stop_mode(FREE);
					mozart_mozart_mode=MED;
					if(-1 != readDataFromFile(fd, value_med, len))
            		{
            			update_list_from_recordlist(value_med);
            		}
					else
					{
						update_list_from_sdcard(mode);
					}
					init_med_config();
					
					Pack_write(Base_station,da,2);
					 /*语音提示：进入冥想模式，请佩戴脑波仪，正在检测脑波信号*/
					mozart_play_key("enter_med_mode");	
					break;
	            case(MED):
					Pack_write(Base_station,da,2);
					 /*语音提示：进入冥想模式，请佩戴脑波仪，正在检测脑波信号*/
					mozart_play_key("enter_med_mode");	
               		break;
	            case(SLEEP):
					mozart_stop_mode(SLEEP);
					mozart_mozart_mode=MED;
                    if(-1 != readDataFromFile(fd, value_med, len))
                    {
                    	update_list_from_recordlist(value_med);
                    }
					else
					{
						update_list_from_sdcard(mode);
					}
					init_med_config();
					
					Pack_write(Base_station,da,2);
					 /*语音提示：进入冥想模式，请佩戴脑波仪，正在检测脑波信号*/
					mozart_play_key("enter_med_mode");	
					break;
				case(CONFIG_WIFI):
					mozart_stop_mode(CONFIG_WIFI);
					mozart_mozart_mode=MED;
					if(-1 != readDataFromFile(fd, value_med, len))
            		{
            			update_list_from_recordlist(value_med);
            		}
					else
					{
						update_list_from_sdcard(mode);
					}
					init_med_config();
					
					Pack_write(Base_station,da,2);
					 /*语音提示：进入冥想模式，请佩戴脑波仪，正在检测脑波信号*/
					mozart_play_key("enter_med_mode");	
					break;
				case(CONFIG_BT):
					mozart_stop_mode(CONFIG_BT);
					mozart_mozart_mode=MED;
					if(-1 != readDataFromFile(fd, value_med, len))
            		{
            			update_list_from_recordlist(value_med);
            		}
					else
					{
						update_list_from_sdcard(mode);
					}
					init_med_config();
					
					Pack_write(Base_station,da,2);
					 /*语音提示：进入冥想模式，请佩戴脑波仪，正在检测脑波信号*/
					mozart_play_key("enter_med_mode");	
					break;	
				case(MUSIC_BT):
					mozart_stop_mode(MUSIC_BT);
					mozart_mozart_mode=MED;
					if(-1 != readDataFromFile(fd, value_med, len))
            		{
            			update_list_from_recordlist(value_med);
            		}
					else
					{
						update_list_from_sdcard(mode);
					}
					init_med_config();
					
					Pack_write(Base_station,da,2);
					 /*语音提示：进入冥想模式，请佩戴脑波仪，正在检测脑波信号*/
					mozart_play_key("enter_med_mode");	
					break;
				case(MUSIC_DLNA):
					mozart_stop_mode(MUSIC_DLNA);
					mozart_mozart_mode=MED;
					if(-1 != readDataFromFile(fd, value_med, len))
            		{
            			update_list_from_recordlist(value_med);
            		}
					else
					{
						update_list_from_sdcard(mode);
					}
					init_med_config();
					
					Pack_write(Base_station,da,2);
					 /*语音提示：进入冥想模式，请佩戴脑波仪，正在检测脑波信号*/
					mozart_play_key("enter_med_mode");	
					break;
				case(MUSIC_AIRPLAY):
					mozart_stop_mode(MUSIC_AIRPLAY);
					mozart_mozart_mode=MED;
					if(-1 != readDataFromFile(fd, value_med, len))
            		{
            			update_list_from_recordlist(value_med);
            		}
					else
					{
						update_list_from_sdcard(mode);
					}
					init_med_config();
					
					Pack_write(Base_station,da,2);
					 /*语音提示：进入冥想模式，请佩戴脑波仪，正在检测脑波信号*/
					mozart_play_key("enter_med_mode");	
					break;
				default:
					mozart_mozart_mode=MED;
					if(-1 != readDataFromFile(fd, value_med, len))
            		{
            			update_list_from_recordlist(value_med);
            		}
					else
					{
						update_list_from_sdcard(mode);
					}
					init_med_config();
					
					Pack_write(Base_station,da,2);
					mozart_play_key("enter_med_mode");	
					break;
                }	
				mozart_ingenicplayer_notify_work_status();
				break;

		case(SLEEP):			
	  		da[0]=0x03;
		    da[1]=0x00;
			fd = openFile(SLEEP_MUSIC_PATHNAME,O_RDONLY);
			len = readLenFromFile(SLEEP_MUSIC_LEN_PATHNAME);
			printf(".........................buf_len:%d\n",len);
			value_sleep = (char *)malloc(sizeof(char)*len+1);
			memset(value_sleep,0,sizeof(char)*len+1);
            switch(mozart_mozart_mode){
				
            	case(FREE):
					mozart_stop_mode(FREE);
					mozart_mozart_mode=SLEEP;
                	if(-1 != readDataFromFile(fd, value_sleep, len))
                	{
                		update_list_from_recordlist(value_sleep);
                	}
					else
					{
						update_list_from_sdcard(mode);
					}
					init_sleep_config();
					sleep_info_init();
					Pack_write(Base_station,da,2);
					break;			
				case(MED):
					mozart_stop_mode(MED);
					mozart_mozart_mode=SLEEP;
                    if(-1 != readDataFromFile(fd, value_sleep, len))
                    {
                    	update_list_from_recordlist(value_sleep);
                    }
					else
					{
						update_list_from_sdcard(mode);
					}
					init_sleep_config();
					sleep_info_init();
					
					Pack_write(Base_station,da,2);
					break;
	            case(SLEEP):
					Pack_write(Base_station,da,2);
                    break;
				case(CONFIG_WIFI):
					mozart_stop_mode(CONFIG_WIFI);
					mozart_mozart_mode=SLEEP;
					if(-1 != readDataFromFile(fd, value_sleep, len))
                	{
                		update_list_from_recordlist(value_sleep);
                	}
					else
					{
						update_list_from_sdcard(mode);
					}
					init_sleep_config();
					sleep_info_init();
					
					Pack_write(Base_station,da,2);
					break;
				case(CONFIG_BT):
					mozart_stop_mode(CONFIG_BT);
					mozart_mozart_mode=SLEEP;
					if(-1 != readDataFromFile(fd, value_sleep, len))
                	{
                		update_list_from_recordlist(value_sleep);
                	}
					else
					{
						update_list_from_sdcard(mode);
					}
					init_sleep_config();
					sleep_info_init();
					
					Pack_write(Base_station,da,2);
					break;
				case(MUSIC_BT):
					mozart_stop_mode(MUSIC_BT);
					mozart_mozart_mode=SLEEP;
					if(-1 != readDataFromFile(fd, value_sleep, len))
                	{
                		update_list_from_recordlist(value_sleep);
                	}
					else
					{
						update_list_from_sdcard(mode);
					}
					init_sleep_config();
					sleep_info_init();
					
					Pack_write(Base_station,da,2);
					break;
				case(MUSIC_DLNA):
					mozart_stop_mode(MUSIC_DLNA);
					mozart_mozart_mode=SLEEP;
					if(-1 != readDataFromFile(fd, value_sleep, len))
                	{
                		update_list_from_recordlist(value_sleep);
                	}
					else
					{
						update_list_from_sdcard(mode);
					}
					init_sleep_config();
					sleep_info_init();
					
					Pack_write(Base_station,da,2);
					break;
				case(MUSIC_AIRPLAY):
					mozart_stop_mode(MUSIC_AIRPLAY);
					mozart_mozart_mode=SLEEP;
					if(-1 != readDataFromFile(fd, value_sleep, len))
                	{
                		update_list_from_recordlist(value_sleep);
                	}
					else
					{
						update_list_from_sdcard(mode);
					}
					init_sleep_config();
					sleep_info_init();
					
					Pack_write(Base_station,da,2);
					break;
				default:
					mozart_mozart_mode=SLEEP;
					if(-1 != readDataFromFile(fd, value_sleep, len))
                	{
                		update_list_from_recordlist(value_sleep);
                	}
					else
					{
						update_list_from_sdcard(mode);
					}
					init_sleep_config();
					sleep_info_init();
					
					Pack_write(Base_station,da,2);
					break;
			}
			mozart_ingenicplayer_notify_work_status();
			//mozart_play_key("enter_sleep_mode");//语音提示：进入助眠模式	
		break;

		/*------------------------------18.6.6号-------------------------------------------------------*/
		case(CONFIG_WIFI):
			switch(mozart_mozart_mode){
				case(FREE):
					mozart_stop_mode(FREE);
					mozart_mozart_mode=CONFIG_WIFI;
					mozart_ingenicplayer_notify_work_status();
					break;
				case(MED):
					mozart_stop_mode(MED);
					mozart_mozart_mode=CONFIG_WIFI;
					mozart_ingenicplayer_notify_work_status();
					break;
				case(SLEEP):
					mozart_stop_mode(SLEEP);
					mozart_mozart_mode=CONFIG_WIFI;
					mozart_ingenicplayer_notify_work_status();
					break;
				case(CONFIG_WIFI):
					break;
				case(CONFIG_BT):
					//蓝牙配置模式不允许进入wifi配置
					break;
				case(MUSIC_BT):
					mozart_stop_mode(MUSIC_BT);
					mozart_mozart_mode=CONFIG_WIFI;
					mozart_ingenicplayer_notify_work_status();
					break;
				case(MUSIC_DLNA):
					mozart_stop_mode(MUSIC_DLNA);
					mozart_mozart_mode=CONFIG_WIFI;
					mozart_ingenicplayer_notify_work_status();
					break;
				case(MUSIC_AIRPLAY):
					mozart_stop_mode(MUSIC_AIRPLAY);
					mozart_mozart_mode=CONFIG_WIFI;
					mozart_ingenicplayer_notify_work_status();
					break;
				default:
					mozart_mozart_mode=CONFIG_WIFI;
					mozart_ingenicplayer_notify_work_status();
			}
		break;	
		case(CONFIG_BT):
			switch(mozart_mozart_mode){
				case(FREE):
					mozart_stop_mode(FREE);
					mozart_mozart_mode=CONFIG_BT;
					mozart_ingenicplayer_notify_work_status();
					break;
				case(MED):
					mozart_stop_mode(MED);
					mozart_mozart_mode=CONFIG_BT;
					mozart_ingenicplayer_notify_work_status();
					break;
				case(SLEEP):
					mozart_stop_mode(SLEEP);
					mozart_mozart_mode=CONFIG_BT;
					mozart_ingenicplayer_notify_work_status();
					break;
				case(CONFIG_WIFI):
					
					break;
				case(CONFIG_BT):
					mozart_mozart_mode=CONFIG_BT;
					mozart_ingenicplayer_notify_work_status();
					break;
				case(MUSIC_BT):
					mozart_stop_mode(MUSIC_BT);
					mozart_mozart_mode=CONFIG_BT;
					mozart_ingenicplayer_notify_work_status();
					break;
				case(MUSIC_DLNA):
					mozart_stop_mode(MUSIC_DLNA);
					mozart_mozart_mode=CONFIG_BT;
					mozart_ingenicplayer_notify_work_status();
					break;
				case(MUSIC_AIRPLAY):
					mozart_stop_mode(MUSIC_AIRPLAY);
					mozart_mozart_mode=CONFIG_BT;
					mozart_ingenicplayer_notify_work_status();
					break;
				default:
					mozart_mozart_mode=CONFIG_BT;
					mozart_ingenicplayer_notify_work_status();
			}
		break;	
		case(MUSIC_BT):
			switch(mozart_mozart_mode){
				case(FREE):
					mozart_stop_mode(FREE);
					mozart_mozart_mode=MUSIC_BT;
					mozart_snd_source_switch();
					break;
				case(MED):
					
					break;
				case(SLEEP):
					
					break;
				case(CONFIG_WIFI):
					
					break;
				case(CONFIG_BT):
					
					break;
				case(MUSIC_BT):
					
					break;
				case(MUSIC_DLNA):
					
					break;
				case(MUSIC_AIRPLAY):
					
					break;
			}
		break;	
			
		case(MUSIC_DLNA):
			switch(mozart_mozart_mode){
				case(FREE):
					mozart_stop_mode(FREE);
					mozart_mozart_mode=MUSIC_DLNA;
					
					break;
				case(MED):
					mozart_stop_mode(MED);
					mozart_mozart_mode=MUSIC_DLNA;
					
					break;
				case(SLEEP):
					mozart_stop_mode(SLEEP);
					mozart_mozart_mode=MUSIC_DLNA;
					
					break;
				case(CONFIG_WIFI):
					mozart_stop_mode(CONFIG_WIFI);
					mozart_mozart_mode=MUSIC_DLNA;
					
					break;
				case(CONFIG_BT):
					mozart_stop_mode(CONFIG_BT);
					mozart_mozart_mode=MUSIC_DLNA;
					
					break;
				case(MUSIC_BT):
					mozart_stop_mode(MUSIC_BT);
					mozart_mozart_mode=MUSIC_DLNA;
				
					break;
				case(MUSIC_DLNA):
					//do nothing
					break;
				case(MUSIC_AIRPLAY):
					mozart_stop_mode(MUSIC_AIRPLAY);
					mozart_mozart_mode=MUSIC_DLNA;
				
					break;
			}
		break;	
		case(MUSIC_AIRPLAY):
			switch(mozart_mozart_mode){
				case(FREE):
					mozart_stop_mode(FREE);
					mozart_mozart_mode=MUSIC_AIRPLAY;
					
					break;
				case(MED):
					mozart_stop_mode(MED);
					mozart_mozart_mode=MUSIC_AIRPLAY;
					
					break;
				case(SLEEP):
					mozart_stop_mode(SLEEP);
					mozart_mozart_mode=MUSIC_AIRPLAY;
					
					break;
				case(CONFIG_WIFI):
					mozart_stop_mode(CONFIG_WIFI);
					mozart_mozart_mode=MUSIC_AIRPLAY;
					
					break;
				case(CONFIG_BT):
					mozart_stop_mode(CONFIG_BT);
					mozart_mozart_mode=MUSIC_AIRPLAY;
					
					break;
				case(MUSIC_BT):
					mozart_stop_mode(MUSIC_BT);
					mozart_mozart_mode=MUSIC_AIRPLAY;
				
					break;
				case(MUSIC_DLNA):
					mozart_stop_mode(MUSIC_DLNA);
					mozart_mozart_mode=MUSIC_AIRPLAY;
				
					break;
				case(MUSIC_AIRPLAY):				
					break;
			}
		break;	
		/*-------------------------------------------------------------------------------------*/
   	}

	if(value_free != NULL)
	{
		free(value_free);
		value_free = NULL;
	}
	if(value_med != NULL)
	{
		free(value_med);
		value_med = NULL;
	}
	if(value_sleep != NULL)
	{
		free(value_sleep);
		value_sleep = NULL;
	}
	
	close(fd);
	states_change_unlock(&work_states_mutex);
}

void mozart_stop_mode(enum mozart_mode mode)
{
	time_t timep;
    int music;
    char music_buf[8]={};
    int cursor;
    char cursor_buf[8]={};
    int volume;
    char volume_buf[8]={};
	
    switch(mode){
		case(FREE):
			if(mozart_mozart_mode==FREE)
			{
		    	preserve_list(FREE);
				music=mozart_musicplayer_musiclist_get_current_index(mozart_musicplayer_handler);
				sprintf(music_buf,"%d",music);  
				mozart_ini_setkey("/usr/data/system.ini", "free", "music", music_buf);
				cursor=mozart_musicplayer_get_pos(mozart_musicplayer_handler);
				sprintf(cursor_buf,"%d",cursor);  
				mozart_ini_setkey("/usr/data/system.ini", "free", "cursor", cursor_buf);

				volume = mozart_musicplayer_get_volume(mozart_musicplayer_handler);
				sprintf(volume_buf,"%d",volume);  
				mozart_ini_setkey("/usr/data/system.ini", "free", "volume",volume_buf);	
					  
				if ( mozart_musicplayer_get_status(mozart_musicplayer_handler) != PLAYER_STOPPED)
					mozart_musicplayer_stop_playback(mozart_musicplayer_handler);  
					    
			}
			else
			{
		    	preserve_list(FREE);
			}
		break;
		case(MED):
			if(mozart_mozart_mode==MED)
		    {
		    	lifting_appliance_led(6);		
		    	preserve_list(MED);
				if(med_info.meding)
				{ 
					med_info.meding = false;
			        time(&timep);
			        sprintf(med_info.medendtime, "%d-%d-%d %d:%d:%d",localtime(&timep)->tm_year+1900,localtime(&timep)->tm_mon+1,
			                                                         localtime(&timep)->tm_mday,localtime(&timep)->tm_hour,
			                                                         localtime(&timep)->tm_min,localtime(&timep)->tm_sec);
					med_file_close();		 
				}
					
			    if (mozart_musicplayer_get_status(mozart_musicplayer_handler) != PLAYER_STOPPED)
			       	mozart_musicplayer_stop_playback(mozart_musicplayer_handler);  
				//语音提示：退出冥想模式
				mozart_play_key("exit_med_mode");
		    }
		break;  
	  	case(SLEEP):
			if(mozart_mozart_mode==SLEEP)
		    {
		    	
		    	preserve_list(SLEEP);
				if(mozart_musicplayer_get_status(mozart_musicplayer_handler) != PLAYER_STOPPED)
					mozart_musicplayer_stop_playback(mozart_musicplayer_handler);
				if(sleep_info.med_time > 300 && sleep_info.med_time == 300)
				{
					conserve_sleep_data();  //助眠数据上传（修改于2018.7.30号）
				}
		    }
	   	  	break;
		case(CONFIG_WIFI):
			break;

		case(CONFIG_BT):
		 	break;

		case(MUSIC_BT):

			break;
		case(MUSIC_DLNA):

			break;
		case(MUSIC_AIRPLAY):

			break;			
     }
	
}
int mozart_module_local_music_start(void)
{
	if (snd_source != SND_SRC_LOCALPLAY) {
		if (mozart_module_stop())
			return -1;
		if (mozart_musicplayer_start(mozart_musicplayer_handler))
			return -1;

		//mozart_ui_localplayer_plugin();
		//mozart_play_key("native_mode");
		snd_source = SND_SRC_LOCALPLAY;
	} else if (!mozart_musicplayer_is_active(mozart_musicplayer_handler)) {
		if (mozart_musicplayer_start(mozart_musicplayer_handler))
			return -1;
	} else {
		//mozart_musicplayer_musiclist_clean(mozart_musicplayer_handler);
	}

	return 0;
}

int mozart_module_local_music_startup(void)
{
	printf("%s %s %d 开启本地回调函数\n",__FILE__, __func__, __LINE__);
	
//	bool scan = true;
//	scan = mozart_localplayer_is_scanning();

	if (mozart_module_local_music_start())
		return -1;
	/******************获取SD卡里的音乐*******************************/
	new_mozart_music_list = calloc(strlen(mozart_localplayer_get_musiclist())+1, 1);
	strncpy(new_mozart_music_list,mozart_localplayer_get_musiclist(),strlen(mozart_localplayer_get_musiclist())+1);
	
	/*****************************************************************/
#if 0	
	if(!scan)
	{
		if(snd_source!=SND_SRC_BT_AVK)
        mozart_start_mode(FREE);  
	}
#endif

	return 0;
}
#endif	/* SUPPORT_LOCALPLAYER */
