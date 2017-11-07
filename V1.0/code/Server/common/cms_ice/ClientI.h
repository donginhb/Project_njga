#ifndef _EV9000_CLIENT_H_
#define _EV9000_CLIENT_H_

#include <Ice/Ice.h>
#include "EV9000_ManageService.h"
#include <iostream>
#include <IceUtil/IceUtil.h>
#include "IceCom.h"
#ifndef WIN32
#include "BoardInit.h"
#include "CPing.h"
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#endif
using namespace std;
using namespace EV9000MS;

extern int m_MyHeart_send_recv;
extern int m_MyHAInfo_send_recv;
extern int m_HA_send_recv;

extern bool PrintfSQLToFile_client(const char* c_sql);

#define WQCLOG

class AMI_ClientI : public AMI_EV9000MSI_OMMPToCMSInfo
{
public:
	
	virtual void ice_response(::Ice::Int nRet, const ::EV9000MS::COMMNMSGHEAD& tOutComMsg, const ::EV9000MS::BYTEBUFFER& sOutData)
	{
		EDVHEAD_EX tOutData;
  		int iLen = sOutData.size();
		char *szResult = new char[iLen+1];
		memset(szResult,0,iLen+1);
		memcpy(szResult,&sOutData[0],iLen);
		cout<<"异步收到数据如下:"<<endl<<szResult<<endl;
		delete [] szResult;
		//cout<<"异步收到数据:"<<(char *)&sOutData[0]<<endl;
	}

    virtual void ice_exception(const ::Ice::Exception&) {};
	
};



// ***BEGIN***  ICE修改 wangqichao 2013/8/29 add
class AMI_EV9000MSI_ICE_GetMyHeart : public AMI_EV9000MSI_GetMyHeart
{
public:
    virtual void ice_response(::Ice::Int ret, const ::std::string& rsp)
    {
//        printf("\r\n AMI_EV9000MSI_ICE_GetMyHeart::ice_response \r\n"); 
//        PrintfSQLToFile_client("AMI_EV9000MSI_ICE_GetMyHeart::ice_response");
        if(ret == 0)
        {
//             printf("\r\n AMI_EV9000MSI_ICE_GetMyHeart::ice_response ret = 0 \r\n"); 
//             PrintfSQLToFile_client("AMI_EV9000MSI_ICE_GetMyHeart::ice_response ret = 0");
             m_MyHeart_send_recv = 0;
//             printf("\r\n AMI_EV9000MSI_ICE_GetMyHeart->response m_send_recv = %d \r\n",m_MyHeart_send_recv);
             return;
        }
    }
    virtual void ice_exception(const ::Ice::Exception&)
    {

    }
};
class AMI_EV9000MSI_ICE_GetMyHAInfo : public AMI_EV9000MSI_GetMyHAInfo
{
public:
    virtual void ice_response(::Ice::Int ret, const ::std::string& rsp)
    {
        if(ret == 0)
        {
//             printf("\r\n AMI_EV9000MSI_ICE_GetMyHAInfo::ice_response ret = 0 \r\n");
//             PrintfSQLToFile_client("AMI_EV9000MSI_ICE_GetMyHAInfo::ice_response ret = 0");
             m_MyHAInfo_send_recv = 2;
             return;
        }
    }
    virtual void ice_exception(const ::Ice::Exception&)
    {

    }
    
    //int reault;
    //int m_send_recv;
};
#ifndef WIN32
class AMI_ICE_HA_Client
{
public:
    AMI_ICE_HA_Client(string StrServerIp,int IntServerPort,string OutStrServerIp,int OutIntServerPort);
    ~AMI_ICE_HA_Client();
    void SendInfo(int Type,int Level,string CMSID,string info);
    void SendInfotoHA(int Type,int Level,string CMSID,string info);
    void Out_SendInfo(int Type,int Level,string CMSID,string info);
    static void* Thread_IceClient(void *p);
    void ProgramRuning();
    void Close();
    int IntIce();
    int OutInitIce();
    void setourstatus(int tmpstatus);
    void updatetimeHA();
    //{
    //    m_our_status = tmpstatus;
    //}
    int getourstatus()
    {
        return m_our_status;
    }

    int check_inner_ice();
    int check_outer_ice();
    

    string m_StrServerIp;
    int    m_IntServerPort;
    string m_StrProxy;
    int    m_iTimer;

    time_t m_looptimer;
    time_t m_innertimer;
    time_t m_outertimer;
    time_t m_HAtimer;
    
    EV9000MS::EV9000MSIPrx prx;


    string m_OutStrServerIp;
    int    m_OutIntServerPort;
    string m_OutStrProxy;
    EV9000MS::EV9000MSIPrx out_prx;

    pthread_t    m_threadHandle;

    bool ThreadLog_Exit;
    bool ICE_Enable;
    // -1 初始化 ，0 主机, 1 备机
    int m_our_status;



    AMI_EV9000MSI_ICE_GetMyHeart *pAMI_EV9000MSI_ICE_GetMyHeart;
    AMI_EV9000MSI_ICE_GetMyHAInfo *pAMI_EV9000MSI_ICE_GetMyHAInfo;

    //int m_MyHeart_send_recv;
    
};
#endif
// ***END***  ICE修改 wangqichao 2013/8/29 add

#endif