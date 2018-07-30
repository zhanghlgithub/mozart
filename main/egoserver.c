#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>  
#include <errno.h>
#include <curl/curl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>

#include "ini_interface.h"
#include "json-c/json.h"
#include "localplayer_interface.h"
#include "mozart_musicplayer.h"
#include "updater_interface.h"
#include "utils_interface.h"

#include "ingenicplayer.h"
#include "egoserver.h"

#include "modules/mozart_module_local_music.h"
#include "modules/mozart_module_bt.h"

#include "battery_capacity.h"	//新添加于2018.7.25号

//bool conserver_med_data_flag = false;
static pthread_t ego_wifi_led_thread;
static pthread_t ego_test_thread;
static pthread_t ego_med_server_thread;
static pthread_mutex_t ego_med_data_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ego_med_data_cound = PTHREAD_COND_INITIALIZER;
static pthread_t ego_sleep_server_thread;
static pthread_mutex_t ego_sleep_data_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ego_sleep_data_cound = PTHREAD_COND_INITIALIZER;
FILE *fp_B;
FILE *fp_R;
FILE *fp_M;
char up_B[100];
char up_R[100];
char up_M[100];
int wifi_status=0;  //1：(没有连接网络)，2:（连接网络，但是没有连接互联网）3：连接上了互联网
struct VER new_ver;
struct WEATHER weather = {1,"",0,0,0,0,0,0,""};
unsigned char weather_data[4] = {};

//功能：把cmd_json下的object下的内容转换成int返回出来
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

//功能：把cmd_json下的object下的内容转换成字符串返回出来
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

/*
static size_t write_data_activeego(void *ptr, size_t size, size_t nmemb, void *stream)
{
if (strlen((char *)stream) + strlen((char *)ptr) > 999999) return 0;
printf("------------%s-----------------�\\n",(char *)ptr);
return size*nmemb;
}
*/
int activeego()        //把初始化的版本信息传到服务器
{
	struct info *info;
    char bsn_buf[20] = {};
    char arg[200]={};
    CURL *curl;   
    CURLcode res;  
    json_object *reply_object = json_object_new_object();
    char *reply_json = NULL;	 

    info = get_meddata_form_ble();	 
    curl = curl_easy_init(); 
    curl_easy_setopt(curl,CURLOPT_URL,"http://myego.applinzi.com/Home/EgoBase/ActiveEgo");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_activeego);	
    if(!mozart_ini_getkey("/usr/data/system.ini", "base", "sn", bsn_buf))
    json_object_object_add(reply_object, "bsn", json_object_new_string(bsn_buf));

	json_object_object_add(reply_object, "tsn", json_object_new_string((char *)info->Tower_number));
	json_object_object_add(reply_object, "xsn", json_object_new_string((char *)info->X_number));
	reply_json = strdup(json_object_get_string(reply_object));  //将打包好的json转换成字符串赋给reply_json

	sprintf(arg,"arg=%s",reply_json);
	printf("\n%s\n",arg);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,arg);

	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl); 
	if(res==0)	
		return 1;
	else
	  	return 0;
}

int binduserId()
{
     char bsn_buf[20] = {};
     char uid_buf[20] = {};
     char arg[200]={}; 	 
     CURL *curl;   
     CURLcode res;  
     json_object *reply_object = json_object_new_object();
     char *reply_json = NULL;
	 
     curl = curl_easy_init(); 
     curl_easy_setopt(curl,CURLOPT_URL,"http://myego.applinzi.com/Home/EgoBase/BindUserId");
     curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
  	 //   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);	
  
    if(!mozart_ini_getkey("/usr/data/system.ini", "base", "sn", bsn_buf))
        json_object_object_add(reply_object, "bsn", json_object_new_string(bsn_buf));
    if(!mozart_ini_getkey("/usr/data/system.ini", "base", "uid", uid_buf))
        json_object_object_add(reply_object, "uid", json_object_new_string(uid_buf));
	reply_json = strdup(json_object_get_string(reply_object));

	sprintf(arg,"arg=%s",reply_json);
	printf("\n%s\n",arg);

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS,arg);
	 res = curl_easy_perform(curl);
	
	 curl_easy_cleanup(curl); 
	  if(res==0)
	  	return 1;
	  else
	  	return 0;

}

