#if !defined(__IWV_LISENCE_MODULE__)
#define __IWV_LISENCE_MODULE__

#include "iwv_dog_constdef.h"

#ifdef _WIN32	//win32
#include <windows.h>
#ifndef IWV_LISENCE_API
#define IWV_LISENCE_API extern "C" __declspec(dllexport)
#else
#define IWV_LISENCE_API extern "C" __declspec(dllimport)
#endif
#elif defined(__linux__)	//linux
#define __stdcall
#define CALLBACK
#define IWV_LISENCE_API extern "C"
#endif

/************************************************************************
* LPLisenceStatusCallback	授权状态回调								*
* bIsGranted				是否已经授权								*
/************************************************************************/
typedef int(CALLBACK *LPLisenceStatusCallback)(bool bGranted);

/************************************************************************
* WV_LISENCE_Init	初始化SDK                                           *
* 返回值		成功返回true 失败返回 false                             *
************************************************************************/
IWV_LISENCE_API bool __stdcall WV_LISENCE_Init();

/************************************************************************
* WV_LISENCE_SetStatusCallback	设置状态变化回调函数                    *
* scbfun					变化回调函数                                *
* 返回值					成功返回true 失败返回 false                 *
************************************************************************/
IWV_LISENCE_API bool __stdcall WV_LISENCE_SetStatusCallback(LPLisenceStatusCallback scbfun);

/************************************************************************
* WV_LISENCE_Start	开始后台验证授权合法性                              *
* 返回值		成功返回true 失败返回 false                             *
************************************************************************/
IWV_LISENCE_API bool __stdcall WV_LISENCE_Start();

/************************************************************************
* WV_LISENCE_Stop	停止后台验证授权合法性                              *
************************************************************************/
IWV_LISENCE_API void __stdcall WV_LISENCE_Stop();

/************************************************************************
* WV_LISENCE_IsGranted	判断是否已经授权                                *
************************************************************************/
IWV_LISENCE_API bool __stdcall WV_LISENCE_IsGranted(bool bRealTime = true);

/************************************************************************
* WV_LISENCE_GetConf	获取配置数据									*
************************************************************************/
IWV_LISENCE_API bool __stdcall WV_LISENCE_GetConf(WV_APP_TYPE AppType,APP_CONF &conf);

/************************************************************************
* WV_LISENCE_FInit	反初始化SDK                                         *
************************************************************************/
IWV_LISENCE_API void __stdcall WV_LISENCE_FInit();

#endif
