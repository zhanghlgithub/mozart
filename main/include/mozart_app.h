/*
* 该文件的功能：启动及关闭所有连网、没联网的功能。
*/

#ifndef __MOZART_APP_H__
#define __MOZART_APP_H__

typedef enum app_t{
	MUSICPLAYER = 1,
	DMR,         //DLNA功能
	DMC,
	DMS,
	AIRPLAY,
	LOCALPLAYER,
	BT,
	VR,
	INGENICPLAYER,
	LAPSULE,
	MULROOM,
}app_t;

extern int start(app_t app);
extern int stop(app_t app);

/**
 * @brief  启动所有服务
 *
 * @param depend_network [in] 是否依赖网络的标志
 * -1: 启动所有服务
 *  0: 启动所有不依赖网络的服务
 *  1: 启动所有依赖网络的服务
 *
 * @return   成功返回0
 */
extern int startall(int depend_network);

/**
 * @brief  关闭所有服务
 *
 * @param depend_network [in] 是否依赖网络的标志
 * -1: 关闭所有服务
 *  0: 关闭所有不依赖网络的服务
 *  1: 关闭所有依赖网络的服务
 *
 * @return   成功返回0
 */
extern int stopall(int depend_network);

/**
 * @brief  重启所有服务
 *
 * @param depend_network [in] 是否依赖网络的标志
 * -1: 重启所有服务
 *  0: 重启所有不依赖网络的服务
 *  1: 重启所有依赖网络的服务
 *
 * @return   成功返回0
 */
extern int restartall(int depend_network);

#endif //_APP_H_