//回调函数
static size_t write_data_getnewver(void *ptr, size_t size, size_t nmemb, void *stream)
{
	int res;
   	nvinfo_t *nvinfo = NULL;	
   	char *data = NULL;
// 	char *ceshi="http://dsc199202.top/";
// 	char *test_ver = "1.0.0";
    if (strlen((char *)stream) + strlen((char *)ptr) > 999999) return 0;

	new_ver.ver=NULL;
	new_ver.time=NULL;
	new_ver.url=NULL;
	new_ver.desc=NULL;
	new_ver.force=-1;

	res=get_object_int( "res",(char *)ptr);
	if(res)
	{
		 data=get_object_string( "ver", NULL,(char *)ptr); 
		 if(data!=NULL)
		 	{
                 new_ver.ver =  get_object_string( "ver", NULL,data); 
	             new_ver.time =  get_object_string( "time", NULL,data); 
	             new_ver.url =  get_object_string( "url", NULL,data); 
	             new_ver.desc =  get_object_string( "update_desc", NULL,data); 
	             new_ver.force =  get_object_int( "force",data); 
		 	}
            printf("\nver:%s.%s.%s.%s.%d\n",new_ver.ver,new_ver.time,new_ver.url,new_ver.desc,new_ver.force);
	        nvinfo=mozart_nv_get_nvinfo();  //获取当前版本信息
	        printf("\n=======ver:%s\n",nvinfo->current_version);
	    
		 if(new_ver.url != NULL)
		 	{
	   	 	   printf("\n+++++++++++%s+++++++++\n",new_ver.url);
		 	   memset(nvinfo->url,0,4096);
			   
		//	   memset(nvinfo->current_version,0,18);
		//	   memcpy(nvinfo->current_version,test_ver,strlen(test_ver));

			   memcpy(nvinfo->url,new_ver.url,strlen(new_ver.url));
        //	   memcpy(nvinfo->url,ceshi,strlen(ceshi));
			   printf("\n+++++++++++%s+++++++++\n",nvinfo->url);			
			   mozart_nv_set_nvinfo	(nvinfo);	//设置版本信息到本地。
			   
		//	   nvinfo=mozart_nv_get_nvinfo();  //获取当前版本信息
		//	   printf("\n=======ver:%s\n",nvinfo->current_version);
		 }

		if(new_ver.force == 1)	
		{
			if ((nvinfo = mozart_updater_chkver()) != NULL)
			{
				free(nvinfo);
				printf("\n=================update======================\n");
				if(capacity > 80)	//新添加于2018.7.19号
				{
					mozart_system("updater -p");
				}
		  	}
			else
			{
				printf("\n=================chkver error======================\n");
			}
		}
	}
	  else
 	printf("\n new cer res = 0");
return size*nmemb;
}

//获取当前版本的版本信息
int getnewversioninfo()
{
	char ver_buf[20] = {};
    char arg[200]={}; 	 
    CURL *curl;   
    CURLcode res;  
    json_object *reply_object = json_object_new_object(); //申请一个json格式的对象
    char *reply_json = NULL;
	 
    curl = curl_easy_init(); 
    curl_easy_setopt(curl,CURLOPT_URL,"http://myego.applinzi.com/Home/EgoBase/GetNewVersionInfo");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);  //设置传输延时时间是10毫秒？？
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_getnewver);	//回调机制还不是很清楚？？？？

    json_object_object_add(reply_object, "ty", json_object_new_int(1));
    if(!mozart_ini_getkey("/usr/data/system.ini", "base", "ver", ver_buf))   //获得base的当前版本
    json_object_object_add(reply_object, "ver", json_object_new_string(ver_buf));//("V0.0"));

	reply_json = strdup(json_object_get_string(reply_object));

	sprintf(arg,"arg=%s",reply_json);
	printf("\n%s\n",arg);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,arg);   //将当前的版本信息传给服务器
	res = curl_easy_perform(curl);    //主要看返回值，返回0表示curl_easy_setopt一切OK

	curl_easy_cleanup(curl);
	
	/********************2018.7.5号**********************************/
#if 0
	if(reply_json)
	{
		free(reply_json);
		reply_json = NULL;
	}
#endif
	/****************************************************************/

	if(res==0)
		return 1;
	else
	  	return 0;
}


