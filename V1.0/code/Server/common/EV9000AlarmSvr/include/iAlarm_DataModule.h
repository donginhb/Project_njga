#if !defined(TOPSOCPE_IDATAMODULE_H_INCLUDED_)
#define TOPSOCPE_IDATAMODULE_H_INCLUDED_

#include <list>
#include "iIFDefine.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//���ݹ����� ģ����̳� �������ݷ����仯
class WISCOMVISION_DLL_CLASS BaseDataManager  
{
public:
	virtual void On_DI_ValueChanaged(int Index,bool oldvalue,bool newvalue);
	virtual void On_DO_ValueChanaged(int Index,bool oldvalue,bool newvalue);
	virtual void On_AI_ValueChanaged(int Index,WORD oldvalue,WORD newvalue);
	virtual void On_AO_ValueChanaged(int Index,WORD oldvalue,WORD newvalue);

	virtual void On_DI_ValueChanaging(int Index,bool oldvalue,bool newvalue,bool &bAgree);
	virtual void On_AI_ValueChanaging(int Index,WORD oldvalue,WORD newvalue,bool &bAgree);
	virtual void On_DO_ValueChanaging(int Index,bool oldvalue,bool newvalue,bool &bAgree);
	virtual void On_AO_ValueChanaging(int Index,WORD oldvalue,WORD newvalue,bool &bAgree);
protected:
	BaseDataManager();
	virtual ~BaseDataManager();
};

//���ݼ���  ģ���ڵ��� ����ʵʱ���ݲ��ṩ��ѯ�޸Ĳ���
class WISCOMVISION_DLL_CLASS CWASDataSet  
{
public:
	virtual void AddDataManager(BaseDataManager * pdm) = 0;
	virtual void RemoveDataManager(BaseDataManager * pdm) = 0;
	
	virtual int GetDICount() = 0;
	virtual int GetDOCount() = 0;
	virtual int GetAICount() = 0;
	virtual int GetAOCount() = 0;
	
	virtual bool SetDIValue(int Index,bool newValue) = 0;
	virtual bool SetDOValue(int Index,bool newValue) = 0;
	virtual bool SetAIValue(int Index,WORD newValue) = 0;
	virtual bool SetAOValue(int Index,WORD newValue) = 0;

	virtual bool TrySetDIValue(int Index,bool newValue) = 0;
	virtual bool TrySetAIValue(int Index,WORD newValue) = 0;
	virtual bool TrySetDOValue(int Index,bool newValue) = 0;
	virtual bool TrySetAOValue(int Index,WORD newValue) = 0;
	
	virtual bool GetDIValue(int Index,bool &Value) = 0;
	virtual bool GetDOValue(int Index,bool &Value) = 0;
	virtual bool GetAIValue(int Index,WORD &Value) = 0;
	virtual bool GetAOValue(int Index,WORD &Value) = 0;
protected:
	CWASDataSet(){ }
	~CWASDataSet(){	}
};


WISCOMVISION_DLL_API BOOL __stdcall WSA_Data_Init(int DI_Count,int DO_Count,int AI_Count,int AO_Count);	//��������ʼ�����ݼ�
WISCOMVISION_DLL_API void __stdcall WSA_Data_FInit();														//����ʼ��
WISCOMVISION_DLL_API CWASDataSet* __stdcall WSA_Data_GetDataSet();										//��ȡ���ݼ�ָ��

#endif