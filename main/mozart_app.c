#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>

#include "utils_interface.h"
#include "sharememory_interface.h"
#include "wifi_interface.h"
#include "localplayer_interface.h"
#include "egoserver.h"

#include "mozart_config.h"
#include "lapsule_control.h"
#include "ingenicplayer.h"
#if (SUPPORT_DMS == 1)
#include "dms.h"
#endif

#include "mozart_app.h"
#include "mozart_key_function.h"
#include "mozart_musicplayer.h"

#ifdef SUPPORT_BT
#include "modules/mozart_module_bt.h"
#endif
#ifdef SUPPORT_VR
#include "modules/mozart_module_vr.h"
#endif
#if (SUPPORT_DMR == 1)
#include "modules/mozart_module_dmr.h"
#endif
#include "modules/mozart_module_airplay.h"
#include "modules/mozart_module_cloud_music.h"
#include "modules/mozart_module_local_music.h"
#if (SUPPORT_MULROOM == 1)
#include "mulroom_interface.h"
#endif

#if (SUPPORT_DMS == 1)
bool dms_started = false;
#endif

#if (SUPPORT_AIRPLAY == 1)
bool airplay_started = false;
#endif

#if (SUPPORT_LOCALPLAYER == 1)
bool localplayer_started = false;
#endif

#ifdef SUPPORT_BT
bool bt_started = false;
#endif

#ifdef SUPPORT_VR
bool vr_started = false;
#endif

#if (SUPPORT_INGENICPLAYER == 1)
bool ingenicplayer_started = false;
#endif

bool ego_started = false;

__attribute__((unused)) static bool lapsule_started = false;

int start(app_t app)
{
	switch(app){
	case MUSICPLAYER:
		mozart_musicplayer_startup();   //开启音乐播放功能
		break;
#if (SUPPORT_DMR == 1)
	case DMR:
		if (dmr_is_started())
			mozart_render_refresh();
		else
			dmr_start();
		break;
#endif
#if (SUPPORT_DMS == 1)
	case DMS:
		if (dms_started) {
			stop(app);
			dms_started = false;
		}

		if (start_dms()) {
           	printf("ushare service start failed.\n");
			return -1;
		}

		dms_started = true;
		return 0;
#endif
	case DMC:
		break;
#if (SUPPORT_AIRPLAY == 1)
	case AIRPLAY:
		if (airplay_started) {
			stop(app);
			airplay_started = false;
		}

		mozart_airplay_start_service();

		airplay_started = true;
		return 0;
#endif

#if (SUPPORT_LOCALPLAYER == 1)
	case LOCALPLAYER:
		if (localplayer_started) {
			stop(app);
			localplayer_started = false;
		}

		if (mozart_localplayer_startup()) {
			return -1;
		}

		localplayer_started = true;
		return 0;
#endif
#ifdef SUPPORT_BT
	case BT:
		if (bt_started) {
			stop(app);
			bt_started = false;
		}

		start_bt(); //开启蓝牙设备
		bt_started = true;
		return 0;
#endif

#ifdef SUPPORT_VR
	case VR:
		if (vr_started) {
			stop(app);
			vr_started = false;
		}

		if (mozart_vr_init(process_vr_callback)) {
			printf("init error, vr disabled!!\n");
			return -1;
		}

		if (mozart_vr_start()) {
			printf("start error, vr disabled!!\n");
			mozart_vr_uninit();
			return -1;
		}

		vr_started = true;
		break;
#endif

#if (SUPPORT_INGENICPLAYER == 1)
	case INGENICPLAYER:
		if (ingenicplayer_started) {  //如果app功能打开了  ，就关闭
			stop(app);
			ingenicplayer_started = false;
		}
		if (mozart_ingenicplayer_startup()) {
			printf("start ingenicplayer failed in %s: %s.\n", __func__, strerror(errno));
			return -1;
		}
		ingenicplayer_started = true;
		break;
#endif

#if (SUPPORT_LAPSULE == 1)
	case LAPSULE:
		if (lapsule_started) {
			stop(app);
			lapsule_started = false;
		}

        if (start_lapsule_app()) {
        	printf("start lapsule services failed in %s: %s.\n", __func__, strerror(errno));
			return -1;
        }

		lapsule_started = true;
		return 0;
#endif

#if (SUPPORT_MULROOM == 1)
	case MULROOM:
		break;
#endif


	default:
		printf("WARNING: Not support this module(id = %d)\n", app);
		break;
	}

	return 0;
}

