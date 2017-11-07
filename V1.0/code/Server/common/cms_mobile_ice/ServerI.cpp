// WorkQueue.cpp: implementation of the WorkQueue class.
//
//////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include <Ice/Ice.h>
#include "ServerI.h"
#include <iostream>
#include "MsgOp.h"
using namespace std;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#ifndef WIN32
extern AMI_ICE_HA_Client* pAMI_ICE_HA_Client ;
extern int m_HA_send_recv;
#endif

bool PrintfSQLToFile_server(const char* c_sql)
{
return true;
char sql[1000]={0};
    strcpy(sql,c_sql);
    //strcat(sql,getFormateCurrentTime("%H:%M:%S"));
    strcat(sql,"\n");
    
    FILE *fd;

    if ((fd = fopen("/app/HAFile_Server", "r+b")) == NULL)
    //if ((fd = fopen(m_FileName, "r+b")) == NULL)
    {
        return false;
    }

    fseek(fd,0,SEEK_END);


    unsigned int nLen = strlen(sql);
    fwrite(sql, sizeof(char), nLen, fd);

    fclose(fd);

    return true;
}

//-----以下为任务处理实现-------
WorkQueue::WorkQueue():
_bIsRun(false)
{
}

WorkQueue::~WorkQueue()
{
}

void WorkQueue::run()
{
	_bIsRun = true;
    while(_bIsRun)
    {
		bool bHaveJob=false; //是否有任务处理
		CallbackEntry entry;
		if (true)
		{
			IceUtil::Monitor<IceUtil::Mutex>::Lock lock(_monitor);
			if(_callbacks.size() > 0)
			{
				entry = _callbacks.front();
				_callbacks.pop_front();
				bHaveJob = true;
				cout<<"开始异步数据处理" << endl; 
				cout<<"等待处理任务长度:"<<_callbacks.size()<<endl;
				cout<<"-------------------------"<<endl;
			}
		}
		
		if (bHaveJob)
		{
			MsgOp::DealMsg(entry.cb,entry.tInCommMsg,entry.sInData);	
		}else{
#ifdef WIN32
			Sleep(1000);  
#else
			sleep(1);
#endif
			continue;
		}		
    }
	
    //
    // Throw exception for any outstanding requests.
    //
//     list<CallbackEntry>::const_iterator p;
//     for(p = _callbacks.begin(); p != _callbacks.end(); ++p)
//     {
//         (*p).cb->ice_exception(Demo::RequestCanceledException());
//     }
}

void WorkQueue::add(const AMD_EV9000MSI_OMMPToCMSInfoPtr& cb, 
					const ::Ice::Int nSendType, 
					const COMMNMSGHEAD& tInCommMsg, 
					const BYTEBUFFER& sInData)
{
    IceUtil::Monitor<IceUtil::Mutex>::Lock lock(_monitor);
    if(_bIsRun)
    {
        //push到任务队列
        CallbackEntry entry;
        entry.cb = cb;
		entry.nSendType = nSendType;
		entry.tInCommMsg = tInCommMsg;
		entry.sInData = sInData;
        _callbacks.push_back(entry);
		cout<<"add异步处理任务到队列中，队列长度:"<<_callbacks.size()<<endl<<endl;
    }
    else
    {
        //
        // Destroyed, throw exception.
        //
        // cb->ice_exception(Demo::RequestCanceledException());
    }
}

void WorkQueue::destroy()
{
    IceUtil::Monitor<IceUtil::Mutex>::Lock lock(_monitor);
    _bIsRun = false; 
}


//-----以下为服务接口实现----------
ServerI::ServerI(const WorkQueuePtr& workQueue):
_workQueue(workQueue)
{
}

ServerI::~ServerI()
{
}

void ServerI::OMMPToCMSInfo_async(const AMD_EV9000MSI_OMMPToCMSInfoPtr& cb, 
								  ::Ice::Int nSendType, 
								  const COMMNMSGHEAD& tInCommMsg, 
								  const BYTEBUFFER& sInData, 
								  const ::Ice::Current& cur)
{
	if(0 ==nSendType )  //同步
	{
		cout <<endl<< "收到客户端同步数据调用命令" << endl;
		MsgOp::DealMsg(cb,tInCommMsg,sInData);
	}else{
		cout <<endl<< "收到客户端异步数据调用命令" << endl;
		_workQueue->add(cb,nSendType, tInCommMsg,sInData);
	}
}

::Ice::Int ServerI::OMMPToCMSCfg(const ::EV9000MS::COMMNMSGHEAD&, const ::EV9000MS::BYTEBUFFER&, const ::Ice::Current&)
{
	return 0;
}

::Ice::Int ServerI::OMMPToCMSQry(const ::EV9000MS::COMMNMSGHEAD&, ::EV9000MS::BYTEBUFFER&, const ::Ice::Current&)
{
	return 0;
}

