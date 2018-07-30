#ifndef __MOZART_MODULE_BT_H__
#define __MOZART_MODULE_BT_H__

#include <inttypes.h>
#include "resample_interface.h"
#include "channels_interface.h"
#include "bluetooth_interface.h"

/* A2DP Source */
#define SUPPORT_BSA_A2DP_SOURCE			0
/* A2DP Synk */
#define SUPPORT_BSA_A2DP_SYNK			1

/* HFP AG */
#define SUPPORT_BSA_HFP_AG			0
/* HFP HF */
#define SUPPORT_BSA_HFP_HF			0

/* phone book server */
#define SUPPORT_BSA_PBS				0
/* phone book client */
#define SUPPORT_BSA_PBC				0

/* OPP Server */
#define SUPPORT_BSA_OPS				0
/* OPP Client */
#define SUPPORT_BSA_OPC				0

/* SPP Server */
#define SUPPORT_BSA_SPPS			0
/* SPP Client */
#define SUPPORT_BSA_SPPC			0

/* BSA BLE SUPPORT */
#define SUPPORT_BSA_BLE				1
#define SUPPORT_BSA_BLE_SERVER			0
#define SUPPORT_BSA_BLE_CLIENT			1

/* BSA BLE HID SUPPORT */
#define SUPPORT_BSA_BLE_HH			0

/* BLE HID DIALOG DEVICE */
#define SUPPORT_BSA_BLE_HH_DIALOG_AUDIO		0

/* Automatic Echo Cancellation RESAMPLE SUPPORT */
#define SUPPORT_AEC_RESAMPLE			0
#define SUPPORT_AEC_RESAMPLE_48K_TO_8K		0


#if (SUPPORT_BSA_HFP_HF == 1)

typedef struct {
	int  in_len;
	char *in_buf;
	char *out_buf;
	int  out_len;
	int  max_outlen;
	af_resample_t *resample_t;
} hs_resample_info;

typedef struct {
	int  in_len;
	char *in_buf;
	char *out_buf;
	int  out_len;
	int  max_outlen;
	af_channels_t *channel_t;
} hs_rechannel_info;

typedef struct {
	hs_resample_info  res;
	hs_rechannel_info rec;
} hs_resample_param;

hs_resample_param hs_bt;
hs_resample_param hs_record;

#if (SUPPORT_WEBRTC == 1)
typedef struct {
	unsigned int outlen;
	char *outbuf;
	hs_resample_param aec;
} hs_aec_param;

hs_resample_param hs_ref;
hs_aec_param hs_aec;
#endif /* SUPPORT_WEBRTC */
#endif /* SUPPORT_BSA_HFP_HF */

struct BLE_INFO
{
	UINT16              conn_id;
	BD_ADDR             bda;
	tBTA_GATTC_CHAR_ID  char_id;
	tBTA_GATT_ID            descr_type;
	UINT16              len;
	UINT8               value[BSA_BLE_MAX_ATTR_LEN];
	BOOLEAN             is_notify;
};
struct brain_parameter {
	
			unsigned char delta[3];
			unsigned char theta[3];
			unsigned char low_alpha[3];	
			unsigned char high_alpha[3];	
			unsigned char low_beta[3];	
			unsigned char high_beta[3];	
			unsigned char low_gamma[3];	
			unsigned char high_gamma[3];	

	};
	
struct motion {
	
			unsigned char x_angle[2];
			unsigned char y_angle[2];
			unsigned char z_angle[2];
			
			unsigned char x_acc;
			unsigned char y_acc;
			unsigned char z_acc;
			
			unsigned char x_angle_acc;
			unsigned char y_angle_acc;
			unsigned char z_angle_acc;
	

	};

struct info {
   unsigned char   X_brain_quality;			//信号质量(0x02)
   unsigned char   X_brain_concentration;	//专注度参数(0x04)
   unsigned char   X_brain_attention;		//注意力参数(0x05)
   struct brain_parameter	brain_parameter;//频谱参数(0x83)	
		
    unsigned char   X_blink;				//眨眼数据(0x16)
    unsigned char   X_power;				//X 的电量数据(0x20)
    unsigned char   X_brain[2];				//原始脑电数据(0x80)
    struct motion 	motion;					//X 的运动姿态数据(0x90)								
    unsigned char   X_model[20];			//X 的设备型号(0x91)
    unsigned char   X_number[20];			//X 的设备序列号数据(0x92)
    unsigned char   X_binnumber[20];		//X 的固件版本号数据(0x93) 
    unsigned char   X_BLEnumber[20];		//X 的 BLE 设备号数据(0x94)

    unsigned char   Tower_request;			//数据请求指令 (0x21)
    int   Tower_power;			//Tower的电量数据(0x22)
    int   Tower_if_power;        //tower是否在充电
    