int updateversion(char *sn,char *ver,char *uid)
{
	char arg[200]={}; 
    CURL *curl;   
    CURLcode res;  
    json_object *reply_object = json_object_new_object();
    char *reply_json = NULL;
	 
    curl = curl_easy_init(); 
    curl_easy_setopt(curl,CURLOPT_URL,"http://myego.applinzi.com/Home/EgoBase/UpdateVersion");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_activeego);		

	json_object_object_add(reply_object, "ty", json_object_new_int(1));
	json_object_object_add(reply_object, "sn", json_object_new_string(sn));
	json_object_object_add(reply_object, "ver", json_object_new_string(ver)); //打包的是新的用户的版本号
	json_object_object_add(reply_object, "uid", json_object_new_string(uid));

	reply_json = strdup(json_object_get_string(reply_object));

	sprintf(arg,"arg=%s",reply_json);
	printf("\n%s\n",arg);
	
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,arg);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl); 
	
	/**********************2018.7.5**********************************/
	
	/***************************************************************/
	if(res==0)	
		return 1;
	else
	 	return 0;
}

//冥想数据上传服务器
void med_data_up()
{
	DIR *dir;  
    struct dirent *ptr; 
    char url[100];
    CURL *curl;     
	struct curl_httppost *post;
   	struct curl_httppost *last;
   	CURLcode res;  

 	if((dir=opendir("/mnt/sdcard/data")) == NULL)  
    {  
        perror("\nOpen dir error...");  
		return;
    }  
   	while ((ptr=readdir(dir)) != NULL)  
    {  
    	if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    
        	continue;  
        else if(ptr->d_type == 8)    
        {  
           //printf("\n%s\n",ptr->d_name);  
			curl = curl_easy_init();
		    post = NULL;
		    last = NULL;
            memset(url, 0, 100);
	        sprintf(url, "/mnt/sdcard/data/%s",ptr->d_name);
	//      printf("\nurl:%s",url);		  
				  
            curl_easy_setopt(curl,CURLOPT_URL,"http://myego.applinzi.com/Home/EgoBase/UploadMedData");
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
		    curl_formadd(&post, &last, CURLFORM_COPYNAME, "name", CURLFORM_COPYCONTENTS, ptr->d_name, CURLFORM_END);
	        curl_formadd(&post, &last, CURLFORM_COPYNAME, "image",CURLFORM_FILE,url, CURLFORM_END);
         	curl_formadd(&post, &last, CURLFORM_COPYNAME, "submit", CURLFORM_COPYCONTENTS, "Submit",   CURLFORM_END);
            curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
			res=curl_easy_perform(curl);  //函数curl_easy_perform()de 功能是将数据curl传到服务器。
	//     	printf("res:%d",res);
            if(res==0)
				unlink(url);	   	
	//		printf("\nres:%d",res);
            curl_easy_cleanup(curl); 
       	}  
  
        else   
        {  
        	continue;  
        }  
    }  
    closedir(dir);  
}

//睡眠数据上传至服务器
void sleep_data_up()
{
	DIR *dir;  
    struct dirent *ptr; 
    char url[100];
    CURL *curl;     
   	struct curl_httppost *post;
   	struct curl_httppost *last;
   	CURLcode res;  
 	if ((dir=opendir("/mnt/sdcard/data1")) == NULL)  
    { 
    	perror("\nOpen dir error...");  
		return;
    }  
   	while ((ptr=readdir(dir)) != NULL)  
    {  
    	if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    
            continue;  
        else if(ptr->d_type == 8)    
        {  
 		 //    printf("\n%s\n",ptr->d_name);  
	   		curl = curl_easy_init();
		    post = NULL;
		    last = NULL;
            memset(url, 0, 100);
	        sprintf(url, "/mnt/sdcard/data1/%s",ptr->d_name);
	//      printf("\nurl:%s",url);		  
			curl_easy_setopt(curl,CURLOPT_URL,"http://myego.applinzi.com/Home/EgoBase/UploadSleepData");
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	      	curl_formadd(&post, &last, CURLFORM_COPYNAME, "name", CURLFORM_COPYCONTENTS, ptr->d_name, CURLFORM_END);
            curl_formadd(&post, &last, CURLFORM_COPYNAME, "image",CURLFORM_FILE,url, CURLFORM_END);
   	      	curl_formadd(&post, &last, CURLFORM_COPYNAME, "submit", CURLFORM_COPYCONTENTS, "Submit",   CURLFORM_END);
	        curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
			res=curl_easy_perform(curl); 
//          printf("res:%d",res);
            if(res==0)
				unlink(url);	   	
//			printf("\nres:%d",res);
            curl_easy_cleanup(curl); 
       	}  
  
        else   
        {  
        	continue;  
        }  
    }  
   closedir(dir);  
}

