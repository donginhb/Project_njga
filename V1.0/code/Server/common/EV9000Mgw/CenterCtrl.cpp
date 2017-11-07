// CenterCtrl.cpp: implementation of the CCenterCtrl class.
//
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include "stdafx.h"
#include "EV9000Mgw.h"
#include "StartupGuard.h"
#else
#include "../../common/include/EV9000_InnerDef.h"
#endif
#include "CenterCtrl.h"
#include "MsgOp.h"
#include <fstream>

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifdef WIN32
extern CEV9000MgwApp theApp;
#endif

int get_sdp_t_time(sdp_message_t* sdp, int* start_time, int* end_time, int* play_time)
{
    if (sdp == NULL)
    {
        mgwlog("get_sdp_t_time() exit---: Param Error \r\n");
        return -1;
    }
    char* tmp = sdp_message_t_start_time_get(sdp, 0);
    char* tmp2 = sdp_message_t_stop_time_get(sdp, 0);
    if (tmp == NULL || tmp2 == NULL)
    {
        return -1;    /* no t line?? */
    }
    *start_time = atoi(tmp);
    *end_time = atoi(tmp2);
    char* tmp3 = sdp_message_r_repeat_get(sdp, 0, 0);
    if (NULL != tmp3)
    {
        *play_time = atoi(tmp3);
    }
    else
    {
        *play_time = 0;
    }
    return 0;
}

CCenterCtrl::CCenterCtrl()
{
	m_strMgwDeviceID ="";
	m_pDevCtrl=NULL;

	for (int i=0; i<THREAD_DEALMSG_MAX; i++)
	{
		m_bThreadDealMsgFlag[i] = FALSE;
#ifdef WIN32
		m_hThreadDealMsg[i] = 0;
		m_dwThreadDealMsgID[i] = 0;
#else
		m_threadDealMsgHandle[i] = 0;
#endif
		memset(&(m_stThpara[i]),0,sizeof(THPARA)); 
	}

	m_bThreadRefreshFlag =FALSE;
#ifdef WIN32
    m_hThreadRefresh = 0;
    m_dwThreadRefreshID =0;
	m_lpInfoUdp =NULL;
	m_bThreadInfoRecvFlag = FALSE;
	m_hThreadInfoRecv = 0;
	m_dwThreadInfoRecvID = 0;
#else
    m_threadRefreshHandle =0;
#endif
}

CCenterCtrl::~CCenterCtrl()
{
	Close();
}

static int count=0;

BOOL CCenterCtrl::Start()
{
	//初始化各模块
	if(CSysCfg::Instance()->Init()!=0)  //初始化配置失败
	{
		mgwlog("初始化配置失败\n");
		return FALSE;
	}
	
	if(!m_pDevCtrl)
	{
		m_pDevCtrl= new CDevCtrl();
		if(!m_pDevCtrl)
		{
			return FALSE;
		}
	}

#ifdef WIN32
	if (!m_lpInfoUdp)    //日记
	{
		m_lpInfoUdp = new CUdp2();
		if (!m_lpInfoUdp->Open(DM_INFO_RECV_PORT, "127.0.0.1"))
		{
			mgwlog("打开信息接收端口[%d]失败", DM_INFO_RECV_PORT);
			return FALSE;
		}
		mgwlog("打开信息接收端口 %d 成功", DM_INFO_RECV_PORT);
	}
	
	if (!m_bThreadInfoRecvFlag)  //创建日记消息线程
	{
		m_bThreadInfoRecvFlag = TRUE;
		m_hThreadInfoRecv = CreateThread(NULL, 0, ThreadInfoRecv, (LPVOID)this, 0, &m_dwThreadInfoRecvID);
		if (!m_hThreadInfoRecv)
		{//创建线程失败
			mgwlog("创建信息接收线程失败\n");
			return FALSE;
		}
		mgwlog("创建信息接收线程[0x%x]", m_dwThreadInfoRecvID);
	}
#endif
	m_strMgwDeviceID = CSysCfg::Instance()->GetstrPara(SYS_CFG_MGWDEVID);   //从配置中获取
	InitDevice();          //初始化device
    StartDealMsg();        //打开命令处理线程
    Register();            //向CMS注册
	//for test chenyu
// 	count++;
// 	if(count<=3)
// 	{
// 		return FALSE;
// 	}
	return TRUE;
}

BOOL CCenterCtrl::Close()
{
	//关闭消息处理线程
	for (int iCount=0; iCount<THREAD_DEALMSG_MAX; iCount++)
	{
#ifdef WIN32
		if(m_hThreadDealMsg[iCount])
		{
			m_bThreadDealMsgFlag[iCount] = FALSE;
			WaitForSingleObject(m_hThreadDealMsg[iCount], INFINITE);
			CLOSE_HANDLE(m_hThreadDealMsg[iCount]);
			m_dwThreadDealMsgID[iCount] = 0;
			
			mgwlog("关闭消息处理线程,ID:[%d]\n",iCount);
		}
#else
		if(m_threadDealMsgHandle[iCount])
		{
			m_bThreadDealMsgFlag[iCount] = FALSE;
			pthread_cancel(m_threadDealMsgHandle[iCount]);
			pthread_join(m_threadDealMsgHandle[iCount], NULL);
			m_threadDealMsgHandle[iCount] = 0;
			mgwlog("关闭消息处理线程,ID:[%d]\n",iCount);
		}
#endif
	}

	//关闭刷新线程
#ifdef WIN32
	if(m_hThreadRefresh)
    {
		m_bThreadRefreshFlag = FALSE;
		WaitForSingleObject(m_hThreadRefresh, INFINITE);
        CLOSE_HANDLE(m_hThreadRefresh);
		m_dwThreadRefreshID = 0;
	}

	if(m_hThreadInfoRecv)
    {//销毁UDP信息接收线程
		m_bThreadInfoRecvFlag = FALSE;
		if (m_lpInfoUdp)
		{
			m_lpInfoUdp->ShutDown();
		}
		if(WAIT_TIMEOUT == WaitForSingleObject(m_hThreadInfoRecv, INFINITE))
		{
			TerminateThread(m_hThreadInfoRecv, 1041);
		}
        CLOSE_HANDLE(m_hThreadInfoRecv);
		m_dwThreadInfoRecvID = 0;
	}
	MEMORY_DELETE(m_lpInfoUdp);
#else
	if(m_threadRefreshHandle)  
    {
		m_bThreadRefreshFlag = FALSE;
		pthread_cancel(m_threadRefreshHandle);
		pthread_join(m_threadRefreshHandle, NULL);
		m_threadRefreshHandle =0;
	}
#endif
	FiniDevice();
	MEMORY_DELETE(m_pDevCtrl);    //关闭设备控制模块
	return TRUE;
}

//根据查询结果，动态加载设备sdk并打开设备。
BOOL CCenterCtrl::InitDevice()
{
	if(m_pDevCtrl)
	{
		return ( 0 == m_pDevCtrl->Start() );
	}else{
		return FALSE;
	}
}

//设备关闭
BOOL CCenterCtrl::FiniDevice()
{
	if(m_pDevCtrl)
	{
		return ( 0 == m_pDevCtrl->Close() );
	}else{
		return FALSE;
	}
}          

