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
* LPLisenceStatusCallback	��Ȩ״̬�ص�								*
* bIsGranted				�Ƿ��Ѿ���Ȩ								*
/************************************************************************/
typedef int(CALLBACK *LPLisenceStatusCallback)(bool bGranted);

/************************************************************************
* WV_LISENCE_Init	��ʼ��SDK                                           *
* ����ֵ		�ɹ�����true ʧ�ܷ��� false                             *
************************************************************************/
IWV_LISENCE_API bool __stdcall WV_LISENCE_Init();

/************************************************************************
* WV_LISENCE_SetStatusCallback	����״̬�仯�ص�����                    *
* scbfun					�仯�ص�����                                *
* ����ֵ					�ɹ�����true ʧ�ܷ��� false                 *
************************************************************************/
IWV_LISENCE_API bool __stdcall WV_LISENCE_SetStatusCallback(LPLisenceStatusCallback scbfun);

/************************************************************************
* WV_LISENCE_Start	��ʼ��̨��֤��Ȩ�Ϸ���                              *
* ����ֵ		�ɹ�����true ʧ�ܷ��� false                             *
************************************************************************/
IWV_LISENCE_API bool __stdcall WV_LISENCE_Start();

/************************************************************************
* WV_LISENCE_Stop	ֹͣ��̨��֤��Ȩ�Ϸ���                              *
************************************************************************/
IWV_LISENCE_API void __stdcall WV_LISENCE_Stop();

/************************************************************************
* WV_LISENCE_IsGranted	�ж��Ƿ��Ѿ���Ȩ                                *
************************************************************************/
IWV_LISENCE_API bool __stdcall WV_LISENCE_IsGranted(bool bRealTime = true);

/************************************************************************
* WV_LISENCE_GetConf	��ȡ��������									*
************************************************************************/
IWV_LISENCE_API bool __stdcall WV_LISENCE_GetConf(WV_APP_TYPE AppType,APP_CONF &conf);

/************************************************************************
* WV_LISENCE_FInit	����ʼ��SDK                                         *
************************************************************************/
IWV_LISENCE_API void __stdcall WV_LISENCE_FInit();

#endif