//功能：打开保存冥想数据的文件夹
void med_file_open(void)
{
	if((fp_B=fopen(med_info.name_B,"wb+"))==NULL)
	{
    	printf("fopen fp_B error");
	}
	if((fp_R=fopen(med_info.name_R,"wb+"))==NULL)
    {
    	printf("fopen fp_R error");
	}
	if((fp_M=fopen(med_info.name_M,"wb+"))==NULL)
    {
    	printf("fopen fp_M error");
	}
}

//功能：将冥想数据保存到文件夹中！
static void conserve_med_data(void)
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
    	json_object_object_add(reply_object, "bsn", json_object_new_string(bsn_buf));
  	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "ver", bver_buf))
    	json_object_object_add(reply_object, "bver", json_object_new_string(bver_buf));
   	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "uid", uid_buf))
    	json_object_object_add(reply_object, "uid", json_object_new_string(uid_buf));
  
 	json_object_object_add(reply_object, "xsn", json_object_new_string((char *)info->X_number));
 	json_object_object_add(reply_object, "xver", json_object_new_string((char *)info->X_binnumber));
 	json_object_object_add(reply_object, "tsn", json_object_new_string((char *)info->Tower_number));
 	json_object_object_add(reply_object, "tver", json_object_new_string((char *)info->Tower_binnumber));
 	json_object_object_add(reply_object, "tpver", json_object_new_string(""));
 	json_object_object_add(reply_object, "fname", json_object_new_string(""));
 	json_object_object_add(reply_object, "ttype", json_object_new_int(1));

 	json_object_object_add(reply_object, "stime", json_object_new_string(med_info.medstarttime));
 	json_object_object_add(reply_object, "etime", json_object_new_string(med_info.medendtime));

	json_object_object_add(reply_object, "mid", json_object_new_int(med_info.mid));
 	json_object_object_add(reply_object, "mid3rd", json_object_new_int(med_info.mid3rd));
 	json_object_object_add(reply_object, "cid", json_object_new_int(med_info.cid));
 	json_object_object_add(reply_object, "mname", json_object_new_string(med_info.mname));
 	json_object_object_add(reply_object, "mfrom", json_object_new_int(med_info.mfrom));

 	json_object_object_add(reply_object, "dur", json_object_new_int(med_info.med_time));

	for(i=0;i<=med_info.med_time;i++)
	{   
		json_object *med_object = json_object_new_object();
		
		json_object_object_add(med_object, "a", json_object_new_int(med_info.zhuanzhu[i]));	
		json_object_object_add(med_object, "m", json_object_new_int(med_info.fangsong[i]));	
		json_object_object_add(med_object, "p", json_object_new_int(0));	
		json_object_array_add(data_object, med_object);
		
	}
	json_object_object_add(reply_object, "aindex", json_object_new_int(100));
	json_object_object_add(reply_object, "mindex", json_object_new_int(100));
	json_object_object_add(reply_object, "score", json_object_new_int(100));
	json_object_object_add(reply_object, "rank", json_object_new_int(1));

	json_object_object_add(reply_object, "data",data_object);
 	reply_json = strdup(json_object_get_string(reply_object));

	fwrite(reply_json,1,strlen(reply_json),fp_M);

	json_object_put(reply_object);
       free(reply_json);	
}

void med_file_close(void)
{
	if(med_info.med_time>=5)
    {
    	if(med_info.med_time>=30)
        conserve_med_data();
	  	fclose(fp_B);
	  	fclose(fp_R);
	  	fclose(fp_M);

        if(med_info.med_time>=30)
        {
         	memset(up_B, 0, 100);
			memset(up_R, 0, 100);
			memset(up_M, 0, 100);
			strncpy(up_B,med_info.name_B,strlen(med_info.name_B));
			strncpy(up_R,med_info.name_R,strlen(med_info.name_R));
			strncpy(up_M,med_info.name_M,strlen(med_info.name_M));
	        pthread_cond_signal(&ego_med_data_cound);
        }
	  	else
		{
        	unlink(med_info.name_B);	
	        unlink(med_info.name_R);	
		 	unlink(med_info.name_M);	
		}
    }
}

//功能：判断闹钟列表里是否有编号为ID的闹钟，如果有返回该ID闹钟的信息，否则返回NULL。
static char *get_alarmid(int id)
{
	int x;
    int length=0;
    char *alarm_list = NULL;
    json_object *tmp = NULL;
    json_object *tmp_id = NULL;
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
			if (json_object_object_get_ex(tmp, "id", &tmp_id))
			{										
				if(id==json_object_get_int(tmp_id))
				{
					return (char *)json_object_get_string(tmp);		 	
				}
			}               
	}
	/*************************修改于2018.6.25********************************************/
	if(object != NULL)
	{
		json_object_put(object);
		object = NULL;
	}
	/***********************************************************************************/

	return NULL;

}