int stop(app_t app)
{
	switch(app){
	case MUSICPLAYER:
		mozart_musicplayer_shutdown();
		break;
#if (SUPPORT_DMR == 1)
	case DMR:
		mozart_render_terminate_fast();
		break;
#endif
#if (SUPPORT_DMS == 1)
	case DMS:
		if (dms_started) {
			stop_dms();
			dms_started = false;
		}
		break;
#endif
	case DMC:
		break;
#if (SUPPORT_AIRPLAY == 1)
	case AIRPLAY:
		if (airplay_started) {
			mozart_airplay_shutdown();
			airplay_started = false;
		}
		break;
#endif
#if (SUPPORT_LOCALPLAYER == 1)
	case LOCALPLAYER:
		if (localplayer_started) {
			printf("%s %s %d 关闭本地播放\n",__FILE__, __func__, __LINE__);
		//	mozart_mozart_mode =0;
			mozart_localplayer_shutdown();
			localplayer_started = false;
		}
		break;
#endif
#ifdef SUPPORT_BT
	case BT:
		if (bt_started) {
			stop_bt();
			bt_started = false;
		}
		break;
#endif
#ifdef SUPPORT_VR
	case VR:
		mozart_vr_stop();
		mozart_vr_uninit();

		vr_started = false;
		break;
#endif

#if (SUPPORT_INGENICPLAYER == 1)
	case INGENICPLAYER:
		if (ingenicplayer_started) {
			mozart_ingenicplayer_shutdown();
			ingenicplayer_started = false;
		}
		break;
#endif

#if (SUPPORT_LAPSULE == 1)
	case LAPSULE:
		if (lapsule_started) {
			stop_lapsule_app();
			lapsule_started = false;
		}
		break;
#endif

#if (SUPPORT_MULROOM == 1)
	case MULROOM:
		mozart_mulroom_dismiss_group();
		break;
#endif
	default:
		printf("WARNING: Not support this module(id = %d)\n", app);
		break;
	}

	return 0;
}


int startall(int depend_network)
{

#ifdef SUPPORT_VR
	wifi_info_t infor = get_wifi_mode();
#endif
	if (depend_network == -1 || depend_network == 1) 
	{
	#if (SUPPORT_DMR == 1)
		start(DMR);
	#endif
	
	#if (SUPPORT_DMS == 1)
	//	start(DMS);
	#endif
	//	start(DMC);

	#if (SUPPORT_AIRPLAY == 1)
	//	start(AIRPLAY);
	#endif

	#ifdef SUPPORT_VR
		if (infor.wifi_mode == STA){
			printf("\nno vr ");
		//start(VR);
		}
	#endif
	
	#if (SUPPORT_LAPSULE == 1)   //此功能没有使用
		start(LAPSULE);
	#endif

	#if (SUPPORT_INGENICPLAYER == 1)	//启动与手机 APP 通讯服务
		start(INGENICPLAYER);
		mozart_appserver_register_callback(1 * 1024 * 1024, appserver_cmd_callback);	//接受app发来的内容，存入链表
	#endif
/*
	#if (SUPPORT_LOCALPLAYER == 1)
		if (snd_source != SND_SRC_LOCALPLAY) {
			if (get_wifi_mode().wifi_mode == STA)
				mozart_module_cloud_music_startup();
		} else {
			printf("Playing local music, will not play cloud music.\n");
		}
	#else
		if (get_wifi_mode().wifi_mode == STA)
			mozart_module_cloud_music_startup();
	#endif
*/

	  	if(!ego_started)
	  	{
	    	egoserverstart();   //开启与服务器交互功能，将冥想数据提交到服务器
            ego_started = true;
	  	}

	}

	if (depend_network == -1 || depend_network == 0) {
		start(MUSICPLAYER);
#ifdef SUPPORT_BT
       	start(BT);
#endif
#if (SUPPORT_LOCALPLAYER == 1)
		start(LOCALPLAYER);
#endif
	}

	return 0;
}

int stopall(int depend_network)
{
	if (depend_network == -1 || depend_network == 1) {
#if (SUPPORT_DMR == 1)
	//	stop(DMR);
#endif
#if (SUPPORT_DMS == 1)
	//	stop(DMS);
#endif
#if (SUPPORT_AIRPLAY == 1)
		stop(AIRPLAY);
#endif
#if 1
#ifdef SUPPORT_VR
		stop(VR);
#endif
#endif
#if (SUPPORT_INGENICPLAYER == 1)
		stop(INGENICPLAYER);
		mozart_appserver_shutdown();
#endif

	}

	if (depend_network == -1 || depend_network == 0) {
#ifdef SUPPORT_BT
		stop(BT);
#endif
#if (SUPPORT_LOCALPLAYER == 1)
		stop(LOCALPLAYER);
#endif
		stop(MUSICPLAYER);
	}

#if (SUPPORT_MULROOM == 1)
	if (depend_network == 1)
		stop(MULROOM);
#endif

	return 0;
}

int restartall(int depend_network)
{
	stopall(depend_network);
	startall(depend_network);

	return 0;
}