    //unsigned char   Tower_brain_power;	//原始脑电数据(0x80)
    
    
    unsigned char   Tower_WIFI_Name[20];	//当前存储的WIFI的SSID (0xA1)   
    unsigned char   Tower_WIFI_Password[20];//当前存储的 WIFI 的密码(0xA2)
    unsigned char   Tower_type[20];			//Tower的设备型号数据(0xA3)
    unsigned char   Tower_number[20];		//Tower的设备序列号数据(0xA4)
    unsigned char   Tower_binnumber[20];	//Tower的固件版本号数据(0xA5)
    unsigned char   Tower_BLEnumber[20];	//Tower的 BLE 设备号数据(0xA6)
    int   Tower_sd_capacity;				//Tower的 SD 卡容量数据(0xA8)
    int   Tower_sd_rest;					//Tower的 SD 卡剩余容量数据(0xA9)
    unsigned char   Tower_gif_state[20];	//Tower的动画播放状态(0xAA)       
    unsigned char   Tower_lighting_state[3];//Tower的灯光状态数据(0xAB)
    unsigned char   Tower_upgrade_state[2];	//Tower的升级状态数据(0xAC)
    unsigned char   Tower_WIFI_state[10];	//Tower的WIFI连接状态信息(0xAD)      
    unsigned char   Tower_clock_setting[20];//Tower的时钟设置状态数据(0xAF)

	//新添加的
	int X_charge;   //x的电池状态，1：正在充电，0：未充电
	unsigned char Tower_blever[20];	//ble固件版本号
	unsigned char Tower_os[20];		//tower操作系统版本号
	unsigned char Tower_MAC[20];   	//MAC地址(已完成)   
	unsigned char Tower_wlanIp[20];  //wlan的IP
	unsigned char Tower_Beiyong[20];  //备用协议（0xd9）
	
};

enum command{
	X_brain,						//脑电信号共36字节
	X_brain_concentration,			//专注度参数(0x04)，数据长度为2，总长度6，需要date变量1字节
	X_brain_attention,				//注意力参数(0x05)，数据长度为2，总长度6，需要date变量1字节
	X_brain_parameter,				//频谱参数(0x83)，，数据长度为26，总长度30，需要date变量24字节
	
	REQUEST_Name_passwd,			//请求发送 Tower 当前配置的 SSID 的名称和密码0x01 
	REQUEST_Tower_type,				//请求发送 Tower 设备型号0x03 ，数据长度为2，总长度6
	REQUEST_Tower_number,			//请求发送 Tower 设备序列号0x04 ，数据长度为2，总长度6
	REQUEST_Tower_binnumber,		//请求发送 Tower 固件版本号0x05 ，数据长度为2，总长度6
	REQUEST_Tower_BLEnumber,		//请求发送 BLE 设备号0x06，数据长度为2，总长度6
	REQUEST_Tower_sd,				//请求发送 SD 卡信息0x08，数据长度为2，总长度6
	REQUEST_Tower_gif_state,		//请求发送动画播放状态0x10，数据长度为2，总长度6
	REQUEST_Tower_lighting_state,	//请求发送灯光状态0x11，数据长度为2，总长度6
	REQUEST_Tower_WIFI_state,		//请求发送 WIFI 连接状态0x12，数据长度为2，总长度6
	Tower_on,						//Tower 的 WIFI 打开指令(0x22)，数据长度为2，总长度6
	Tower_off,						//Tower 的 WIFI 关闭指令(0x22)，数据长度为2，总长度6
	Get_gif_list,					//获取动画文件列表(0x23)，数据长度为2，总长度6
	Get_light_list,
	Control_gif,					//动画播放控制指令(0x25)，数据长度为2，总长度6，需要date变量(1字节)
	Check_update,					//检查升级包指令(0x26),数据长度为2，总长度6
	Reset,							//恢复出厂配置指令(0x27),数据长度为2，总长度6
	Power_on,						//开机指令(0x28) date[0]=0x01打开,数据长度为2，总长度6
	Power_off,						//关机指令(0x28) date[0]=0x00关闭,数据长度为2，总长度6
	
	motion,							//运动姿态数据(0x90)，数据长度为14，总长度18，需要date变量12字节
	Wifi_name,						//WIFI 的 SSID (0x91)，数据长度为length+2，总长度length+6，需要date变量length字节
	Wifi_passwd,					//WIFI 的密码(0x92)，数据长度为length+2，总长度length+6，需要date变量length字节
	Sync_time,						//时钟同步数据(0x93)，数据长度为length+2，总长度length+6，需要date变量length字节（length固定为6）
	Weather,						//天气数据(0x94)，数据长度为length+2，总长度length+6，需要date变量length字节（length固定为4）
	Light_effect,					//灯光效果指令(0x95)，数据长度为length+2，总长度length+6，需要date变量length字节（length固定为2）
	Light_intensity,				//灯光亮度设置指令(0x96)，数据长度为length+2，总长度length+6，需要date变量length字节（length固定为1）
	Base_station,					//Base工作状态数据(0x97)，数据长度为length+2，总长度length+6，需要date变量length字节（length固定为10）
	Select_gif,						//选择指定动画文件指令(0x98)，数据长度为length+2，总长度length+6，需要date变量length字节
    Beiyong,
};

extern struct info *info;
extern char  light_list[100][13];
extern char  gif_list[100][13];
extern unsigned char g_Tower_binnumber[20];		//新添加于2018.7.30号
extern int start_bt(void);
extern int stop_bt(void);
extern struct info  * get_meddata_form_ble();
extern int  Pack_write(enum command command,unsigned char *date,int length);
extern int  New_Pack_write(enum command command,unsigned char *date,int length);
extern int ble_client_write(unsigned char *data);
extern int ble_config();
extern int  get_cuband_state();
extern int  get_tower_state();
extern int  ble_client_reconnect2();
extern void avk_start();


#endif /* __MOZART_MODULE_BT_H__ */

