#if !defined(WA4ISM_ICOMTCPMODBUS_H_INCLUDED_)
#define WA4ISM_ICOMTCPMODBUS_H_INCLUDED_

#ifndef WA4ISM_DLL_CLASS
#define WA4ISM_DLL_CLASS __declspec(dllexport)
#else
#define WA4ISM_DLL_CLASS __declspec(dllimport)
#endif

#ifndef WA4ISM_DLL_API
#define WA4ISM_DLL_API extern "C" __declspec(dllexport)
#else
#define WA4ISM_DLL_API extern "C" __declspec(dllimport)
#endif
#include <list>

typedef unsigned int	uint;
typedef unsigned short	ushort;
typedef void*			lpvoid;

//错误码
enum WA4ISM_ERROR{
	WA4ISM_ERROR_NOERROR = 0,		//无错误
	WA4ISM_ERROR_NOINIT,			//未初始化
	WA4ISM_ERROR_REINIT,			//重复初始化
	WA4ISM_ERROR_DATABLOCK,			//创建数据内存块错误
	WA4ISM_ERROR_SEATTYPE,			//错误的点位类型
	WA4ISM_ERROR_SEATINDEX,			//错误的点位索引
	WA4ISM_ERROR_NODEVICE,			//不存在的被控设备
	WA4ISM_ERROR_OFFDEVICE,			//被控设备不在线
	WA4ISM_ERROR_CTRLBUFFULL,		//控制命令缓存满
	WA4ISM_ERROR_UNKNOW
};

//点位类型
enum WA4ISM_SEATTYPE
{
	WA4ISM_SEATTYPE_DI = 0,
	WA4ISM_SEATTYPE_DO,
	WA4ISM_SEATTYPE_AI,
	WA4ISM_SEATTYPE_AO
};

struct MBDEVCFG 
{
	char chIp[20];				//设备IP地址
	short sPort;				//设备端口
	uint uDevAddr;				//设备总线地址
	uint uDevOnOff;				//设备在线状态点位
	uint uDiOffset;				//di偏移量	相对客户端本地数据实例
	uint uDiCount;				//di数量
	uint uDoOffset;				//do偏移量	相对客户端本地数据实例
	uint uDoCount;				//do数量
	uint uAiOffset;				//ai偏移量	相对客户端本地数据实例
	uint uAiCount;				//ai数量
	uint uAoOffset;				//ao偏移量	相对客户端本地数据实例
	uint uAoCount;				//ao数量
};

/**
 *  点位状态变化回调函数
 *  @type 点位类型
 *  @index 点位索引
 *  @oldValue 原值
 *  @newValue 新值
 *  @return
**/
typedef int (CALLBACK* LPUSERSTATUSCALLBACK)(WA4ISM_SEATTYPE type, uint index , ushort oldValue , ushort newValue,void* lpUserData);

/**
 *  初始化
 *  @return 
**/
WA4ISM_DLL_API WA4ISM_ERROR __stdcall WA4ISM_Init(uint uDiCount,uint uDoCount,uint uAiCount,uint uAoCount);

/**
 *  反初始化
 *  @return 
**/
WA4ISM_DLL_API void __stdcall WA4ISM_FInit();

/**
 *  添加需要监视的设备
 *  @sMbCfg	设备配置
 *  @return 
**/
WA4ISM_DLL_API WA4ISM_ERROR __stdcall WA4ISM_AddDevice(MBDEVCFG sMbCfg);

/**
 *  设置点位状态回调
 *	@lpCallBack 回调函数
 *  @lpUserData 用户数据
 *  @return
**/
WA4ISM_DLL_API WA4ISM_ERROR __stdcall WA4ISM_SetSeatStatusCallback(LPUSERSTATUSCALLBACK lpCallBack,lpvoid lpUserData);

/**
 *  获取点位状态
 *  @type 点位类型
 *  @index 点位索引
 *  @Value 值（输出）
**/
WA4ISM_DLL_API WA4ISM_ERROR __stdcall WA4ISM_GetSeatStatus(WA4ISM_SEATTYPE type, uint index , ushort& Value);

/**
 *  尝试设置点位状态（控制）
 *  @type 点位类型
 *  @index 点位索引
 *  @Value 值
**/
WA4ISM_DLL_API WA4ISM_ERROR __stdcall WA4ISM_TrySetSeatStatus(WA4ISM_SEATTYPE type, uint index , ushort Value);



#endif