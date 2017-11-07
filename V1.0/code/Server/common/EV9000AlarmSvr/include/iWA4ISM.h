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

//������
enum WA4ISM_ERROR{
	WA4ISM_ERROR_NOERROR = 0,		//�޴���
	WA4ISM_ERROR_NOINIT,			//δ��ʼ��
	WA4ISM_ERROR_REINIT,			//�ظ���ʼ��
	WA4ISM_ERROR_DATABLOCK,			//���������ڴ�����
	WA4ISM_ERROR_SEATTYPE,			//����ĵ�λ����
	WA4ISM_ERROR_SEATINDEX,			//����ĵ�λ����
	WA4ISM_ERROR_NODEVICE,			//�����ڵı����豸
	WA4ISM_ERROR_OFFDEVICE,			//�����豸������
	WA4ISM_ERROR_CTRLBUFFULL,		//�����������
	WA4ISM_ERROR_UNKNOW
};

//��λ����
enum WA4ISM_SEATTYPE
{
	WA4ISM_SEATTYPE_DI = 0,
	WA4ISM_SEATTYPE_DO,
	WA4ISM_SEATTYPE_AI,
	WA4ISM_SEATTYPE_AO
};

struct MBDEVCFG 
{
	char chIp[20];				//�豸IP��ַ
	short sPort;				//�豸�˿�
	uint uDevAddr;				//�豸���ߵ�ַ
	uint uDevOnOff;				//�豸����״̬��λ
	uint uDiOffset;				//diƫ����	��Կͻ��˱�������ʵ��
	uint uDiCount;				//di����
	uint uDoOffset;				//doƫ����	��Կͻ��˱�������ʵ��
	uint uDoCount;				//do����
	uint uAiOffset;				//aiƫ����	��Կͻ��˱�������ʵ��
	uint uAiCount;				//ai����
	uint uAoOffset;				//aoƫ����	��Կͻ��˱�������ʵ��
	uint uAoCount;				//ao����
};

/**
 *  ��λ״̬�仯�ص�����
 *  @type ��λ����
 *  @index ��λ����
 *  @oldValue ԭֵ
 *  @newValue ��ֵ
 *  @return
**/
typedef int (CALLBACK* LPUSERSTATUSCALLBACK)(WA4ISM_SEATTYPE type, uint index , ushort oldValue , ushort newValue,void* lpUserData);

/**
 *  ��ʼ��
 *  @return 
**/
WA4ISM_DLL_API WA4ISM_ERROR __stdcall WA4ISM_Init(uint uDiCount,uint uDoCount,uint uAiCount,uint uAoCount);

/**
 *  ����ʼ��
 *  @return 
**/
WA4ISM_DLL_API void __stdcall WA4ISM_FInit();

/**
 *  �����Ҫ���ӵ��豸
 *  @sMbCfg	�豸����
 *  @return 
**/
WA4ISM_DLL_API WA4ISM_ERROR __stdcall WA4ISM_AddDevice(MBDEVCFG sMbCfg);

/**
 *  ���õ�λ״̬�ص�
 *	@lpCallBack �ص�����
 *  @lpUserData �û�����
 *  @return
**/
WA4ISM_DLL_API WA4ISM_ERROR __stdcall WA4ISM_SetSeatStatusCallback(LPUSERSTATUSCALLBACK lpCallBack,lpvoid lpUserData);

/**
 *  ��ȡ��λ״̬
 *  @type ��λ����
 *  @index ��λ����
 *  @Value ֵ�������
**/
WA4ISM_DLL_API WA4ISM_ERROR __stdcall WA4ISM_GetSeatStatus(WA4ISM_SEATTYPE type, uint index , ushort& Value);

/**
 *  �������õ�λ״̬�����ƣ�
 *  @type ��λ����
 *  @index ��λ����
 *  @Value ֵ
**/
WA4ISM_DLL_API WA4ISM_ERROR __stdcall WA4ISM_TrySetSeatStatus(WA4ISM_SEATTYPE type, uint index , ushort Value);



#endif