//向CMS注册
BOOL CCenterCtrl::Register()
{
	mgwlog("开始向CMS注册\n");
	EV9000_LOGININFO loginInfo;
	sprintf(loginInfo.sServerIP,CSysCfg::Instance()->GetstrPara(SYS_CFG_CMSIP).c_str());             // 服务器IP
	loginInfo.nServerPort=CSysCfg::Instance()->GetdwPara(SYS_CFG_SERVERSIPPORT);                                 // 服务器端口
	sprintf(loginInfo.sUserName,CSysCfg::Instance()->GetstrPara(SYS_CFG_USERNAME).c_str());        //[EV9000_NORMAL_STRING_LEN];
	sprintf(loginInfo.sUserPwd,CSysCfg::Instance()->GetstrPara(SYS_CFG_USERPWD).c_str());                //[EV9000_NORMAL_STRING_LEN];
	sprintf(loginInfo.sServerID,CSysCfg::Instance()->GetstrPara(SYS_CFG_SERVERID).c_str());    //[EV9000_NORMAL_STRING_LEN];             // 服务器编号
	sprintf(loginInfo.sUserID,(char *)CSysCfg::Instance()->GetstrPara(SYS_CFG_MGWDEVID).c_str());      //[EV9000_NORMAL_STRING_LEN];
    //sprintf(loginInfo.sLocalIP,CSysCfg::Instance()->GetstrPara(SYS_CFG_LOCALIP_IN).c_str());            //[EV9000_NORMAL_STRING_LEN];
	sprintf(loginInfo.sLocalIP,CSysCfg::Instance()->GetstrPara(SYS_CFG_LOCALIP_INTERCOM).c_str());            //[EV9000_NORMAL_STRING_LEN]; 
	mgwlog("登录信息ServerIP:%s ServerPort:%d \nServerID:%s UserName:%s UserPwd:%s \nUserID:%s LocalIP:%s\n",
			loginInfo.sServerIP,loginInfo.nServerPort,loginInfo.sServerID,
			loginInfo.sUserName,loginInfo.sUserPwd,loginInfo.sUserID,loginInfo.sLocalIP);
	loginInfo.nDigital =0; 	// 是否使用数字证书登录 0 不使用 1 使用
    CCtrlProOp::Instance()->SetLoginInfo(loginInfo);
	int nRet = CCtrlProOp::Instance()->Register();
    if(nRet !=0)
	{
		mgwlog("向CMS注册失败\n");
	}
	
    mgwlog("启动刷新线程\n");
#ifdef WIN32
	if(!m_bThreadRefreshFlag)  //启动刷新线程
	{
		m_bThreadRefreshFlag = TRUE;
		m_hThreadRefresh = CreateThread(NULL, 0, ThreadRefresh, (LPVOID)this, 0, &m_dwThreadRefreshID);
	}
#else
	int err=0;
	if (!m_bThreadRefreshFlag)
	{
		m_bThreadRefreshFlag = TRUE;
		err = pthread_create(&m_threadRefreshHandle,NULL,ThreadRefresh,(void*)this);
		if(err!=0)
		{
			return FALSE;
		}
	}
#endif
	return ( 0 == nRet);
}

//启动SIP Message处理线程
BOOL CCenterCtrl::StartDealMsg()
{
   	//多线程
	for (int i=0; i<THREAD_DEALMSG_MAX; i++)
	{
		m_stThpara[i].p = this;
		m_stThpara[i].iHandle = i;
#ifdef WIN32
		if (!m_bThreadDealMsgFlag[i])
		{
			m_bThreadDealMsgFlag[i] = TRUE;
			m_hThreadDealMsg[i] = CreateThread(NULL, 0, ThreadDealMsg, &(m_stThpara[i]), 0, &m_dwThreadDealMsgID[i]);
		}
#else
		int err=0;
		if (!m_bThreadDealMsgFlag[i])
		{
			m_bThreadDealMsgFlag[i] = TRUE;
			err = pthread_create(&m_threadDealMsgHandle[i],NULL,ThreadDealMsg,(void*)&(m_stThpara[i]));
			if(err!=0)
			{
				return FALSE;
			}
		}
#endif
		mgwlog("打开消息处理线程:%d\n",i);
	}
	return TRUE;
}

//待增加
//消息类型
//非及时处理：普通命令
//及时处理1： 控球命令 
//及时处理2： Invite命令
//其它命令：  获取参数 错发I帧
//处理命令前判断设备状态。
//分类处理 每类多线程处理

// Sip命令处理函数
int	CCenterCtrl::OnDealMsg(int iHandle)
{
	mgwlog("进入Sip命令处理函数，开始处理消息，线程ID:%d\n",iHandle);
	CBSIPMSG stCbSipmsg;
	while (m_bThreadDealMsgFlag[iHandle])
	{
		if(!CCtrlProOp::Instance()->GetMsg(stCbSipmsg))
		{
#ifdef WIN32
			Sleep(10);
#else
			usleep(10*1000);
#endif
			//mgwlog("待处理消息队列长度:%d",CCtrlProOp::Instance()->GetMsgLen());
			continue;
		}
		
		DealMsg(stCbSipmsg);
	}
	mgwlog("退出Sip命令处理函数\n");
	return 0;	
}

// 注册刷新函数
int	CCenterCtrl::OnRefresh()
{
	mgwlog("进入刷新函数\n");
	int nRegFailNum=0;
	string strTimeOut = CSysCfg::Instance()->GetstrPara(SYS_CFG_SIPREFRESHTIME);
	int iTimeOutDefault = atoi(strTimeOut.c_str());
	if(iTimeOutDefault < MIN_SIP_TIMEOUT)
	{
          iTimeOutDefault = MAX_SIP_TIMEOUT; 
	}
	int iTimeOut = MAX_SIP_TIMEOUT;  //单位 s
	iTimeOut = iTimeOutDefault;
    DWORD dwTime = GetTickCount();
	DWORD dwDevRefreshTime = GetTickCount();
	while (m_bThreadRefreshFlag)
	{
		if((GetTickCount()-dwTime)>= iTimeOut*1000)
		{
			mgwlog("----发送刷新消息,时间间隔:%ld----\n",iTimeOut);
			if(CCtrlProOp::Instance()->RefreshReg()!=0)
			{
				nRegFailNum ++;
				if(nRegFailNum >3)
				{
					mgwlog("----Info:刷新注册失败----\n");
					CCenterCtrl::Register();
					nRegFailNum =0;
				}
				iTimeOut = iTimeOutDefault/2;   //失败缩短超时
			}else{
				mgwlog("----Info:刷新注册成功----\n");
				iTimeOut = iTimeOutDefault;
			}
			dwTime = GetTickCount();  //更新刷新时间
		}

#ifdef WIN32
		Sleep(2000);
#else
		if((GetTickCount()-dwDevRefreshTime)>= REFRESH_DEV_TIME*1000)  //10 min
		{
			mgwlog("----更新内存数据库信息，开始刷新设备----\n");
			if(m_pDevCtrl)
			{
				if(0==CSysCfg::Instance()->Init())
				{
					mgwlog("----开始刷新设备----\n"); 
					m_pDevCtrl->OpenAllDev();
					m_pDevCtrl->CheckAllDev(); 
				}
			}
			mgwlog("----结束刷新设备----\n"); 
			dwDevRefreshTime = GetTickCount();  //更新刷新时间
		}
		sleep(2);
#endif
	}
	mgwlog("退出刷新函数\n");
	return 0;	
}

#ifdef WIN32
int CCenterCtrl::OnInfoRecv()
{
	while (m_bThreadInfoRecvFlag)
	{
		DWORD dwRemotIP = 0;
		WORD wRemotPort = 0;
		int nLen = m_lpInfoUdp->Recv(dwRemotIP, wRemotPort);
		if (0 < nLen)
		{
			PEDVHEAD lpHead = (PEDVHEAD)m_lpInfoUdp->GetBuffer();
			if (lpHead)
			{
				if (nLen == EDV_HEAD_LEN+lpHead->wPayLoadLen)
				{
					//TRACE("UDP:0x%x\n", lpHead->dwEvent);
					switch (lpHead->dwEvent)
					{
					case DM_LOG_NOTIFY:
						{//运行信息
							char chInfo[MAX_PATH*2] = {'\0'};
							//sprintf(chInfo, "DeviceModel>> %s", (char*)lpHead->ucPayLoad);
							sprintf(chInfo, "EV9000Mgw log>> %s", (char*)lpHead->ucPayLoad);
							mgwlog("%s\n",chInfo);
						}
						break;
					case DM_REALSTREAM_INFO_NOTIFY:
						{
							
						}
						break;
					default:
						break;
					}
				}
				else
				{
					mgwlog("CCenterCtrl::OnInfoRecv() m_lpInfoUdp->Recv 长度不对\n");
				}
			}
		}
		Sleep(10);
	}
	return 0;
}
#endif