//功能：保存助眠数据到文件
int conserve_sleep_data(void)
{
	FILE *fp;
  	struct info *info;
  	char bsn_buf[20] = {};
  	char bver_buf[20] = {};
  	char uid_buf[20] = {};
  	json_object *reply_object = json_object_new_object();
  	char *reply_json = NULL;

  	if((fp=fopen(sleep_info.name,"wb+"))==NULL)
    {
    	printf("fopen fp_sleep error\n");
		return -1;
	}

    info = get_meddata_form_ble();
   
  	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "sn", bsn_buf))
    	json_object_object_add(reply_object, "bsn", json_object_new_string(bsn_buf));
  	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "ver", bver_buf))
    	json_object_object_add(reply_object, "bver", json_object_new_string(bver_buf));
   	if(!mozart_ini_getkey("/usr/data/system.ini", "base", "uid", uid_buf))
    	json_object_object_add(reply_object, "uid", json_object_new_string(uid_buf));
  
 	json_object_object_add(reply_object, "xsn", json_object_new_string((char *)info->X_number));
 	json_object_object_add(reply_object, "xver", json_object_new_string((char *)info->X_binnumber));
 	json_object_object_add(reply_object, "tsn", json_object_new_string((char *)info->Tower_number));
 	json_object_object_add(reply_object, "tver", json_object_new_string((char *)info->Tower_binnumber));
 	json_object_object_add(reply_object, "tpver", json_object_new_string(""));
 	json_object_object_add(reply_object, "fname", json_object_new_string(""));
 	json_object_object_add(reply_object, "ttype", json_object_new_int(2));

 	json_object_object_add(reply_object, "stime", json_object_new_string(sleep_info.sleepstarttime));
 	json_object_object_add(reply_object, "etime", json_object_new_string(sleep_info.sleependtime));

	json_object_object_add(reply_object, "mid", json_object_new_int(sleep_info.mid));
 	json_object_object_add(reply_object, "mid3rd", json_object_new_int(sleep_info.mid3rd));
 	json_object_object_add(reply_object, "cid", json_object_new_int(sleep_info.cid));
 	json_object_object_add(reply_object, "mname", json_object_new_string(sleep_info.mname));
 	json_object_object_add(reply_object, "mfrom", json_object_new_int(sleep_info.mfrom));

	printf("sleep_info.alarm_id :%d\n",sleep_info.alarm_id);
  	if(sleep_info.alarm_id==0)
  	{
    	json_object_object_add(reply_object, "snooze", json_object_new_int(0));
	 	json_object_object_add(reply_object, "alarm", json_object_new_string(""));
  	}
  	else
  	{
        json_object_object_add(reply_object, "snooze", json_object_new_int(sleep_info.snooze));
        json_object_object_add(reply_object, "alarm", json_object_new_string(get_alarmid(sleep_info.alarm_id)));
  	}
	if(json_object_get_string(reply_object) != NULL){
  		reply_json = strdup(json_object_get_string(reply_object));
     	printf("this is 助眠数据上传。。。。\n");  
  		fwrite(reply_json,1,strlen(reply_json),fp);
		free(reply_json);
	}

  	json_object_put(reply_object);
  		   
  	fclose(fp);
  	pthread_cond_signal(&ego_sleep_data_cound);
	return 0;
}

//功能：记录悬浮日志；TOWER上升到最顶端、降到最底端或发生偏移脱落时，需要向服务器发数据
int suspensionlog(int cmd)
{
     char bsn_buf[20] = {};
     char uid_buf[20] = {};
     char ver_buf[20]={};	 
     char arg[200]={}; 
     CURL *curl;   
     CURLcode res;  
      json_object *reply_object = json_object_new_object();
     char *reply_json = NULL;
	 
     curl = curl_easy_init(); 
     curl_easy_setopt(curl,CURLOPT_URL,"http://myego.applinzi.com/Home/EgoBase/Suspensionlog");
     curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_activeego);		

	json_object_object_add(reply_object, "cmd", json_object_new_int(cmd));
    if(!mozart_ini_getkey("/usr/data/system.ini", "base", "sn", bsn_buf))
        json_object_object_add(reply_object, "bsn", json_object_new_string(bsn_buf));
    if(!mozart_ini_getkey("/usr/data/system.ini", "base", "uid", uid_buf))
        json_object_object_add(reply_object, "uid", json_object_new_string(uid_buf));
    if(!mozart_ini_getkey("/usr/data/system.ini", "base", "ver", ver_buf))
        json_object_object_add(reply_object, "ver", json_object_new_string(ver_buf));

	reply_json = strdup(json_object_get_string(reply_object));

	sprintf(arg,"arg=%s",reply_json);
	printf("\n%s\n",arg);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS,arg);
	 res = curl_easy_perform(curl);
	 curl_easy_cleanup(curl); 
         if(res==0)	
	  	return 1;
	  else
	  	return 0;
}