::Ice::Int ServerI::TSURegister(const ::std::string& sTsuID, ::Ice::Int iSlotID, const ::std::string& sTsuVideoIP, ::Ice::Int iVideoIPEth, const ::std::string& sTsuDeviceIP, ::Ice::Int iDeviceIPEth, ::Ice::Int iExpires, ::Ice::Int iRefresh, ::Ice::Int iTsuType, ::Ice::Int & iTsuIndex, const ::Ice::Current&)
{
#ifndef WIN32
	MsgOp::DealTSURegister(sTsuID, iSlotID, sTsuVideoIP, iVideoIPEth, sTsuDeviceIP, iDeviceIPEth, iExpires, iRefresh, iTsuType, iTsuIndex);
#endif
	return 0;
}

::Ice::Int ServerI::AudioTSURegister(const ::std::string& sTsuID,::Ice::Int iExpires, ::Ice::Int iRefresh, const ::EV9000MS::AudioTaskAttributeList& tAudioTaskAttribute, const ::Ice::Current&)
{
#ifndef WIN32
	MsgOp::DealAudioTSURegister(sTsuID, iExpires, iRefresh, tAudioTaskAttribute);
#endif
	return 0;
}

::Ice::Int ServerI::TSUGetTime(const ::Ice::Current&)
{
#ifndef WIN32
	return MsgOp::DealTSUGetTime();
#else
	return 0;
#endif
}

::Ice::Int ServerI::TSUNotifyPlayEnd(const ::EV9000MS::TSUTaskAttribute& tTSUTaskAttribute, const ::Ice::Current&)
{
#ifndef WIN32
	return MsgOp::DealTSUNotifyPlayEnd(tTSUTaskAttribute);
#else
	return 0;
#endif
}

::Ice::Int ServerI::TSUNotifyPausePlay(const ::EV9000MS::TSUTaskAttribute& tTSUTaskAttribute, const ::Ice::Current&)
{
#ifndef WIN32
	return MsgOp::DealTSUNotifyPausePlay(tTSUTaskAttribute);
#else
	return 0;
#endif
}

::Ice::Int ServerI::TSUNotifyResumePlay(const ::EV9000MS::TSUTaskAttribute& tTSUTaskAttribute, const ::Ice::Current&)
{
#ifndef WIN32
	return MsgOp::DealTSUNotifyResumePlay(tTSUTaskAttribute);
#else
	return 0;
#endif
}

::Ice::Int ServerI::TSUNotifyCurrentTask(const ::EV9000MS::TSUTaskAttributeList& tTaskAttributeList, const ::Ice::Current&)
{
#ifndef WIN32
	return MsgOp::DealTSUNotifyCurrentTask(tTaskAttributeList);
#else
	return 0;
#endif
}

#if 1  //手机app增加接口


//rtsp当前任务上报
::Ice::Int ServerI::RTSPNotifyCurrentTask(const ::EV9000MS::RTSPTaskAttributeList& tRTSPTaskAttribute, const ::Ice::Current&)
{
#ifndef WIN32
	return MsgOp::DealRTSPNotifyCurrentTask(tRTSPTaskAttribute);
#else
	return 0;
#endif
}

//Tsc注册接口
::Ice::Int ServerI::TSCServerRegister(const ::std::string& sTscID, const ::std::string& sTscVideoIP, const ::std::string& iVideoIPEth, const ::std::string& sTscDeviceIP, const ::std::string& iDeviceIPEth, ::Ice::Int iExpires, ::Ice::Int iRefresh, int rtspserverport,const ::Ice::Current&)
{
#ifndef WIN32
		MsgOp::DealTSCRegister( sTscID,  sTscVideoIP,  iVideoIPEth,  sTscDeviceIP,  iDeviceIPEth,  iExpires,  iRefresh,rtspserverport );
#endif
		return 0;
}


//rtsp注册
::Ice::Int ServerI::RTSPServerRegister(const ::std::string& sTsuID, const ::std::string& sTsuVideoIP, const ::std::string& sVideoIPEth, const ::std::string& sTsuDeviceIP, const ::std::string& sDeviceIPEth, ::Ice::Int iExpires, ::Ice::Int iRefresh, int rtspserverport,const ::Ice::Current&)		
{
#ifndef WIN32
	MsgOp::DealRTSPRegister(sTsuID, sTsuVideoIP, sVideoIPEth,sTsuDeviceIP,sDeviceIPEth, iExpires, iRefresh,rtspserverport);
#endif
	return 0;
}


//TSC通知mms清除任务
::Ice::Int ServerI::TscNotifyTaskEnd(const ::std::string& sTscTaskID, const ::Ice::Current&)
{

#ifndef WIN32
	return MsgOp::DealTSCNotifyRTSPTaskEnd(sTscTaskID);
#else
	return 0;
#endif
}