//执行命令
int CCenterCtrl::DealMsg(CBSIPMSG &stCbSipmsg)
{
	if(SIP_MSG_MSG == stCbSipmsg.eSipMsgType)  //msg 消息
	{
		//解析XML指令
		MsgOp oMsgOp;
		oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
		string  strMsgType,strCmdType;
		strMsgType = oMsgOp.GetMsgType();
		strCmdType = oMsgOp.GetMsgText("CmdType");
		
		//处理指令
		if(("Query" == strMsgType ) && ("DeviceInfo" == strCmdType))  //获取设备信息信息指令
		{
			mgwlog("----处理Query消息,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoDeviceInfoCmd(stCbSipmsg);
		}
		else if("Catalog" == strCmdType )
		{
			mgwlog("----处理Catalog消息,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoCatalogCmd(stCbSipmsg);
		}
		else if( ("Control" == strMsgType) && (strCmdType == "DeviceControl"))  //控球命令
		{
			mgwlog("----处理Control消息,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoDeviceControlCmd(stCbSipmsg);
		}
		else if(strCmdType == "PresetConfig")  //设置/删除/执行前端预置位 
		{
			mgwlog("----处理PresetConfig消息,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoDeviceControlCmd(stCbSipmsg);  //和控球命令处理相同
		}
		else if(strCmdType == "SetDeviceVideoParam")  //设置前端图像参数
		{
			mgwlog("----处理SetDeviceVideoParam消息,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoSetDeviceVideoParamCmd(stCbSipmsg);
		}
		else if(strCmdType == "GetDeviceVideoParam")  //获取前端图像参数
		{
			mgwlog("----处理GetDeviceVideoParam消息,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoGetDeviceVideoParamCmd(stCbSipmsg);
		}
		else if(strCmdType == "RequestIFrameData")  //请求I帧
		{
			mgwlog("----处理RequestIFrameData消息,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoRequestIFrameDataCmd(stCbSipmsg);
		}
		else if(strCmdType == "AutoZoomIn")  //点击放大
		{
			mgwlog("----处理AutoZoomIn消息,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoAutoZoomInCmd(stCbSipmsg);
		}
		else if(strCmdType == "RecordInfo")  //点击放大
		{
			mgwlog("----处理RecordInfo消息,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
#ifdef WIN32
			DoRecordQuery(stCbSipmsg);
#endif
		}
		else
		{
			mgwlog("----不能处理的msg消息指令\n");
		}
	}
	else if(SIP_MSG_INVITE == stCbSipmsg.eSipMsgType) //invite 消息
	{
		mgwlog("----处理Invite消息,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
		DoInviteCmd(stCbSipmsg);
	}
	else if((SIP_MSG_BYE == stCbSipmsg.eSipMsgType)||(SIP_MSG_CANCEL == stCbSipmsg.eSipMsgType)) //bye/cancle 消息
	{
		if(SIP_MSG_CANCEL == stCbSipmsg.eSipMsgType)
		{
			mgwlog("----处理Cancel消息,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
		}else{
			mgwlog("----处理bye消息,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
		}
		DoByeCmd(stCbSipmsg);
	}
	else if((SIP_MSG_EXPIRE == stCbSipmsg.eSipMsgType)) //expire 消息
	{
		string callee_id;
		CCtrlProOp::Instance()->GetIDByDialogIndex(stCbSipmsg.idialog_index,callee_id);
		stCbSipmsg.strcallee_id = callee_id;
		mgwlog("----处理expire消息,callee_id:%s \n",stCbSipmsg.strcallee_id.c_str());
		DoByeCmd(stCbSipmsg);
	}
	else if((SIP_MSG_INFORM == stCbSipmsg.eSipMsgType)) //inform 消息
	{
		//mgwlog("----处理Inform消息,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
		DoInform(stCbSipmsg);
	}
	else
	{
		mgwlog("----不能处理的消息类型\n");
	}
	return 0;
}

//处理Catalog命令
int CCenterCtrl::DoCatalogCmd(CBSIPMSG &stCbSipmsg)
{
	mgwlog("进入 CCenterCtrl::DoCatalogCmd\n");
	int iCount =0;  //已处理节点数
	int iSumNum =0;  //节点总数
    map<string,UNGB_CHCONFIGREC>  mapChConfig;
    CSysCfg::Instance()->GetChCfg(mapChConfig);
	iSumNum = mapChConfig.size();
	if(0 == iSumNum)
	{
		return 0;
	}
    map<string,UNGB_CHCONFIGREC>::iterator iter = mapChConfig.begin(); 
	while (iCount<iSumNum)
	{
		//构造回复消息
		SIP_MSG_INFO msg_info;
		ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
		sprintf(msg_info.CallerID,"%s",stCbSipmsg.strcallee_id.c_str());  //回复消息调换
		sprintf(msg_info.CalleedID,"%s",stCbSipmsg.strcaller_id.c_str());
		
		MsgOp oMsgOp;
		oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
		
		//创建一个XML的文档对象。
		TiXmlDocument *myDocument = new TiXmlDocument();
		if(!myDocument)
		{
			MEMORY_DELETE_EX(msg_info.pMsg);
			return -1;
		}
		TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
		myDocument->LinkEndChild(decl);
		
		//创建一个根元素(消息类型)并连接。
		TiXmlElement *RootElement = new TiXmlElement("Response");
		myDocument->LinkEndChild(RootElement);
		
		//创建一个CmdType元素并连接
		TiXmlElement *xmlNode = new TiXmlElement("CmdType");
		RootElement->LinkEndChild(xmlNode);
		TiXmlText *xmlContent = new TiXmlText("Catalog");
		xmlNode->LinkEndChild(xmlContent);
		
		xmlNode = new TiXmlElement("SN");
		RootElement->LinkEndChild(xmlNode);
		xmlContent = new TiXmlText(oMsgOp.GetMsgText("SN").c_str());
		xmlNode->LinkEndChild(xmlContent);
		
		xmlNode = new TiXmlElement("DeviceID");
		RootElement->LinkEndChild(xmlNode);
		xmlContent = new TiXmlText(oMsgOp.GetMsgText("DeviceID").c_str());
		xmlNode->LinkEndChild(xmlContent);
		
		//查询数据库表(UnGBPhyDeviceChannelConfig)得到通道信息  
		char szSumNum[10]={0};
		sprintf(szSumNum,"%d",iSumNum);
		xmlNode = new TiXmlElement("SumNum");
		RootElement->LinkEndChild(xmlNode);
		xmlContent = new TiXmlText(szSumNum);
		xmlNode->LinkEndChild(xmlContent);
		
		int iNum=0;
        if(iCount + MAX_SO_NUM<=iSumNum)
		{
	        iNum = MAX_SO_NUM;
		}else{
			iNum = iSumNum - iCount;
		}
		char szNum[10]={0};
		sprintf(szNum,"%d",iNum);
		TiXmlElement *xmlDeviceListNode = new TiXmlElement("DeviceList");  //DeviceList
		xmlDeviceListNode->SetAttribute("Num",szNum);
		RootElement->LinkEndChild(xmlDeviceListNode);
		
		int iItemNum=0;   //本次xml已处理数目
		TiXmlElement *xmlItemNode = NULL;
		TiXmlText *xmlItemContent = NULL;
		TiXmlElement *xmlItemNodeEle = NULL;
		TiXmlText *xmlItemContentEle = NULL;
		
		while (iItemNum<iNum)
		{   
			xmlItemNode = new TiXmlElement("Item");         //<Item>
			xmlDeviceListNode->LinkEndChild(xmlItemNode);
			
			xmlItemNodeEle = new TiXmlElement("DeviceID");
			xmlItemNode->LinkEndChild(xmlItemNodeEle);
			xmlItemContentEle = new TiXmlText((iter->second).strLogicDeviceID.c_str());
			xmlItemNodeEle->LinkEndChild(xmlItemContentEle);
			
			xmlItemNodeEle = new TiXmlElement("Name");
			xmlItemNode->LinkEndChild(xmlItemNodeEle);
			xmlItemContentEle = new TiXmlText((iter->second).strChannelName.c_str());
			xmlItemNodeEle->LinkEndChild(xmlItemContentEle);
			
			xmlItemNodeEle = new TiXmlElement("Manufacturer");
			xmlItemNode->LinkEndChild(xmlItemNodeEle);
			xmlItemContentEle = new TiXmlText("Manufacturer1");
			xmlItemNodeEle->LinkEndChild(xmlItemContentEle);
			
			xmlItemNodeEle = new TiXmlElement("Model");
			xmlItemNode->LinkEndChild(xmlItemNodeEle);
			xmlItemContentEle = new TiXmlText("Model1");
			xmlItemNodeEle->LinkEndChild(xmlItemContentEle);
			
			xmlItemNodeEle = new TiXmlElement("Owner");
			xmlItemNode->LinkEndChild(xmlItemNodeEle);
			xmlItemContentEle = new TiXmlText("Owner 1");
			xmlItemNodeEle->LinkEndChild(xmlItemContentEle);
			
			xmlItemNodeEle = new TiXmlElement("ParentID");
			xmlItemNode->LinkEndChild(xmlItemNodeEle);
			xmlItemContentEle = new TiXmlText((char *)m_strMgwDeviceID.c_str());
			xmlItemNodeEle->LinkEndChild(xmlItemContentEle);
			
			xmlItemNodeEle = new TiXmlElement("Status");
			xmlItemNode->LinkEndChild(xmlItemNodeEle);

			CHANNELITEM_STATE eChannelState = CHANNELITEM_STATE_CLOSE;
			string strChState = "ON";
			if(m_pDevCtrl)
			{
				m_pDevCtrl->GetUnitState((iter->second).strLogicDeviceID,eChannelState);
				strChState = CovertState2str(eChannelState);		
			}
#ifndef WIN32
			mgwlog("ID:%s ChName:%s ChState:%s\n",
				(iter->second).strLogicDeviceID.c_str(),(iter->second).strChannelName.c_str(),strChState.c_str());
#endif
			xmlItemContentEle = new TiXmlText(strChState.c_str());
			xmlItemNodeEle->LinkEndChild(xmlItemContentEle);
			iItemNum++;
			iCount++;
			iter++;
		}
		
		//xml文档内容输出为字符串
		TiXmlPrinter printer;
		myDocument->Accept(&printer);
		string strSipMsg ="";
		strSipMsg = printer.CStr();
		int iLen = strSipMsg.length();

		msg_info.pMsg=NULL;
		msg_info.pMsg = new char[iLen+1];    //1024
		memset(msg_info.pMsg,0,iLen+1);
		sprintf(msg_info.pMsg,"%s",printer.CStr());
		msg_info.nMsgLen=strlen(msg_info.pMsg);
		int iRet =-1;
		iRet = CCtrlProOp::Instance()->SendMsg(msg_info);
		MEMORY_DELETE(myDocument);
		MEMORY_DELETE_EX(msg_info.pMsg);	
	}
	mgwlog("退出 CCenterCtrl::DoCatalogCmd\n");
	return 0;
}

#ifdef WIN32

string CCenterCtrl::CovertSystime2ISO(SYSTEMTIME systime)
{
	char buf[500]={0};
	sprintf(buf, "%i-%02i-%02iT%02i:%02i:%02i", 
		systime.wYear, systime.wMonth, systime.wDay,systime.wHour,systime.wMinute,systime.wSecond);
    return buf;
}

int CCenterCtrl::DoRecordQuery(CBSIPMSG &stCbSipmsg)
{
	mgwlog("进入 CCenterCtrl::DoRecordQuery\n");
	if(m_pDevCtrl)
	{
		//解析XML指令
		MsgOp oMsgOp;
		oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
		string  strDeviceID;
		string  strStartTime;
		string  strStopTime;
		strDeviceID = oMsgOp.GetMsgText("DeviceID");
        strStartTime = oMsgOp.GetMsgText("StartTime");
		strStopTime = oMsgOp.GetMsgText("EndTime");
		//mgwlog("RecordQuery:%s\n",stCbSipmsg.strMsg.c_str());
		CString s(strStartTime.c_str());   
		int nYear,nMonth,nDay,nHour,nMin,nSec;   
		sscanf(s,"%d-%d-%dT%d:%d:%d",&nYear,&nMonth,&nDay,&nHour,&nMin,&nSec);   
        SYSTEMTIME sStartTime,sStopTime;
		sStartTime.wYear = nYear;
		sStartTime.wMonth = nMonth;
		sStartTime.wDay = nDay;
		sStartTime.wHour = nHour;
		sStartTime.wMinute = nMin;
		sStartTime.wSecond = nSec;
		
		s = strStopTime.c_str();   
		sscanf(s,"%d-%d-%dT%d:%d:%d",&nYear,&nMonth,&nDay,&nHour,&nMin,&nSec);   
		sStopTime.wYear = nYear;
		sStopTime.wMonth = nMonth;
		sStopTime.wDay = nDay;
		sStopTime.wHour = nHour;
		sStopTime.wMinute = nMin;
		sStopTime.wSecond = nSec;
		int nTableCount = MAX_REC_QUERY_NUM; //查询录像记录
		EDVDVRRECORDTABLE* lpTable =NULL;
		lpTable = new EDVDVRRECORDTABLE[nTableCount];
		int iTotalCount = 0;
		int nRet = m_pDevCtrl->GetRecordInfo(strDeviceID,sStartTime,sStopTime,lpTable,nTableCount,iTotalCount);
		if (iTotalCount== -1)
		{
			mgwlog("查询到录像记录失败\n");
			MEMORY_DELETE_EX(lpTable);
			SendEmptyRecordMsg(stCbSipmsg);  //发送空消息
			return 0;
		}
		mgwlog("查询到录像记录:%d 获取:%d\n",iTotalCount,nTableCount);
        int iMinCount = 0;
		iMinCount = (iTotalCount<=nTableCount)?iTotalCount:nTableCount;
		
    	int iCount =0;  //已处理节点数
		int iSumNum =0;  //节点总数
		
		iSumNum = iMinCount;
		if(0 == iSumNum)
		{
			MEMORY_DELETE_EX(lpTable);
			SendEmptyRecordMsg(stCbSipmsg);  //发送空消息
			return 0;
		}
		while (iCount<iSumNum)
		{
			//构造回复消息
			SIP_MSG_INFO msg_info;
			ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
			sprintf(msg_info.CallerID,"%s",stCbSipmsg.strcallee_id.c_str());  //回复消息调换
			sprintf(msg_info.CalleedID,"%s",stCbSipmsg.strcaller_id.c_str());
			
			// 			MsgOp oMsgOp;
			// 			oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
			
			//创建一个XML的文档对象。
			TiXmlDocument *myDocument = new TiXmlDocument();
			if(!myDocument)
			{
				MEMORY_DELETE_EX(msg_info.pMsg);
				return -1;
			}
			TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
			myDocument->LinkEndChild(decl);
			
			//创建一个根元素(消息类型)并连接。
			TiXmlElement *RootElement = new TiXmlElement("Response");
			myDocument->LinkEndChild(RootElement);
			
			//创建一个CmdType元素并连接
			TiXmlElement *xmlNode = new TiXmlElement("CmdType");
			RootElement->LinkEndChild(xmlNode);
			TiXmlText *xmlContent = new TiXmlText("RecordInfo");
			xmlNode->LinkEndChild(xmlContent);
			
			xmlNode = new TiXmlElement("SN");
			RootElement->LinkEndChild(xmlNode);
			xmlContent = new TiXmlText(oMsgOp.GetMsgText("SN").c_str());
			xmlNode->LinkEndChild(xmlContent);
			
			xmlNode = new TiXmlElement("DeviceID");
			RootElement->LinkEndChild(xmlNode);
			xmlContent = new TiXmlText(strDeviceID.c_str());
			xmlNode->LinkEndChild(xmlContent);
			
			//查询数据库表(UnGBPhyDeviceChannelConfig)得到通道信息  
			char szSumNum[10]={0};
			sprintf(szSumNum,"%d",iSumNum);
			xmlNode = new TiXmlElement("SumNum");
			RootElement->LinkEndChild(xmlNode);
			xmlContent = new TiXmlText(szSumNum);
			xmlNode->LinkEndChild(xmlContent);
			
			int iNum=0;
			if(iCount + MAX_SO_NUM<=iSumNum)
			{
				iNum = MAX_SO_NUM;
			}else{
				iNum = iSumNum - iCount;
			}
			char szNum[10]={0};
			sprintf(szNum,"%d",iNum);
			TiXmlElement *xmlDeviceListNode = new TiXmlElement("RecordList");  //RecordList
			xmlDeviceListNode->SetAttribute("Num",szNum);
			RootElement->LinkEndChild(xmlDeviceListNode);
			
			int iItemNum=0;   //本次xml已处理数目
			TiXmlElement *xmlItemNode = NULL;
			TiXmlText *xmlItemContent = NULL;
			TiXmlElement *xmlItemNodeEle = NULL;
			TiXmlText *xmlItemContentEle = NULL;
			
			while (iItemNum<iNum)
			{   
				xmlItemNode = new TiXmlElement("Item");         //<Item>
				xmlDeviceListNode->LinkEndChild(xmlItemNode);
				
				xmlItemNodeEle = new TiXmlElement("DeviceID");
				xmlItemNode->LinkEndChild(xmlItemNodeEle);
				xmlItemContentEle = new TiXmlText(strDeviceID.c_str());
				xmlItemNodeEle->LinkEndChild(xmlItemContentEle);
				
				xmlItemNodeEle = new TiXmlElement("Name");
				xmlItemNode->LinkEndChild(xmlItemNodeEle);
				xmlItemContentEle = new TiXmlText(((EDVDVRRECORDTABLE*)(lpTable+iCount))->FileName);
				xmlItemNodeEle->LinkEndChild(xmlItemContentEle);
				
				xmlItemNodeEle = new TiXmlElement("StartTime");
				xmlItemNode->LinkEndChild(xmlItemNodeEle);
				xmlItemContentEle = new TiXmlText(CovertSystime2ISO(((EDVDVRRECORDTABLE*)(lpTable+iCount))->FileTime).c_str());
				xmlItemNodeEle->LinkEndChild(xmlItemContentEle);
				
				xmlItemNodeEle = new TiXmlElement("EndTime");
				xmlItemNode->LinkEndChild(xmlItemNodeEle);
				xmlItemContentEle = new TiXmlText(CovertSystime2ISO(((EDVDVRRECORDTABLE*)(lpTable+iCount))->StopFileTime).c_str());
				xmlItemNodeEle->LinkEndChild(xmlItemContentEle);
				
				xmlItemNodeEle = new TiXmlElement("Type");
				xmlItemNode->LinkEndChild(xmlItemNodeEle);
				xmlItemContentEle = new TiXmlText("time");
				xmlItemNodeEle->LinkEndChild(xmlItemContentEle);
				
				xmlItemNodeEle = new TiXmlElement("RecorderID");
				xmlItemNode->LinkEndChild(xmlItemNodeEle);
				xmlItemContentEle = new TiXmlText(strDeviceID.c_str());
				xmlItemNodeEle->LinkEndChild(xmlItemContentEle);
				iItemNum++;
				iCount++;
			}
			
			//xml文档内容输出为字符串
			TiXmlPrinter printer;
			myDocument->Accept(&printer);
			string strSipMsg ="";
			strSipMsg = printer.CStr();
			int iLen = strSipMsg.length();
			
			msg_info.pMsg=NULL;
			msg_info.pMsg = new char[iLen+1];    //1024
			memset(msg_info.pMsg,0,iLen+1);
			sprintf(msg_info.pMsg,"%s",printer.CStr());
			msg_info.nMsgLen=strlen(msg_info.pMsg);
			int iRet =-1;
			iRet = CCtrlProOp::Instance()->SendMsg(msg_info);
			MEMORY_DELETE(myDocument);
			MEMORY_DELETE_EX(msg_info.pMsg);
		}
		mgwlog("退出 CCenterCtrl::DoCatalogCmd\n");
		MEMORY_DELETE_EX(lpTable);
		return 0;
	}
	return 0;
}

//空记录消息
int CCenterCtrl::SendEmptyRecordMsg(CBSIPMSG &stCbSipmsg)
{
	//解析XML指令
	MsgOp oMsgOp;
	oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
	string  strDeviceID;
	strDeviceID = oMsgOp.GetMsgText("DeviceID");
	//构造回复消息
	SIP_MSG_INFO msg_info;
	ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
	sprintf(msg_info.CallerID,"%s",stCbSipmsg.strcallee_id.c_str());  //回复消息调换
	sprintf(msg_info.CalleedID,"%s",stCbSipmsg.strcaller_id.c_str());
	//创建一个XML的文档对象。
	TiXmlDocument *myDocument = new TiXmlDocument();
	if(!myDocument)
	{
		MEMORY_DELETE_EX(msg_info.pMsg);
		return -1;
	}
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
	myDocument->LinkEndChild(decl);
	
	//创建一个根元素(消息类型)并连接。
	TiXmlElement *RootElement = new TiXmlElement("Response");
	myDocument->LinkEndChild(RootElement);
	
	//创建一个CmdType元素并连接
	TiXmlElement *xmlNode = new TiXmlElement("CmdType");
	RootElement->LinkEndChild(xmlNode);
	TiXmlText *xmlContent = new TiXmlText("RecordInfo");
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("SN");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText(oMsgOp.GetMsgText("SN").c_str());
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("DeviceID");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText(strDeviceID.c_str());
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("SumNum");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText("0");
	xmlNode->LinkEndChild(xmlContent);
	
	//xml文档内容输出为字符串
	TiXmlPrinter printer;
	myDocument->Accept(&printer);
	string strSipMsg ="";
	strSipMsg = printer.CStr();
	int iLen = strSipMsg.length();
	
	msg_info.pMsg=NULL;
	msg_info.pMsg = new char[iLen+1];    //1024
	memset(msg_info.pMsg,0,iLen+1);
	sprintf(msg_info.pMsg,"%s",printer.CStr());
	msg_info.nMsgLen=strlen(msg_info.pMsg);
	int iRet =-1;
	iRet = CCtrlProOp::Instance()->SendMsg(msg_info);
	MEMORY_DELETE(myDocument);
	MEMORY_DELETE_EX(msg_info.pMsg);
	return 0;
}
#endif

int  CCenterCtrl::DoInviteCmd(CBSIPMSG &stCbSipmsg)
{
	//mgwlog("enter CCenterCtrl::DoInviteCmd\n");
   //解析invite获取 DeviceID tsu:ip port
	/* 3、获取来源的sdp信息，根据其中的s字段，判断业务类型 */
#ifdef SIP_NEW
	sdp_message_t* pRemoteSDP = NULL;
#else
	sdp_t* pRemoteSDP = NULL;
#endif
    pRemoteSDP = SIP_GetInviteDialogRemoteSDP(stCbSipmsg.idialog_index);
    if (NULL == pRemoteSDP)
    {
        SIP_AnswerToInvite(stCbSipmsg.idialog_index, 503, (char*)"Get Remote SDP Error");
        mgwlog("Get Remote SDP Error\n");
		return -1;
    }

	unsigned long addr;
    int port = 0;
    int code = 0;
    int flag = 0;
	int i = SIP_GetSDPVideoInfo(pRemoteSDP, &addr, &port, &code, &flag);
	if (i != 0)
    {
        SIP_AnswerToInvite(stCbSipmsg.idialog_index, 503, (char*)"Get SDPVideoInfo Error");
        mgwlog("Get SDPVideoInfo Error\n");
		return -1;
    }

	//---------added by chenyu begin-----------------------
    /* 解析sdp中的S name 信息*/
    INVITE_TYPE eInviteType = INVITE_PLAY;   //实时视频
	char* s_name = NULL;
	int start_time =0,end_time=0,play_time=0;
    s_name = sdp_message_s_name_get(pRemoteSDP);
    if (NULL == s_name)
    {
        sdp_message_free(pRemoteSDP);
        pRemoteSDP = NULL;
        SIP_AnswerToInvite(stCbSipmsg.idialog_index, 503, (char*)"Get SDP S Name Error");
        return -1;
    }
    if (0 == strncmp(s_name, "Playback", 8))
    {
        eInviteType = INVITE_PLAYBACK;
		/* 解析sdp中的时间信息*/
		i = get_sdp_t_time(pRemoteSDP, &start_time, &end_time, &play_time);
		if (i != 0)
		{
			sdp_message_free(pRemoteSDP);
            pRemoteSDP = NULL;
			SIP_AnswerToInvite(stCbSipmsg.idialog_index, 503, (char*)"get_sdp_t_time Error");
			mgwlog("get_sdp_t_time Error\n");
			return -1;
		}
		mgwlog("解析invite SDP,类型:PlayBack\n");
    }
    else if (0 == strncmp(s_name, "Play", 4))
    {
        eInviteType = INVITE_PLAY;
		mgwlog("解析invite SDP,类型:Play\n");
    }
    else
    {
        sdp_message_free(pRemoteSDP);
        pRemoteSDP = NULL;
        SIP_AnswerToInvite(stCbSipmsg.idialog_index, 488, (char*)"SDP S Type Not Support");
        return -1;
    }
	//---------added by chenyu end-----------------------

	unsigned short usPort=0;
	char szPort[10]={0};
	usPort = CCtrlProOp::Instance()->GetFreePort();
	sprintf(szPort,"%u",usPort);

	//生成回复消息
#ifdef SIP_NEW
	sdp_message_t* pLocalSDP = NULL;
#else
	sdp_t* pLocalSDP = NULL;
#endif

	char *pcLocalSDPPort=szPort;
	//char *localip = (char *)CSysCfg::Instance()->GetstrPara(SYS_CFG_LOCALIP_IN).c_str();
	char *localip = (char *)CSysCfg::Instance()->GetstrPara(SYS_CFG_LOCALIP_INTERCOM).c_str();
	int  iCodeType = EV9000_STREAMDATA_TYPE_PS;   //0--ps 2--h.264 4--HIK1  5--HIK2
    UNGB_CHCONFIGREC stChCfg;
	if(!CSysCfg::Instance()->GetOneChCfg(stCbSipmsg.strcallee_id,stChCfg)) //未找到
	{
		mgwlog("strcallee_id:[%s] Channel not Found\n",stCbSipmsg.strcallee_id.c_str());
		SIP_AnswerToInvite(stCbSipmsg.idialog_index, 404, (char*)"Channel not Found");
		return -1;  //没有找到指定设备
	}

#ifdef WIN32
// 	if(stChCfg.iNeedCodec) 
// 	{
// 		iCodeType = EV9000_STREAMDATA_TYPE_VIDEO_H264;  //编解码后，码流类型为h264
// 	}else{
// 		//iCodeType = stChCfg.iStreamType;          //不编解码，就是通道的输出码流类型
//         iCodeType = 0;                              //ps流
// 	}
//	iCodeType = 0;                                 //ps流
	iCodeType = EV9000_STREAMDATA_TYPE_PS;         //正常输出码流均为ps流
    mgwlog("通道strcallee_id:%s 流类型iCodeType:%d\n",stCbSipmsg.strcallee_id.c_str(),iCodeType);
    i = SIP_GeneratingSDPAnswer(stCbSipmsg.idialog_index, &pLocalSDP, NULL, pcLocalSDPPort, 
		localip, (char*)"Play", 0, 0,0, 1, -1, iCodeType);
#else
    bool bFindInfo = false;
    string strUsr,strPwd,strIP,strMapChannel;
	int iChannelPort = 0;  //rtsp 服务端口
    if(m_pDevCtrl)
	{
		CMediaItemDevice *lpDevice = NULL;
		if( 0 == m_pDevCtrl->FindDevice(stCbSipmsg.strcallee_id,lpDevice))  //找到
		{
			if(lpDevice)
			{
				strUsr = lpDevice->GetDevUserName();   //m_stDevConfig.strUseName;
				strPwd = lpDevice->GetDevPwd();        //m_stDevConfig.strPassword;
				strIP = lpDevice->GetDevIP();          //m_stDevConfig.strDeviceIP;
				strMapChannel = stChCfg.strMapChannel;
				iChannelPort = stChCfg.iChannelPort;
				bFindInfo = true;
			}
		}
	}
    if(false == bFindInfo)
    {
    	SIP_AnswerToInvite(stCbSipmsg.idialog_index, 488, NULL);
    	mgwlog("unready fail ack:488 \n");
    	return -1;
    }
    _sdp_extend_param_t* pExPara=NULL;
    pExPara = new _sdp_extend_param_t();
    memset(pExPara->onvif_url,0,256);
    sprintf(pExPara->onvif_url,"%s:%s@rtsp://%s:%d%s",
		strUsr.c_str(),strPwd.c_str(),strIP.c_str(),iChannelPort,strMapChannel.c_str());
    mgwlog("to tsu Onvif_url:%s\n",pExPara->onvif_url);
    i = SIP_GeneratingSDPAnswer(stCbSipmsg.idialog_index, &pLocalSDP, NULL, pcLocalSDPPort,
  		localip, (char*)"Play", 0, 0,0, 1, -1, iCodeType,pExPara);
    if(pExPara)
    {
    	delete pExPara;
    }
#endif
	//mgwlog("DoInviteCmd i=%d\n",i);
	if ((i != 0) || (NULL == pLocalSDP))
	{
		if(i!=0)
		{
			mgwlog("SIP_GeneratingSDPAnswer fail i!=0 \n");
		}
		if(NULL == pLocalSDP)
		{
			mgwlog("SIP_GeneratingSDPAnswer fail NULL == pLocalSDP \n");
		}
		SIP_AnswerToInvite(stCbSipmsg.idialog_index, 488, NULL);
		mgwlog("fail ack:488 \n");
		return -1;
	}
	
	//创建指定的设备通道
	if(m_pDevCtrl)
	{
		STREAM_CON_INFO stStreamConInfo;
		stStreamConInfo.strLogicDeviceID = stCbSipmsg.strcallee_id;
		stStreamConInfo.dwDestIP = addr;
		stStreamConInfo.iDestPort = port;
		stStreamConInfo.iStartTime = start_time;
		stStreamConInfo.iStopTime = end_time;
		if (play_time<=0)
		{
            play_time = start_time;
		}
		stStreamConInfo.iPlayTime = play_time;
		DWORD dwIP=0;
#ifdef WIN32
		SockObj::ConvertStringToIP(&dwIP,(char*)CSysCfg::Instance()->GetstrPara(SYS_CFG_LOCALIP_INTERCOM).c_str());
#else
		dwIP = iptoint((char*)CSysCfg::Instance()->GetstrPara(SYS_CFG_LOCALIP_INTERCOM).c_str());
#endif
		stStreamConInfo.dwSrcIP = dwIP;
		stStreamConInfo.iSrcPort = usPort;
		stStreamConInfo.stSipContext = stCbSipmsg;
		int nRet = 0 ;
		if(INVITE_PLAY == eInviteType)  //实时视频
		{
			mgwlog("开始打开unit:%s\n",stCbSipmsg.strcallee_id.c_str());
            nRet = m_pDevCtrl->OpenReal(stStreamConInfo,UNIT_OPEN_COM);
		}else{
#ifdef WIN32
        	mgwlog("开始打开录像unit:%s starttime:%d endtime:%d playtime:%d\n",
				   stCbSipmsg.strcallee_id.c_str(),start_time,end_time,play_time);
			nRet = m_pDevCtrl->PlayBackOpen(stStreamConInfo,UNIT_OPEN_COM);
			if(!nRet)
			{
				mgwlog("m_pDevCtrl->PlayBackOpen success\n");
			}
#endif
		}
		if (nRet !=0 )  //打开失败
		{
			SIP_AnswerToInvite(stCbSipmsg.idialog_index, 503, (char*)"Open Channel Fail");
			mgwlog("Open Channel Fail \n");
			return -1;
		}
	}

	/* 9、回应200 ok消息给主叫侧*/
	//modified by chenyu 131214
	/*
	*需在编译命令行中加入 /EHa 参数,
	*这样VC编译器就不会把没有throw的try_catch模块优化掉
	*/
	static int iExceptionCount=0;  //异常计数
	try
	{
		i = SIP_AcceptInvite(stCbSipmsg.idialog_index, pLocalSDP);
		CCtrlProOp::Instance()->RecordDialogIndex(stCbSipmsg.idialog_index,stCbSipmsg.strcallee_id);
		mgwlog("CCtrlProOp::Instance()->RecordDialogIndex,插入对象记录 idialog_index:%d\n",stCbSipmsg.idialog_index);
	}
	catch (...)
	{
		i=-1;  // 异常
		iExceptionCount++;  //计数
		if(iExceptionCount > MAX_EXCEPTION_NUM)
		{
			mgwlog("Exception: CCenterCtrl::DoInviteCmd 异常超过100次，退出....\n");
			exit(-1);
		}
		mgwlog("Exception: CCenterCtrl::DoInviteCmd SIP_AcceptInvite,异常次数:%d\n",iExceptionCount);
	}

	if (i != 0)
	{
		SIP_AnswerToInvite(stCbSipmsg.idialog_index, 503, (char*)"Accept Invite Error");
		mgwlog("Accept Invite Error \n");
		return -1;
	}
    return 0;
}

int CCenterCtrl::DoInform(CBSIPMSG &stCbSipmsg)
{
	int nRet = 0; 
#ifdef SUP_PLAYBACK
	//创建指定的设备通道
	if(!m_pDevCtrl)
	{
		//SIP_AnswerToInvite(stCbSipmsg.idialog_index, 503, (char*)"Open Channel Fail");
		mgwlog("DoInform Fail\n");
		return -1;
	}
	
    //根据RTSP中的信息
	char* msg_body = (char*)stCbSipmsg.strMsg.c_str();
	if (0 == strncmp(msg_body, "PLAY", 4))
	{
		nRet = m_pDevCtrl->PlayBackCtrl(stCbSipmsg,RECORD_CTRL_RESUME,0,NULL);
	}
	else if (0 == strncmp(msg_body, "PAUSE", 5))
	{
		//mgwlog("----收到服务器 pause 命令-------\n");
		nRet = m_pDevCtrl->PlayBackCtrl(stCbSipmsg,RECORD_CTRL_PAUSE,0,NULL);
	}
	else if (0 == strncmp(msg_body, "TEARDOWN", 8))
	{
		mgwlog("CCenterCtrl::DoInform PlayBackStop\n");
		nRet = m_pDevCtrl->PlayBackStop(stCbSipmsg);
	}
	else if (0 == strncmp(msg_body, "SEEK", 4))
	{
        //用find来解析
		string strRange = "";
		int bpos = stCbSipmsg.strMsg.find("Range:npt=");
		if(bpos==string::npos)
		{
			mgwlog("find Range开始 Fail\n");
			return -1;
		}
		int epos = stCbSipmsg.strMsg.find("\r\n",bpos+10);
		if(epos==string::npos)
		{
			mgwlog("find Range结束 Fail\n");
			return -1;
		}
        if(epos>=bpos)
		{
			strRange = stCbSipmsg.strMsg.substr(bpos+10, epos-(bpos+10));
		}
        mgwlog("SEEK time strRange:%s\n",strRange.c_str());
		unsigned int time = atoi(strRange.c_str());
        nRet = m_pDevCtrl->PlayBackCtrl(stCbSipmsg,RECORD_CTRL_SETPLAYPOS,time,NULL);
		/* 解析MANSRTSP 消息 */
		// 		int i=0;
		// 		i = mansrtsp_init(&rtsp);
		// 		if (i != 0)
		// 		{
		// 			return -1;
		// 		}
		// 		i = mansrtsp_parse(rtsp, msg_body);
		// 		if (i != 0)
		// 		{
		// 			mansrtsp_free(rtsp);
		// 			osip_free(rtsp);
		// 			rtsp = NULL;
		// 			return -1;
		// 		}
		// 		if (is_device_ip_used() && '\0' != pCrData->tsu_device_ip[0])
		// 		{
		// 			i = notify_tsu_seek_replay(pCrData->tsu_device_ip, pCrData->task_id, osip_atoi(rtsp->range->start));
		// 		}else{
		// 			i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, osip_atoi(rtsp->range->start));
		// 		}
		// 		mansrtsp_free(rtsp);
		// 		osip_free(rtsp);
		// 		rtsp = NULL;
	}
#endif
	return nRet;
}

int  CCenterCtrl::DoByeCmd(CBSIPMSG &stCbSipmsg)
{
	//mgwlog("enter CCenterCtrl::DoByeCmd");
	//关闭指定的设备通道
	if(m_pDevCtrl)
	{
		mgwlog("开始关闭unit:%s \n",stCbSipmsg.strcallee_id.c_str());
		m_pDevCtrl->Close(stCbSipmsg,FALSE);
		CCtrlProOp::Instance()->DelDialogIndex(stCbSipmsg.idialog_index);
	}

	SIP_AnswerToBye(stCbSipmsg.idialog_index, 200, NULL);
    return 0;
}

int  CCenterCtrl::DoDeviceControlCmd(CBSIPMSG &stCbSipmsg)
{
	//mgwlog("CCenterCtrl::DoDeviceControlCmd caller:%s\n",stCbSipmsg.strcaller_id.c_str());
	if(m_pDevCtrl)
	{
		//命令转换
		//解析XML指令
		MsgOp oMsgOp;
		oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
		string  strDeviceID,strPTZCmd;
		strDeviceID = oMsgOp.GetMsgText("DeviceID");
        strPTZCmd = oMsgOp.GetMsgText("PTZCmd");
		mgwlog("::::PTZCtrl:%s",strPTZCmd.c_str());
		m_pDevCtrl->PTZCtrl(strDeviceID,strPTZCmd);
	}
	return 0;
}

//处理DeviceInfo命令
int CCenterCtrl::DoDeviceInfoCmd(CBSIPMSG &stCbSipmsg)
{
	//mgwlog("enter CCenterCtrl::DoDeviceInfoCmd\n");
	//构造回复消息
	SIP_MSG_INFO msg_info;
	ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
	sprintf(msg_info.CallerID,"%s",stCbSipmsg.strcallee_id.c_str());  //回复消息调换
	sprintf(msg_info.CalleedID,"%s",stCbSipmsg.strcaller_id.c_str());
	
	MsgOp oMsgOp;
	oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
	
	//创建一个XML的文档对象。
	TiXmlDocument *myDocument = new TiXmlDocument();
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
	myDocument->LinkEndChild(decl);
	
	//创建一个根元素(消息类型)并连接。
	TiXmlElement *RootElement = new TiXmlElement("Response");
	myDocument->LinkEndChild(RootElement);
	
	//创建一个CmdType元素并连接
	TiXmlElement *xmlNode = new TiXmlElement("CmdType");
	RootElement->LinkEndChild(xmlNode);
	TiXmlText *xmlContent = new TiXmlText("DeviceInfo");
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("SN");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText(oMsgOp.GetMsgText("SN").c_str());
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("DeviceID");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText(oMsgOp.GetMsgText("DeviceID").c_str());
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("Result");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText("OK");
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("DeviceType");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText("MGW");
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("Manufacturer");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText("Test");
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("Model");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText("WisMGWEV9000");
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("Firmware");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText("V9000");
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("MaxCamera");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText("1");
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("MaxAlarm");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText("1");
	xmlNode->LinkEndChild(xmlContent);
	
	//xml文档内容输出为字符串
	TiXmlPrinter printer;
	myDocument->Accept(&printer);
	string strSipMsg ="";
	strSipMsg = printer.CStr();
	int iLen = strSipMsg.length();
	if(iLen<512)
	{
       // mgwlog("::::make xmlmsg:\n%s\n",strSipMsg.c_str());
	}else{
//		mgwlog("msg is too large\n");
		// 		static ofstream  ofNewFile;
		// 		if (!ofNewFile.is_open())
		// 		{
		// 			ofNewFile.open("E:\\catalog.txt",	ios::trunc | ios::binary);
		// 			ofNewFile<<strSipMsg<<endl;
		// 			ofNewFile.flush();
		// 			ofNewFile.close();
		// 		}
	}	
	
	msg_info.pMsg=NULL;
	msg_info.pMsg = new char[iLen+1];    //1024
	memset(msg_info.pMsg,0,iLen+1);
	sprintf(msg_info.pMsg,"%s",printer.CStr());
	msg_info.nMsgLen=strlen(msg_info.pMsg);
	int iRet =-1;
	iRet = CCtrlProOp::Instance()->SendMsg(msg_info);
	MEMORY_DELETE(myDocument);
	MEMORY_DELETE_EX(msg_info.pMsg);
	return iRet;
}

int CCenterCtrl::DoSetDeviceVideoParamCmd(CBSIPMSG &stCbSipmsg)
{
	if(m_pDevCtrl)
	{
		//解析XML指令
		MsgOp oMsgOp;
		oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
		string  strDeviceID;
		string  strBrightness,strSaturation,strContrast,strColourDegree;
		strDeviceID = oMsgOp.GetMsgText("DeviceID");
		strBrightness = oMsgOp.GetMsgText("Brightness");
		strSaturation = oMsgOp.GetMsgText("Saturation");
		strContrast = oMsgOp.GetMsgText("Contrast");
        strColourDegree = oMsgOp.GetMsgText("ColourDegree");
		m_pDevCtrl->SetVideoParam(strDeviceID,"Brightness",strBrightness);
        m_pDevCtrl->SetVideoParam(strDeviceID,"Saturation",strSaturation);
		m_pDevCtrl->SetVideoParam(strDeviceID,"Contrast",strContrast);
		m_pDevCtrl->SetVideoParam(strDeviceID,"ColourDegree",strColourDegree);
        mgwlog("SetDeviceVideoParam 色度 %s 亮度 %s 对比度 %s 饱和 %s\n",
			strColourDegree.c_str(),strBrightness.c_str(),strContrast.c_str(),strSaturation.c_str());
		//构造回复消息
		SIP_MSG_INFO msg_info;
		ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
		sprintf(msg_info.CallerID,"%s",stCbSipmsg.strcallee_id.c_str());  //回复消息调换
		sprintf(msg_info.CalleedID,"%s",stCbSipmsg.strcaller_id.c_str());
		
		//创建一个XML的文档对象。
		TiXmlDocument *myDocument = new TiXmlDocument();
		TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
		myDocument->LinkEndChild(decl);
		
		//创建一个根元素(消息类型)并连接。
		TiXmlElement *RootElement = new TiXmlElement("Response");
		myDocument->LinkEndChild(RootElement);
		
		//创建一个CmdType元素并连接
		TiXmlElement *xmlNode = new TiXmlElement("CmdType");
		RootElement->LinkEndChild(xmlNode);
		TiXmlText *xmlContent = new TiXmlText("SetDeviceVideoParam");
		xmlNode->LinkEndChild(xmlContent);
		
		xmlNode = new TiXmlElement("SN");
		RootElement->LinkEndChild(xmlNode);
		xmlContent = new TiXmlText(oMsgOp.GetMsgText("SN").c_str());
		xmlNode->LinkEndChild(xmlContent);
		
		xmlNode = new TiXmlElement("DeviceID");
		RootElement->LinkEndChild(xmlNode);
		xmlContent = new TiXmlText(oMsgOp.GetMsgText("DeviceID").c_str());
		xmlNode->LinkEndChild(xmlContent);
		
		xmlNode = new TiXmlElement("Result");
		RootElement->LinkEndChild(xmlNode);
		xmlContent = new TiXmlText("OK");
		xmlNode->LinkEndChild(xmlContent);
		
		//xml文档内容输出为字符串
		TiXmlPrinter printer;
		myDocument->Accept(&printer);
		string strSipMsg ="";
		strSipMsg = printer.CStr();
		int iLen = strSipMsg.length();
		if(iLen<512)
		{
			//mgwlog("::::make xmlmsg:\n%s\n",strSipMsg.c_str());
		}else{
			//		mgwlog("msg is too large\n");
			// 		static ofstream  ofNewFile;
			// 		if (!ofNewFile.is_open())
			// 		{
			// 			ofNewFile.open("E:\\catalog.txt",	ios::trunc | ios::binary);
			// 			ofNewFile<<strSipMsg<<endl;
			// 			ofNewFile.flush();
			// 			ofNewFile.close();
			// 		}
		}	
		msg_info.pMsg=NULL;
		msg_info.pMsg = new char[iLen+1];    //1024
		memset(msg_info.pMsg,0,iLen+1);
		sprintf(msg_info.pMsg,"%s",printer.CStr());
		msg_info.nMsgLen=strlen(msg_info.pMsg);
		int iRet =-1;
		iRet = CCtrlProOp::Instance()->SendMsg(msg_info);
		MEMORY_DELETE(myDocument);
		MEMORY_DELETE_EX(msg_info.pMsg);
	    return iRet;
	}
	return 0;
}

int CCenterCtrl::DoGetDeviceVideoParamCmd(CBSIPMSG &stCbSipmsg)
{
	if(m_pDevCtrl)
	{
		//解析XML指令
		MsgOp oMsgOp;
		oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
		string strDeviceID ="";
		strDeviceID = oMsgOp.GetMsgText("DeviceID");
		string  strColourDegree,strBrightness,strContrast,strSaturation;
		unsigned int nParam=0;
		m_pDevCtrl->GetVideoParam(strDeviceID,nParam);
		//mgwlog("CCenterCtrl::DoGetDeviceVideoParamCmd 颜色参数nParam:0x%x\n",nParam);
        //转换为0-255数字
		DWORD dwBrightValue=0, dwContrastValue=0, dwSaturationValue=0, dwHueValue=0;
		dwSaturationValue = nParam & 0x000000ff;
        dwContrastValue = (nParam>>8) & 0x000000ff;
        dwBrightValue = (nParam>>16) & 0x000000ff;
        dwHueValue = (nParam>>24) & 0x000000ff;
		char szValue[10]={0};
		memset(szValue,0,10);
		sprintf(szValue,"%u",dwHueValue);
		strColourDegree = szValue;
		memset(szValue,0,10);
		sprintf(szValue,"%u",dwBrightValue);
		strBrightness = szValue;
		memset(szValue,0,10);
		sprintf(szValue,"%u",dwContrastValue);
		strContrast = szValue;
		memset(szValue,0,10);
		sprintf(szValue,"%u",dwSaturationValue);
		strSaturation = szValue;
		mgwlog("GetDeviceVideoParam 色度 %s 亮度 %s 对比度 %s 饱和 %s\n",
			strColourDegree.c_str(),strBrightness.c_str(),strContrast.c_str(),strSaturation.c_str());
		//构造回复消息
		SIP_MSG_INFO msg_info;
		ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
		sprintf(msg_info.CallerID,"%s",stCbSipmsg.strcallee_id.c_str());  //回复消息调换
		sprintf(msg_info.CalleedID,"%s",stCbSipmsg.strcaller_id.c_str());
		
		//创建一个XML的文档对象。
		TiXmlDocument *myDocument = new TiXmlDocument();
		TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
		myDocument->LinkEndChild(decl);
		
		//创建一个根元素(消息类型)并连接。
		TiXmlElement *RootElement = new TiXmlElement("Response");
		myDocument->LinkEndChild(RootElement);
		
		//创建一个CmdType元素并连接
		TiXmlElement *xmlNode = new TiXmlElement("CmdType");
		RootElement->LinkEndChild(xmlNode);
		TiXmlText *xmlContent = new TiXmlText("SetDeviceVideoParam");
		xmlNode->LinkEndChild(xmlContent);
		
		xmlNode = new TiXmlElement("SN");
		RootElement->LinkEndChild(xmlNode);
		xmlContent = new TiXmlText(oMsgOp.GetMsgText("SN").c_str());
		xmlNode->LinkEndChild(xmlContent);
		
		xmlNode = new TiXmlElement("DeviceID");
		RootElement->LinkEndChild(xmlNode);
		xmlContent = new TiXmlText(oMsgOp.GetMsgText("DeviceID").c_str());
		xmlNode->LinkEndChild(xmlContent);
		
		xmlNode = new TiXmlElement("Brightness");
		RootElement->LinkEndChild(xmlNode);
		xmlContent = new TiXmlText(strBrightness.c_str());
		xmlNode->LinkEndChild(xmlContent);
		
		xmlNode = new TiXmlElement("Saturation");
		RootElement->LinkEndChild(xmlNode);
		xmlContent = new TiXmlText(strSaturation.c_str());
		xmlNode->LinkEndChild(xmlContent);
		
		xmlNode = new TiXmlElement("Contrast");
		RootElement->LinkEndChild(xmlNode);
		xmlContent = new TiXmlText(strContrast.c_str());
		xmlNode->LinkEndChild(xmlContent);
		
		xmlNode = new TiXmlElement("ColourDegree");
		RootElement->LinkEndChild(xmlNode);
		xmlContent = new TiXmlText(strColourDegree.c_str());
		xmlNode->LinkEndChild(xmlContent);
		
		//xml文档内容输出为字符串
		TiXmlPrinter printer;
		myDocument->Accept(&printer);
		string strSipMsg ="";
		strSipMsg = printer.CStr();
		int iLen = strSipMsg.length();
		if(iLen<512)
		{
			//mgwlog("::::make xmlmsg:\n%s\n",strSipMsg.c_str());
		}else{
			//		mgwlog("msg is too large\n");
			// 		static ofstream  ofNewFile;
			// 		if (!ofNewFile.is_open())
			// 		{
			// 			ofNewFile.open("E:\\catalog.txt",	ios::trunc | ios::binary);
			// 			ofNewFile<<strSipMsg<<endl;
			// 			ofNewFile.flush();
			// 			ofNewFile.close();
			// 		}
		}	
		msg_info.pMsg=NULL;
		msg_info.pMsg = new char[iLen+1];    //1024
		memset(msg_info.pMsg,0,iLen+1);
		sprintf(msg_info.pMsg,"%s",printer.CStr());
		msg_info.nMsgLen=strlen(msg_info.pMsg);
		int iRet =-1;
		iRet = CCtrlProOp::Instance()->SendMsg(msg_info);
		MEMORY_DELETE(myDocument);
		MEMORY_DELETE_EX(msg_info.pMsg);
		return iRet;
	}
	return 0;
}

int CCenterCtrl::DoRequestIFrameDataCmd(CBSIPMSG &stCbSipmsg)
{
	//mgwlog("CCenterCtrl::DoRequestIFrameDataCmd caller:%s\n",stCbSipmsg.strcaller_id.c_str());
	if(m_pDevCtrl)
	{
		MsgOp oMsgOp;
		oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
		string  strDeviceID;
		strDeviceID = oMsgOp.GetMsgText("DeviceID");
		m_pDevCtrl->MakeKeyFrame(strDeviceID);
	}
	
	return 0;
}

int CCenterCtrl::DoAutoZoomInCmd(CBSIPMSG &stCbSipmsg)
{
	//mgwlog("CCenterCtrl::DoAutoZoomInCmd caller:%s\n",stCbSipmsg.strcaller_id.c_str());
	if(m_pDevCtrl)
	{
		//命令转换
		//解析XML指令
		MsgOp oMsgOp;
		oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
		string  strDeviceID;
		AutoZoomInPara stAutoZoomInPara;
		strDeviceID = oMsgOp.GetMsgText("DeviceID");
        stAutoZoomInPara.strDisplayFrameWide = oMsgOp.GetMsgText("DisplayFrameWide");
        stAutoZoomInPara.strDisplayFrameHigh = oMsgOp.GetMsgText("DisplayFrameHigh");
		stAutoZoomInPara.strDestMatrixTopLeftX = oMsgOp.GetMsgText("DestMatrixTopLeftX");
        stAutoZoomInPara.strDestMatrixTopLeftY = oMsgOp.GetMsgText("DestMatrixTopLeftY");
		stAutoZoomInPara.strDestMatrixWide = oMsgOp.GetMsgText("DestMatrixWide");
        stAutoZoomInPara.strDestMatrixHigh = oMsgOp.GetMsgText("DestMatrixHigh");
		m_pDevCtrl->PTZCtrl(strDeviceID,stAutoZoomInPara);
	}
	return 0;
}