//功能：BASE播放音乐结束或停止时，向服务器提交播放记录
int syncmusicdata()          
{
     int index = -1;
     int time = -1;
     int duration = 0;		 
     struct music_info *info = NULL;	 
     char bsn_buf[20] = {};
     char uid_buf[20] = {};
     char ver_buf[20]={};	 
     char arg[2000]={}; 
     CURL *curl;   
     CURLcode res;  
     json_object *reply_object = json_object_new_object();
     char *reply_json = NULL;
	 
     curl = curl_easy_init(); //初始化一个curl会话
     curl_easy_setopt(curl,CURLOPT_URL,"http://myego.applinzi.com/Home/EgoBase/SyncMusicData");	//设置响应的回话选项
     curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);   //最多传输十秒。
    // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_activeego);		
 
   	index = mozart_musicplayer_musiclist_get_current_index(mozart_musicplayer_handler);
	json_object_object_add(reply_object, "mid", json_object_new_int(index));
	info = mozart_musicplayer_musiclist_get_current_info(mozart_musicplayer_handler);
	if (info) 
    {
		time = mozart_musicplayer_get_duration(mozart_musicplayer_handler);	
		json_object_object_add(reply_object, "duration", json_object_new_int(time));
		duration = mozart_musicplayer_get_pos(mozart_musicplayer_handler);
		json_object_object_add(reply_object, "mposition", json_object_new_int(duration));
    }
	
    json_object_object_add(reply_object, "bsn", json_object_new_string(bsn_buf));
    if(!mozart_ini_getkey("/usr/data/system.ini", "base", "uid", uid_buf))
        json_object_object_add(reply_object, "uid", json_object_new_string(uid_buf));
    if(!mozart_ini_getkey("/usr/data/system.ini", "base", "ver", ver_buf))
        json_object_object_add(reply_object, "ver", json_object_new_string(ver_buf));
	
	reply_json = strdup(json_object_get_string(reply_object));

	sprintf(arg,"arg=%s",reply_json);
	printf("\n%s\n",arg);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,arg);    //将arg的内容提交到服务器。
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);  //返回0表示上传成功，非零表示错误。
	if(res==0)	
	  	return 1;
	else
	  	return 0;
}

//服务器返回的天气信息
static size_t write_data_getweather(void *ptr, size_t size, size_t nmemb, void *stream)
{
    if (strlen((char *)stream) + strlen((char *)ptr) > 999999) return 0;
	weather.res = 1;
    weather.city = NULL;
    weather.temp = 0;
	weather.wind = 0;   
    weather.weather = 0;
    weather.day_url = 0;
	weather.night_url = 0;  
	weather.windLevel = 0;
	weather.date = NULL;
    weather.res =  get_object_int( "res",(char *)ptr); 
	weather.city =  get_object_string( "city", NULL,(char *)ptr); 
	weather.temp =  get_object_int( "temperature",(char *)ptr); 
	weather.wind =  get_object_int( "wind",(char *)ptr); 
	weather.weather =  get_object_int( "weather",(char *)ptr); 
	weather.day_url =  get_object_int( "dayPictureUrl",(char *)ptr); 
	weather.night_url =  get_object_int( "nightPictureUrl",(char *)ptr); 
	weather.windLevel = get_object_int("windLevel",(char *)ptr);
	weather.date =  get_object_string( "date", NULL,(char *)ptr); 

	weather_data[0] = weather.weather;
	weather_data[1] = weather.wind;
	weather_data[2] = weather.windLevel;
	weather_data[3] = weather.temp;
	Pack_write(Weather,(unsigned char *)weather_data,4); 

    printf("\nweather:%s.%d.%d.%s\n",weather.city,weather.temp,weather.weather,weather.date);	
	return size*nmemb;
}