//增加手机rtsp通知接口
::Ice::Int ServerI::RTSPNotifyTaskEnd(const ::std::string& sTaskID, const ::Ice::Current&)
{

#ifndef WIN32
	return MsgOp::DealTSUNotifyRTSPTaskEnd(sTaskID);
#else
	return 0;
#endif
}


#endif

::Ice::Int ServerI::TSUNotifyDeviceNoStream(const ::std::string& sDeviceID, const ::Ice::Current&)
{
#ifndef WIN32
	return MsgOp::DealTSUNotifyDeviceNoStream(sDeviceID);
#else
	return 0;
#endif
}

void ServerI::TSUNotifyCPUTemperature(::Ice::Int iSlotID, ::Ice::Int iTemperature, const ::Ice::Current&)
{
#ifndef WIN32
	MsgOp::DealTSUNotifyCPUTemperature(iSlotID, iTemperature);
	return;
#else
	return ;
#endif
}

::Ice::Int ServerI::TSUNotifyCreateTaskResult(const ::std::string& sTaskID, ::Ice::Int iResult, const ::Ice::Current&)
{
#ifndef WIN32
	return MsgOp::DealTSUNotifyCreateTaskResult(sTaskID, iResult);
#else
	return 0;
#endif
}

::Ice::Int ServerI::TSUNotifyAlarmInfo(const ::EV9000MS::TSUAlarmMsg& tTSUAlarmMsg, const ::Ice::Current&)
{
#ifndef WIN32
	return MsgOp::DealTSUNotifyAlarmInfo(tTSUAlarmMsg);
#else
	return 0;
#endif
}

::Ice::Int ServerI::GetCmsUsedStatus(const ::Ice::Current&)
{
#ifndef WIN32
    return MsgOp::DealGetCmsUsedStatus();
#else
    return 0;
#endif
}

::Ice::Int ServerI::GetSlaveCmsStatus(const ::Ice::Current&)
{
#ifndef WIN32
    return MsgOp::DealGetSlaveCmsStatus();
#else
    return 0;
#endif
}

// ***BEGIN***  ICE修改 wangqichao 2013/8/29 add
void ServerI::GetMyHeart_async(const ::EV9000MS::AMD_EV9000MSI_GetMyHeartPtr& cb,
                               ::Ice::Int nSendType, 
                               ::Ice::Int Type, 
                               ::Ice::Int Level,                               
                               const ::std::string& CMSID, 
                               const ::std::string& StrInfo, 
                               const ::Ice::Current& cur)
{
//    printf("\r\n GetMyHeart_async ok \r\n");
    if(0 == nSendType) //同步
    {
//        printf("\r\n GetMyHeart_async ok nSendType = 0\r\n");
//        PrintfSQLToFile_server("GetMyHeart_async ok nSendType = 0");
        int ret = 0;
        cb->ice_response(ret,"ok");
        //m_HA_send_recv = 0;
#ifndef WIN32
        pAMI_ICE_HA_Client->updatetimeHA();
#endif
    }
    else//异步
    {
//        printf("\r\n GetMyHeart_async ok nSendType = 1\r\n");
//        PrintfSQLToFile_server("GetMyHeart_async ok nSendType = 1");
        int ret = 0;
        cb->ice_response(ret,"ok");
#ifndef WIN32
        pAMI_ICE_HA_Client->updatetimeHA();
#endif
        //m_HA_send_recv = 0;
    }
}



void ServerI::GetMyHAInfo_async(const ::EV9000MS::AMD_EV9000MSI_GetMyHAInfoPtr& cb,
                                ::Ice::Int nSendType, 
                                ::Ice::Int Type, 
                                ::Ice::Int Level,                               
                                const ::std::string& CMSID, 
                                const ::std::string& StrInfo, 
                                const ::Ice::Current& cur)
{
    if(0 == nSendType) //同步
    {
        printf("\r\n GetMyHAInfo_async ok nSendType = 0\r\n");
//        PrintfSQLToFile_server("GetMyHAInfo_async ok nSendType = 0");
        int ret = 0;
#ifndef WIN32
        S2MexchangeProc();
#endif 
        cb->ice_response(ret,"ok");
#ifndef WIN32
        pAMI_ICE_HA_Client->setourstatus(0);
#endif        
    }
    else//异步
    {
        printf("\r\n GetMyHAInfo_async ok nSendType = 1\r\n");
//        PrintfSQLToFile_server("GetMyHAInfo_async ok nSendType = 1");
        int ret = 0;
#ifndef WIN32
        S2MexchangeProc();
#endif
		cb->ice_response(ret,"ok");
#ifndef WIN32
        pAMI_ICE_HA_Client->setourstatus(0);
#endif        
    }
}
// ***END***  ICE修改 wangqichao 2013/8/29 add

