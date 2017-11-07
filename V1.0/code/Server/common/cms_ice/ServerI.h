#ifndef _EV9000_SERVER_H_
#define _EV9000_SERVER_H_

#include <EV9000_ManageService.h>
#include <Ice/Application.h>
#include <IceUtil/Monitor.h>
#include <IceUtil/IceUtil.h>
#include <iostream>
#include <list>

#ifndef WIN32
#include "platformms/MSexchange.h"
#endif




using namespace std;
using namespace EV9000MS;

extern bool PrintfSQLToFile_server(const char* c_sql);


//任务处理队列
class WorkQueue : public IceUtil::Thread  
{
public:
	WorkQueue();
	virtual ~WorkQueue();
	
    virtual void run();
	void add(const AMD_EV9000MSI_OMMPToCMSInfoPtr& cb, 
		     const ::Ice::Int nSendType,
			 const COMMNMSGHEAD& tInCommMsg,
			 const BYTEBUFFER& sInData);
    void destroy();
	
private:
	
    struct CallbackEntry
    {
        AMD_EV9000MSI_OMMPToCMSInfoPtr cb;
        ::Ice::Int nSendType;
		COMMNMSGHEAD tInCommMsg;
		BYTEBUFFER sInData;
    };
	
    IceUtil::Monitor<IceUtil::Mutex> _monitor;  //锁
    std::list<CallbackEntry> _callbacks;  //任务队列
    bool _bIsRun;   //是否运行标志	
};

typedef IceUtil::Handle<WorkQueue> WorkQueuePtr;


//服务器处理接口类
class ServerI : virtual public EV9000MSI
{
public:
	
	ServerI(const WorkQueuePtr& workQueue);
	
	~ServerI();
	
	
    virtual void OMMPToCMSInfo_async(const AMD_EV9000MSI_OMMPToCMSInfoPtr& cb, 
	                                ::Ice::Int nSendType, 
	                              	const COMMNMSGHEAD& tInCommMsg, 
	                            	const BYTEBUFFER& sInData, 
	                            	const ::Ice::Current& = ::Ice::Current());

	virtual ::Ice::Int OMMPToCMSCfg(const ::EV9000MS::COMMNMSGHEAD&, const ::EV9000MS::BYTEBUFFER&, const ::Ice::Current& = ::Ice::Current());
	
	virtual ::Ice::Int OMMPToCMSQry(const ::EV9000MS::COMMNMSGHEAD&, ::EV9000MS::BYTEBUFFER&, const ::Ice::Current& = ::Ice::Current());

  virtual ::Ice::Int TSURegister(const ::std::string& sTsuID, ::Ice::Int iSlotID, const ::std::string& sTsuVideoIP, ::Ice::Int iVideoIPEth, const ::std::string& sTsuDeviceIP, ::Ice::Int iDeviceIPEth, ::Ice::Int iExpires, ::Ice::Int iRefresh, ::Ice::Int iTsuType, ::Ice::Int& iTsuIndex, const ::Ice::Current& = ::Ice::Current());
  virtual ::Ice::Int AudioTSURegister(const ::std::string& sTsuID,::Ice::Int iExpires, ::Ice::Int iRefresh, const ::EV9000MS::AudioTaskAttributeList&, const ::Ice::Current&);

  virtual ::Ice::Int TSUGetTime(const ::Ice::Current& = ::Ice::Current());
  virtual ::Ice::Int TSUNotifyPlayEnd(const ::EV9000MS::TSUTaskAttribute&, const ::Ice::Current&);
  virtual ::Ice::Int TSUNotifyPausePlay(const ::EV9000MS::TSUTaskAttribute&, const ::Ice::Current&);
  virtual ::Ice::Int TSUNotifyResumePlay(const ::EV9000MS::TSUTaskAttribute&, const ::Ice::Current&);
  virtual ::Ice::Int TSUNotifyCurrentTask(const ::EV9000MS::TSUTaskAttributeList&, const ::Ice::Current&);
  virtual ::Ice::Int TSUNotifyDeviceNoStream(const ::std::string& sDeviceID, const ::Ice::Current&);
  virtual ::Ice::Int TSUNotifyTcpTansferEnd(const ::std::string& strTranferSessionID, ::Ice::Int iType, const ::Ice::Current&);
  virtual void TSUNotifyCPUTemperature(::Ice::Int iSlotID, ::Ice::Int iTemperature, const ::Ice::Current&);
  virtual ::Ice::Int TSUNotifyCreateTaskResult(const ::std::string& sTaskID, ::Ice::Int iResult, const ::Ice::Current&);
  virtual ::Ice::Int TSUNotifyAlarmInfo(const ::EV9000MS::TSUAlarmMsg&, const ::Ice::Current&);
  virtual ::Ice::Int TSUSendImageResult(::Ice::Int iType, ::Ice::Int iResult, const ::std::string& strDeviceID, ::Ice::Int iChannelID, const ::std::string& strGuid, ::Ice::Int iPicCount, const BYTEBUFFER& strPicContent, const ::Ice::Current&);
  virtual ::Ice::Int GetCmsUsedStatus(const ::Ice::Current& = ::Ice::Current());
  virtual ::Ice::Int GetSlaveCmsStatus(const ::Ice::Current& = ::Ice::Current());	

  // ***BEGIN***  ICE修改 wangqichao 2013/8/29 add
  virtual void GetMyHeart_async(const ::EV9000MS::AMD_EV9000MSI_GetMyHeartPtr& cb,
                               ::Ice::Int nSendType, 
                               ::Ice::Int Type, 
                               ::Ice::Int Level,                               
                               const ::std::string& CMSID, 
                               const ::std::string& StrInfo, 
                               const ::Ice::Current& = ::Ice::Current());
  virtual void GetMyHAInfo_async(const ::EV9000MS::AMD_EV9000MSI_GetMyHAInfoPtr& cb,
                                ::Ice::Int nSendType, 
                                ::Ice::Int Type, 
                                ::Ice::Int Level,                               
                                const ::std::string& CMSID, 
                                const ::std::string& StrInfo, 
                                const ::Ice::Current& = ::Ice::Current());
  // ***END***  ICE修改 wangqichao 2013/8/29 add

private:
	WorkQueuePtr _workQueue;
};

#endif