int getweather()
{
    char city_buf[20] = {};
    char arg[2000]={}; 	 
    CURL *curl;   
    CURLcode res;  
    json_object *reply_object = json_object_new_object();
    char *reply_json = NULL;
	 
    curl = curl_easy_init(); 
    curl_easy_setopt(curl,CURLOPT_URL,"http://myego.applinzi.com/Home/EgoBase/GetWeather");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_getweather);	

	if(!mozart_ini_getkey("/usr/data/system.ini", "med", "city", city_buf))
        json_object_object_add(reply_object, "cid", json_object_new_string(city_buf));

	reply_json = strdup(json_object_get_string(reply_object));

	sprintf(arg,"arg=%s",reply_json);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,arg);
	res = curl_easy_perform(curl);

	curl_easy_cleanup(curl); 
	if(res==0)	
		return 1;
	else
	  	return 0;
}

/*每隔1小时监测一次天气变化和版本信息*/
static void *ego_test(void *arg)  
{
	while(1){
		getnewversioninfo();
	   	getweather();
	   	sleep(3600);   //睡眠一小时
   	}
  	return NULL;
}

/*30秒钟检查一次网络状况*/
static void *ego_wifi_led(void *arg)
{
	int i;
  	while(1)
  	{
        i=mozart_system("ping baidu.com -q -c1 -w1");	//如果ping通了返回值为0
      	printf(">>>>>>>>>>>>>>>>>I:%d\n",i);
		if(i==0)
	   		wifi_status=3;
		
	 	else 
	 	{
	 		//添加于2018.7.26号
	 		if(wifi_status==3 || wifi_status==2)
	 		{
				wifi_status=2;
			}
	        else
	        {
				wifi_status = 1;
			}
	 	}
		printf(">>>>>>>>>>>>>>>>>>>>>wifi_status: %d>>>>>>>>>>>>>>>>\n",wifi_status);
        if(wifi_status== 0||wifi_status==1)
        {      	     
        	system("echo timer > /sys/class/leds/led-wifi2/trigger");
            system("echo 0 > /sys/class/leds/led-wifi2/delay_on");	
            system("echo 0 > /sys/class/leds/led-wifi2/delay_off");
		    system("echo 1 > /sys/class/leds/led-wifi2/brightness");	//控制灯灭
        }
	 	else if(wifi_status==2)
	 	{
        	system("echo 1 > /sys/class/leds/led-wifi2/brightness");	//控制灯灭
            system("echo timer > /sys/class/leds/led-wifi2/trigger");	
            system("echo 500 > /sys/class/leds/led-wifi2/delay_on");	//每500ms
            system("echo 500 > /sys/class/leds/led-wifi2/delay_off");
	 	}
	 	else if(wifi_status== 3)
	 	{
        	system("echo timer > /sys/class/leds/led-wifi2/trigger");
            system("echo 0 > /sys/class/leds/led-wifi2/delay_on");	
            system("echo 0 > /sys/class/leds/led-wifi2/delay_off");
		 	system("echo 0 > /sys/class/leds/led-wifi2/brightness");
	 	}
	 	sleep(30);
  	}
	return NULL;
}

//网络连接正常的情况下，如果有冥想数据保存到文件，则上传服务器
static void *ego_med_server(void *arg)
{
	CURL *curl;     
   	struct curl_httppost *post;
   	struct curl_httppost *last;
   	CURLcode res;  
    while(1)
    {
    	pthread_mutex_lock(&ego_med_data_mutex);
		pthread_cond_wait(&ego_med_data_cound,&ego_med_data_mutex);
		
/***********************up M***********************************/
      	curl = curl_easy_init();
		post = NULL;
		last = NULL;

        curl_easy_setopt(curl,CURLOPT_URL,"http://myego.applinzi.com/Home/EgoBase/UploadMedData");
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	    curl_formadd(&post, &last, CURLFORM_COPYNAME, "name", CURLFORM_COPYCONTENTS, up_M+17, CURLFORM_END);
	    curl_formadd(&post, &last, CURLFORM_COPYNAME, "image",CURLFORM_FILE, up_M, CURLFORM_END);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "submit", CURLFORM_COPYCONTENTS, "Submit",   CURLFORM_END);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
		res=curl_easy_perform(curl); 
        printf("res:%d",res);
        if(res==0)
		unlink(up_M);	   	
		//printf("\nres:%d",res);
        curl_easy_cleanup(curl); 

/*****************************up B**************************/
		
      	curl = curl_easy_init();
		post = NULL;
		last = NULL;
        curl_easy_setopt(curl,CURLOPT_URL,"http://myego.applinzi.com/Home/EgoBase/UploadMedData");
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
		curl_formadd(&post, &last, CURLFORM_COPYNAME, "name", CURLFORM_COPYCONTENTS, up_B+17, CURLFORM_END);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "image",CURLFORM_FILE, up_B, CURLFORM_END);
       	curl_formadd(&post, &last, CURLFORM_COPYNAME, "submit", CURLFORM_COPYCONTENTS, "Submit",   CURLFORM_END);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
		res=curl_easy_perform(curl); 
        if(res==0)
			unlink(up_B);
		//printf("\nres:%d",res);
        curl_easy_cleanup(curl); 

 /************************up R***********************************/

      	curl = curl_easy_init();
		post = NULL;
		last = NULL;
        curl_easy_setopt(curl,CURLOPT_URL,"http://myego.applinzi.com/Home/EgoBase/UploadMedData");
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "name", CURLFORM_COPYCONTENTS,up_R+17, CURLFORM_END);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "image",CURLFORM_FILE,up_R, CURLFORM_END);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "submit", CURLFORM_COPYCONTENTS, "Submit",   CURLFORM_END);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
        res=curl_easy_perform(curl); 
        if(res==0)
	 	unlink(up_R);	   	
      //printf("\nres:%d",res);
        curl_easy_cleanup(curl); 	 
        pthread_mutex_unlock(&ego_med_data_mutex);

    }
	return NULL;
}

static void *ego_sleep_server(void *arg)
{
	CURL *curl;     
    struct curl_httppost *post;
    struct curl_httppost *last;
   	CURLcode res;  
   	while(1)
    {
    	pthread_mutex_lock(&ego_sleep_data_mutex);
		pthread_cond_wait(&ego_sleep_data_cound,&ego_sleep_data_mutex);
		
/****************************up S***************************************/

		curl = curl_easy_init();
		post = NULL;
		last = NULL;
		printf("开始上传助眠数据\n");
		//curl_easy_setopt(curl,CURLOPT_URL,"172.21.59.184/Home/EgoBase/UploadSleepData");
		curl_easy_setopt(curl,CURLOPT_URL,"http://myego.applinzi.com/Home/EgoBase/UploadSleepData");
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
		//将助眠的文件夹上传到服务器
    	curl_formadd(&post, &last, CURLFORM_COPYNAME, "name", CURLFORM_COPYCONTENTS, sleep_info.name+17, CURLFORM_END);
    	curl_formadd(&post, &last, CURLFORM_COPYNAME, "image",CURLFORM_FILE, sleep_info.name, CURLFORM_END);
    	curl_formadd(&post, &last, CURLFORM_COPYNAME, "submit", CURLFORM_COPYCONTENTS, "Submit",   CURLFORM_END);
    	curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
		res=curl_easy_perform(curl); 
        printf("res:%d",res);
        if(res==0)
		unlink(sleep_info.name);	   	
        //printf("\nres:%d",res);
        curl_easy_cleanup(curl); 
        pthread_mutex_unlock(&ego_sleep_data_mutex);
   	}
	return NULL;
}

void egoserverstart()
{
	char uid_buf[20] = {};
    system("mkdir -p /mnt/sdcard/data");
	system("mkdir -p /mnt/sdcard/data1");
	   
    if(!mozart_ini_getkey("/usr/data/system.ini", "base", "uid", uid_buf))
  	if(strcmp(uid_buf,"0")==0)  
  	{
    	activeego();  
  	}
#if 0
	updateversion("b20180115","V1.8","2");
	suspensionlog(21);
	syncmusicdata();
#endif
	if(0 != pthread_create(&ego_test_thread, NULL,ego_test, NULL))  //每隔一小时检测一下天气和版本信息。
		printf("Can't create ego test!\n");
		
	if(0 != pthread_create(&ego_wifi_led_thread, NULL,ego_wifi_led, NULL))	//每隔30秒钟查看一下网络状态
		printf("Can't create ego wifi led !\n");
		
	if(0 != pthread_create(&ego_med_server_thread, NULL,ego_med_server, NULL))  //冥想数据上传服务器
		printf("Can't create ego med server!\n");

    if(0 != pthread_create(&ego_sleep_server_thread, NULL,ego_sleep_server, NULL))	//助眠数据上传服务器
		printf("Can't create ego sleep server!\n");
}
