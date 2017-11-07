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
	//��ʼ����ģ��
	if(CSysCfg::Instance()->Init()!=0)  //��ʼ������ʧ��
	{
		mgwlog("��ʼ������ʧ��\n");
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
	if (!m_lpInfoUdp)    //�ռ�
	{
		m_lpInfoUdp = new CUdp2();
		if (!m_lpInfoUdp->Open(DM_INFO_RECV_PORT, "127.0.0.1"))
		{
			mgwlog("����Ϣ���ն˿�[%d]ʧ��", DM_INFO_RECV_PORT);
			return FALSE;
		}
		mgwlog("����Ϣ���ն˿� %d �ɹ�", DM_INFO_RECV_PORT);
	}
	
	if (!m_bThreadInfoRecvFlag)  //�����ռ���Ϣ�߳�
	{
		m_bThreadInfoRecvFlag = TRUE;
		m_hThreadInfoRecv = CreateThread(NULL, 0, ThreadInfoRecv, (LPVOID)this, 0, &m_dwThreadInfoRecvID);
		if (!m_hThreadInfoRecv)
		{//�����߳�ʧ��
			mgwlog("������Ϣ�����߳�ʧ��\n");
			return FALSE;
		}
		mgwlog("������Ϣ�����߳�[0x%x]", m_dwThreadInfoRecvID);
	}
#endif
	m_strMgwDeviceID = CSysCfg::Instance()->GetstrPara(SYS_CFG_MGWDEVID);   //�������л�ȡ
	InitDevice();          //��ʼ��device
    StartDealMsg();        //��������߳�
    Register();            //��CMSע��
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
	//�ر���Ϣ�����߳�
	for (int iCount=0; iCount<THREAD_DEALMSG_MAX; iCount++)
	{
#ifdef WIN32
		if(m_hThreadDealMsg[iCount])
		{
			m_bThreadDealMsgFlag[iCount] = FALSE;
			WaitForSingleObject(m_hThreadDealMsg[iCount], INFINITE);
			CLOSE_HANDLE(m_hThreadDealMsg[iCount]);
			m_dwThreadDealMsgID[iCount] = 0;
			
			mgwlog("�ر���Ϣ�����߳�,ID:[%d]\n",iCount);
		}
#else
		if(m_threadDealMsgHandle[iCount])
		{
			m_bThreadDealMsgFlag[iCount] = FALSE;
			pthread_cancel(m_threadDealMsgHandle[iCount]);
			pthread_join(m_threadDealMsgHandle[iCount], NULL);
			m_threadDealMsgHandle[iCount] = 0;
			mgwlog("�ر���Ϣ�����߳�,ID:[%d]\n",iCount);
		}
#endif
	}

	//�ر�ˢ���߳�
#ifdef WIN32
	if(m_hThreadRefresh)
    {
		m_bThreadRefreshFlag = FALSE;
		WaitForSingleObject(m_hThreadRefresh, INFINITE);
        CLOSE_HANDLE(m_hThreadRefresh);
		m_dwThreadRefreshID = 0;
	}

	if(m_hThreadInfoRecv)
    {//����UDP��Ϣ�����߳�
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
	MEMORY_DELETE(m_pDevCtrl);    //�ر��豸����ģ��
	return TRUE;
}

//���ݲ�ѯ�������̬�����豸sdk�����豸��
BOOL CCenterCtrl::InitDevice()
{
	if(m_pDevCtrl)
	{
		return ( 0 == m_pDevCtrl->Start() );
	}else{
		return FALSE;
	}
}

//�豸�ر�
BOOL CCenterCtrl::FiniDevice()
{
	if(m_pDevCtrl)
	{
		return ( 0 == m_pDevCtrl->Close() );
	}else{
		return FALSE;
	}
}          

//��CMSע��
BOOL CCenterCtrl::Register()
{
	mgwlog("��ʼ��CMSע��\n");
	EV9000_LOGININFO loginInfo;
	sprintf(loginInfo.sServerIP,CSysCfg::Instance()->GetstrPara(SYS_CFG_CMSIP).c_str());             // ������IP
	loginInfo.nServerPort=CSysCfg::Instance()->GetdwPara(SYS_CFG_SERVERSIPPORT);                                 // �������˿�
	sprintf(loginInfo.sUserName,CSysCfg::Instance()->GetstrPara(SYS_CFG_USERNAME).c_str());        //[EV9000_NORMAL_STRING_LEN];
	sprintf(loginInfo.sUserPwd,CSysCfg::Instance()->GetstrPara(SYS_CFG_USERPWD).c_str());                //[EV9000_NORMAL_STRING_LEN];
	sprintf(loginInfo.sServerID,CSysCfg::Instance()->GetstrPara(SYS_CFG_SERVERID).c_str());    //[EV9000_NORMAL_STRING_LEN];             // ���������
	sprintf(loginInfo.sUserID,(char *)CSysCfg::Instance()->GetstrPara(SYS_CFG_MGWDEVID).c_str());      //[EV9000_NORMAL_STRING_LEN];
    //sprintf(loginInfo.sLocalIP,CSysCfg::Instance()->GetstrPara(SYS_CFG_LOCALIP_IN).c_str());            //[EV9000_NORMAL_STRING_LEN];
	sprintf(loginInfo.sLocalIP,CSysCfg::Instance()->GetstrPara(SYS_CFG_LOCALIP_INTERCOM).c_str());            //[EV9000_NORMAL_STRING_LEN]; 
	mgwlog("��¼��ϢServerIP:%s ServerPort:%d \nServerID:%s UserName:%s UserPwd:%s \nUserID:%s LocalIP:%s\n",
			loginInfo.sServerIP,loginInfo.nServerPort,loginInfo.sServerID,
			loginInfo.sUserName,loginInfo.sUserPwd,loginInfo.sUserID,loginInfo.sLocalIP);
	loginInfo.nDigital =0; 	// �Ƿ�ʹ������֤���¼ 0 ��ʹ�� 1 ʹ��
    CCtrlProOp::Instance()->SetLoginInfo(loginInfo);
	int nRet = CCtrlProOp::Instance()->Register();
    if(nRet !=0)
	{
		mgwlog("��CMSע��ʧ��\n");
	}
	
    mgwlog("����ˢ���߳�\n");
#ifdef WIN32
	if(!m_bThreadRefreshFlag)  //����ˢ���߳�
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

//����SIP Message�����߳�
BOOL CCenterCtrl::StartDealMsg()
{
   	//���߳�
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
		mgwlog("����Ϣ�����߳�:%d\n",i);
	}
	return TRUE;
}

//������
//��Ϣ����
//�Ǽ�ʱ������ͨ����
//��ʱ����1�� �������� 
//��ʱ����2�� Invite����
//�������  ��ȡ���� ��I֡
//��������ǰ�ж��豸״̬��
//���ദ�� ÿ����̴߳���

// Sip�������
int	CCenterCtrl::OnDealMsg(int iHandle)
{
	mgwlog("����Sip�����������ʼ������Ϣ���߳�ID:%d\n",iHandle);
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
			//mgwlog("��������Ϣ���г���:%d",CCtrlProOp::Instance()->GetMsgLen());
			continue;
		}
		
		DealMsg(stCbSipmsg);
	}
	mgwlog("�˳�Sip�������\n");
	return 0;	
}

// ע��ˢ�º���
int	CCenterCtrl::OnRefresh()
{
	mgwlog("����ˢ�º���\n");
	int nRegFailNum=0;
	string strTimeOut = CSysCfg::Instance()->GetstrPara(SYS_CFG_SIPREFRESHTIME);
	int iTimeOutDefault = atoi(strTimeOut.c_str());
	if(iTimeOutDefault < MIN_SIP_TIMEOUT)
	{
          iTimeOutDefault = MAX_SIP_TIMEOUT; 
	}
	int iTimeOut = MAX_SIP_TIMEOUT;  //��λ s
	iTimeOut = iTimeOutDefault;
    DWORD dwTime = GetTickCount();
	DWORD dwDevRefreshTime = GetTickCount();
	while (m_bThreadRefreshFlag)
	{
		if((GetTickCount()-dwTime)>= iTimeOut*1000)
		{
			mgwlog("----����ˢ����Ϣ,ʱ����:%ld----\n",iTimeOut);
			if(CCtrlProOp::Instance()->RefreshReg()!=0)
			{
				nRegFailNum ++;
				if(nRegFailNum >3)
				{
					mgwlog("----Info:ˢ��ע��ʧ��----\n");
					CCenterCtrl::Register();
					nRegFailNum =0;
				}
				iTimeOut = iTimeOutDefault/2;   //ʧ�����̳�ʱ
			}else{
				mgwlog("----Info:ˢ��ע��ɹ�----\n");
				iTimeOut = iTimeOutDefault;
			}
			dwTime = GetTickCount();  //����ˢ��ʱ��
		}

#ifdef WIN32
		Sleep(2000);
#else
		if((GetTickCount()-dwDevRefreshTime)>= REFRESH_DEV_TIME*1000)  //10 min
		{
			mgwlog("----�����ڴ����ݿ���Ϣ����ʼˢ���豸----\n");
			if(m_pDevCtrl)
			{
				if(0==CSysCfg::Instance()->Init())
				{
					mgwlog("----��ʼˢ���豸----\n"); 
					m_pDevCtrl->OpenAllDev();
					m_pDevCtrl->CheckAllDev(); 
				}
			}
			mgwlog("----����ˢ���豸----\n"); 
			dwDevRefreshTime = GetTickCount();  //����ˢ��ʱ��
		}
		sleep(2);
#endif
	}
	mgwlog("�˳�ˢ�º���\n");
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
						{//������Ϣ
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
					mgwlog("CCenterCtrl::OnInfoRecv() m_lpInfoUdp->Recv ���Ȳ���\n");
				}
			}
		}
		Sleep(10);
	}
	return 0;
}
#endif

//ִ������
int CCenterCtrl::DealMsg(CBSIPMSG &stCbSipmsg)
{
	if(SIP_MSG_MSG == stCbSipmsg.eSipMsgType)  //msg ��Ϣ
	{
		//����XMLָ��
		MsgOp oMsgOp;
		oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
		string  strMsgType,strCmdType;
		strMsgType = oMsgOp.GetMsgType();
		strCmdType = oMsgOp.GetMsgText("CmdType");
		
		//����ָ��
		if(("Query" == strMsgType ) && ("DeviceInfo" == strCmdType))  //��ȡ�豸��Ϣ��Ϣָ��
		{
			mgwlog("----����Query��Ϣ,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoDeviceInfoCmd(stCbSipmsg);
		}
		else if("Catalog" == strCmdType )
		{
			mgwlog("----����Catalog��Ϣ,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoCatalogCmd(stCbSipmsg);
		}
		else if( ("Control" == strMsgType) && (strCmdType == "DeviceControl"))  //��������
		{
			mgwlog("----����Control��Ϣ,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoDeviceControlCmd(stCbSipmsg);
		}
		else if(strCmdType == "PresetConfig")  //����/ɾ��/ִ��ǰ��Ԥ��λ 
		{
			mgwlog("----����PresetConfig��Ϣ,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoDeviceControlCmd(stCbSipmsg);  //�Ϳ����������ͬ
		}
		else if(strCmdType == "SetDeviceVideoParam")  //����ǰ��ͼ�����
		{
			mgwlog("----����SetDeviceVideoParam��Ϣ,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoSetDeviceVideoParamCmd(stCbSipmsg);
		}
		else if(strCmdType == "GetDeviceVideoParam")  //��ȡǰ��ͼ�����
		{
			mgwlog("----����GetDeviceVideoParam��Ϣ,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoGetDeviceVideoParamCmd(stCbSipmsg);
		}
		else if(strCmdType == "RequestIFrameData")  //����I֡
		{
			mgwlog("----����RequestIFrameData��Ϣ,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoRequestIFrameDataCmd(stCbSipmsg);
		}
		else if(strCmdType == "AutoZoomIn")  //����Ŵ�
		{
			mgwlog("----����AutoZoomIn��Ϣ,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
			DoAutoZoomInCmd(stCbSipmsg);
		}
		else if(strCmdType == "RecordInfo")  //����Ŵ�
		{
			mgwlog("----����RecordInfo��Ϣ,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
#ifdef WIN32
			DoRecordQuery(stCbSipmsg);
#endif
		}
		else
		{
			mgwlog("----���ܴ����msg��Ϣָ��\n");
		}
	}
	else if(SIP_MSG_INVITE == stCbSipmsg.eSipMsgType) //invite ��Ϣ
	{
		mgwlog("----����Invite��Ϣ,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
		DoInviteCmd(stCbSipmsg);
	}
	else if((SIP_MSG_BYE == stCbSipmsg.eSipMsgType)||(SIP_MSG_CANCEL == stCbSipmsg.eSipMsgType)) //bye/cancle ��Ϣ
	{
		if(SIP_MSG_CANCEL == stCbSipmsg.eSipMsgType)
		{
			mgwlog("----����Cancel��Ϣ,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
		}else{
			mgwlog("----����bye��Ϣ,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
		}
		DoByeCmd(stCbSipmsg);
	}
	else if((SIP_MSG_EXPIRE == stCbSipmsg.eSipMsgType)) //expire ��Ϣ
	{
		string callee_id;
		CCtrlProOp::Instance()->GetIDByDialogIndex(stCbSipmsg.idialog_index,callee_id);
		stCbSipmsg.strcallee_id = callee_id;
		mgwlog("----����expire��Ϣ,callee_id:%s \n",stCbSipmsg.strcallee_id.c_str());
		DoByeCmd(stCbSipmsg);
	}
	else if((SIP_MSG_INFORM == stCbSipmsg.eSipMsgType)) //inform ��Ϣ
	{
		//mgwlog("----����Inform��Ϣ,caller:%s callee_id:%s \n",stCbSipmsg.strcaller_id.c_str(),stCbSipmsg.strcallee_id.c_str());
		DoInform(stCbSipmsg);
	}
	else
	{
		mgwlog("----���ܴ������Ϣ����\n");
	}
	return 0;
}

//����Catalog����
int CCenterCtrl::DoCatalogCmd(CBSIPMSG &stCbSipmsg)
{
	mgwlog("���� CCenterCtrl::DoCatalogCmd\n");
	int iCount =0;  //�Ѵ���ڵ���
	int iSumNum =0;  //�ڵ�����
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
		//����ظ���Ϣ
		SIP_MSG_INFO msg_info;
		ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
		sprintf(msg_info.CallerID,"%s",stCbSipmsg.strcallee_id.c_str());  //�ظ���Ϣ����
		sprintf(msg_info.CalleedID,"%s",stCbSipmsg.strcaller_id.c_str());
		
		MsgOp oMsgOp;
		oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
		
		//����һ��XML���ĵ�����
		TiXmlDocument *myDocument = new TiXmlDocument();
		if(!myDocument)
		{
			MEMORY_DELETE_EX(msg_info.pMsg);
			return -1;
		}
		TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
		myDocument->LinkEndChild(decl);
		
		//����һ����Ԫ��(��Ϣ����)�����ӡ�
		TiXmlElement *RootElement = new TiXmlElement("Response");
		myDocument->LinkEndChild(RootElement);
		
		//����һ��CmdTypeԪ�ز�����
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
		
		//��ѯ���ݿ��(UnGBPhyDeviceChannelConfig)�õ�ͨ����Ϣ  
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
		
		int iItemNum=0;   //����xml�Ѵ�����Ŀ
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
		
		//xml�ĵ��������Ϊ�ַ���
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
	mgwlog("�˳� CCenterCtrl::DoCatalogCmd\n");
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
	mgwlog("���� CCenterCtrl::DoRecordQuery\n");
	if(m_pDevCtrl)
	{
		//����XMLָ��
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
		int nTableCount = MAX_REC_QUERY_NUM; //��ѯ¼���¼
		EDVDVRRECORDTABLE* lpTable =NULL;
		lpTable = new EDVDVRRECORDTABLE[nTableCount];
		int iTotalCount = 0;
		int nRet = m_pDevCtrl->GetRecordInfo(strDeviceID,sStartTime,sStopTime,lpTable,nTableCount,iTotalCount);
		if (iTotalCount== -1)
		{
			mgwlog("��ѯ��¼���¼ʧ��\n");
			MEMORY_DELETE_EX(lpTable);
			SendEmptyRecordMsg(stCbSipmsg);  //���Ϳ���Ϣ
			return 0;
		}
		mgwlog("��ѯ��¼���¼:%d ��ȡ:%d\n",iTotalCount,nTableCount);
        int iMinCount = 0;
		iMinCount = (iTotalCount<=nTableCount)?iTotalCount:nTableCount;
		
    	int iCount =0;  //�Ѵ���ڵ���
		int iSumNum =0;  //�ڵ�����
		
		iSumNum = iMinCount;
		if(0 == iSumNum)
		{
			MEMORY_DELETE_EX(lpTable);
			SendEmptyRecordMsg(stCbSipmsg);  //���Ϳ���Ϣ
			return 0;
		}
		while (iCount<iSumNum)
		{
			//����ظ���Ϣ
			SIP_MSG_INFO msg_info;
			ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
			sprintf(msg_info.CallerID,"%s",stCbSipmsg.strcallee_id.c_str());  //�ظ���Ϣ����
			sprintf(msg_info.CalleedID,"%s",stCbSipmsg.strcaller_id.c_str());
			
			// 			MsgOp oMsgOp;
			// 			oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
			
			//����һ��XML���ĵ�����
			TiXmlDocument *myDocument = new TiXmlDocument();
			if(!myDocument)
			{
				MEMORY_DELETE_EX(msg_info.pMsg);
				return -1;
			}
			TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
			myDocument->LinkEndChild(decl);
			
			//����һ����Ԫ��(��Ϣ����)�����ӡ�
			TiXmlElement *RootElement = new TiXmlElement("Response");
			myDocument->LinkEndChild(RootElement);
			
			//����һ��CmdTypeԪ�ز�����
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
			
			//��ѯ���ݿ��(UnGBPhyDeviceChannelConfig)�õ�ͨ����Ϣ  
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
			
			int iItemNum=0;   //����xml�Ѵ�����Ŀ
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
			
			//xml�ĵ��������Ϊ�ַ���
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
		mgwlog("�˳� CCenterCtrl::DoCatalogCmd\n");
		MEMORY_DELETE_EX(lpTable);
		return 0;
	}
	return 0;
}

//�ռ�¼��Ϣ
int CCenterCtrl::SendEmptyRecordMsg(CBSIPMSG &stCbSipmsg)
{
	//����XMLָ��
	MsgOp oMsgOp;
	oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
	string  strDeviceID;
	strDeviceID = oMsgOp.GetMsgText("DeviceID");
	//����ظ���Ϣ
	SIP_MSG_INFO msg_info;
	ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
	sprintf(msg_info.CallerID,"%s",stCbSipmsg.strcallee_id.c_str());  //�ظ���Ϣ����
	sprintf(msg_info.CalleedID,"%s",stCbSipmsg.strcaller_id.c_str());
	//����һ��XML���ĵ�����
	TiXmlDocument *myDocument = new TiXmlDocument();
	if(!myDocument)
	{
		MEMORY_DELETE_EX(msg_info.pMsg);
		return -1;
	}
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
	myDocument->LinkEndChild(decl);
	
	//����һ����Ԫ��(��Ϣ����)�����ӡ�
	TiXmlElement *RootElement = new TiXmlElement("Response");
	myDocument->LinkEndChild(RootElement);
	
	//����һ��CmdTypeԪ�ز�����
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
	
	//xml�ĵ��������Ϊ�ַ���
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
   //����invite��ȡ DeviceID tsu:ip port
	/* 3����ȡ��Դ��sdp��Ϣ���������е�s�ֶΣ��ж�ҵ������ */
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
    /* ����sdp�е�S name ��Ϣ*/
    INVITE_TYPE eInviteType = INVITE_PLAY;   //ʵʱ��Ƶ
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
		/* ����sdp�е�ʱ����Ϣ*/
		i = get_sdp_t_time(pRemoteSDP, &start_time, &end_time, &play_time);
		if (i != 0)
		{
			sdp_message_free(pRemoteSDP);
            pRemoteSDP = NULL;
			SIP_AnswerToInvite(stCbSipmsg.idialog_index, 503, (char*)"get_sdp_t_time Error");
			mgwlog("get_sdp_t_time Error\n");
			return -1;
		}
		mgwlog("����invite SDP,����:PlayBack\n");
    }
    else if (0 == strncmp(s_name, "Play", 4))
    {
        eInviteType = INVITE_PLAY;
		mgwlog("����invite SDP,����:Play\n");
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

	//���ɻظ���Ϣ
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
	if(!CSysCfg::Instance()->GetOneChCfg(stCbSipmsg.strcallee_id,stChCfg)) //δ�ҵ�
	{
		mgwlog("strcallee_id:[%s] Channel not Found\n",stCbSipmsg.strcallee_id.c_str());
		SIP_AnswerToInvite(stCbSipmsg.idialog_index, 404, (char*)"Channel not Found");
		return -1;  //û���ҵ�ָ���豸
	}

#ifdef WIN32
// 	if(stChCfg.iNeedCodec) 
// 	{
// 		iCodeType = EV9000_STREAMDATA_TYPE_VIDEO_H264;  //��������������Ϊh264
// 	}else{
// 		//iCodeType = stChCfg.iStreamType;          //������룬����ͨ���������������
//         iCodeType = 0;                              //ps��
// 	}
//	iCodeType = 0;                                 //ps��
	iCodeType = EV9000_STREAMDATA_TYPE_PS;         //�������������Ϊps��
    mgwlog("ͨ��strcallee_id:%s ������iCodeType:%d\n",stCbSipmsg.strcallee_id.c_str(),iCodeType);
    i = SIP_GeneratingSDPAnswer(stCbSipmsg.idialog_index, &pLocalSDP, NULL, pcLocalSDPPort, 
		localip, (char*)"Play", 0, 0,0, 1, -1, iCodeType);
#else
    bool bFindInfo = false;
    string strUsr,strPwd,strIP,strMapChannel;
	int iChannelPort = 0;  //rtsp ����˿�
    if(m_pDevCtrl)
	{
		CMediaItemDevice *lpDevice = NULL;
		if( 0 == m_pDevCtrl->FindDevice(stCbSipmsg.strcallee_id,lpDevice))  //�ҵ�
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
	
	//����ָ�����豸ͨ��
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
		if(INVITE_PLAY == eInviteType)  //ʵʱ��Ƶ
		{
			mgwlog("��ʼ��unit:%s\n",stCbSipmsg.strcallee_id.c_str());
            nRet = m_pDevCtrl->OpenReal(stStreamConInfo,UNIT_OPEN_COM);
		}else{
#ifdef WIN32
        	mgwlog("��ʼ��¼��unit:%s starttime:%d endtime:%d playtime:%d\n",
				   stCbSipmsg.strcallee_id.c_str(),start_time,end_time,play_time);
			nRet = m_pDevCtrl->PlayBackOpen(stStreamConInfo,UNIT_OPEN_COM);
			if(!nRet)
			{
				mgwlog("m_pDevCtrl->PlayBackOpen success\n");
			}
#endif
		}
		if (nRet !=0 )  //��ʧ��
		{
			SIP_AnswerToInvite(stCbSipmsg.idialog_index, 503, (char*)"Open Channel Fail");
			mgwlog("Open Channel Fail \n");
			return -1;
		}
	}

	/* 9����Ӧ200 ok��Ϣ�����в�*/
	//modified by chenyu 131214
	/*
	*���ڱ����������м��� /EHa ����,
	*����VC�������Ͳ����û��throw��try_catchģ���Ż���
	*/
	static int iExceptionCount=0;  //�쳣����
	try
	{
		i = SIP_AcceptInvite(stCbSipmsg.idialog_index, pLocalSDP);
		CCtrlProOp::Instance()->RecordDialogIndex(stCbSipmsg.idialog_index,stCbSipmsg.strcallee_id);
		mgwlog("CCtrlProOp::Instance()->RecordDialogIndex,��������¼ idialog_index:%d\n",stCbSipmsg.idialog_index);
	}
	catch (...)
	{
		i=-1;  // �쳣
		iExceptionCount++;  //����
		if(iExceptionCount > MAX_EXCEPTION_NUM)
		{
			mgwlog("Exception: CCenterCtrl::DoInviteCmd �쳣����100�Σ��˳�....\n");
			exit(-1);
		}
		mgwlog("Exception: CCenterCtrl::DoInviteCmd SIP_AcceptInvite,�쳣����:%d\n",iExceptionCount);
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
	//����ָ�����豸ͨ��
	if(!m_pDevCtrl)
	{
		//SIP_AnswerToInvite(stCbSipmsg.idialog_index, 503, (char*)"Open Channel Fail");
		mgwlog("DoInform Fail\n");
		return -1;
	}
	
    //����RTSP�е���Ϣ
	char* msg_body = (char*)stCbSipmsg.strMsg.c_str();
	if (0 == strncmp(msg_body, "PLAY", 4))
	{
		nRet = m_pDevCtrl->PlayBackCtrl(stCbSipmsg,RECORD_CTRL_RESUME,0,NULL);
	}
	else if (0 == strncmp(msg_body, "PAUSE", 5))
	{
		//mgwlog("----�յ������� pause ����-------\n");
		nRet = m_pDevCtrl->PlayBackCtrl(stCbSipmsg,RECORD_CTRL_PAUSE,0,NULL);
	}
	else if (0 == strncmp(msg_body, "TEARDOWN", 8))
	{
		mgwlog("CCenterCtrl::DoInform PlayBackStop\n");
		nRet = m_pDevCtrl->PlayBackStop(stCbSipmsg);
	}
	else if (0 == strncmp(msg_body, "SEEK", 4))
	{
        //��find������
		string strRange = "";
		int bpos = stCbSipmsg.strMsg.find("Range:npt=");
		if(bpos==string::npos)
		{
			mgwlog("find Range��ʼ Fail\n");
			return -1;
		}
		int epos = stCbSipmsg.strMsg.find("\r\n",bpos+10);
		if(epos==string::npos)
		{
			mgwlog("find Range���� Fail\n");
			return -1;
		}
        if(epos>=bpos)
		{
			strRange = stCbSipmsg.strMsg.substr(bpos+10, epos-(bpos+10));
		}
        mgwlog("SEEK time strRange:%s\n",strRange.c_str());
		unsigned int time = atoi(strRange.c_str());
        nRet = m_pDevCtrl->PlayBackCtrl(stCbSipmsg,RECORD_CTRL_SETPLAYPOS,time,NULL);
		/* ����MANSRTSP ��Ϣ */
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
	//�ر�ָ�����豸ͨ��
	if(m_pDevCtrl)
	{
		mgwlog("��ʼ�ر�unit:%s \n",stCbSipmsg.strcallee_id.c_str());
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
		//����ת��
		//����XMLָ��
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

//����DeviceInfo����
int CCenterCtrl::DoDeviceInfoCmd(CBSIPMSG &stCbSipmsg)
{
	//mgwlog("enter CCenterCtrl::DoDeviceInfoCmd\n");
	//����ظ���Ϣ
	SIP_MSG_INFO msg_info;
	ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
	sprintf(msg_info.CallerID,"%s",stCbSipmsg.strcallee_id.c_str());  //�ظ���Ϣ����
	sprintf(msg_info.CalleedID,"%s",stCbSipmsg.strcaller_id.c_str());
	
	MsgOp oMsgOp;
	oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
	
	//����һ��XML���ĵ�����
	TiXmlDocument *myDocument = new TiXmlDocument();
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
	myDocument->LinkEndChild(decl);
	
	//����һ����Ԫ��(��Ϣ����)�����ӡ�
	TiXmlElement *RootElement = new TiXmlElement("Response");
	myDocument->LinkEndChild(RootElement);
	
	//����һ��CmdTypeԪ�ز�����
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
	
	//xml�ĵ��������Ϊ�ַ���
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
		//����XMLָ��
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
        mgwlog("SetDeviceVideoParam ɫ�� %s ���� %s �Աȶ� %s ���� %s\n",
			strColourDegree.c_str(),strBrightness.c_str(),strContrast.c_str(),strSaturation.c_str());
		//����ظ���Ϣ
		SIP_MSG_INFO msg_info;
		ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
		sprintf(msg_info.CallerID,"%s",stCbSipmsg.strcallee_id.c_str());  //�ظ���Ϣ����
		sprintf(msg_info.CalleedID,"%s",stCbSipmsg.strcaller_id.c_str());
		
		//����һ��XML���ĵ�����
		TiXmlDocument *myDocument = new TiXmlDocument();
		TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
		myDocument->LinkEndChild(decl);
		
		//����һ����Ԫ��(��Ϣ����)�����ӡ�
		TiXmlElement *RootElement = new TiXmlElement("Response");
		myDocument->LinkEndChild(RootElement);
		
		//����һ��CmdTypeԪ�ز�����
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
		
		//xml�ĵ��������Ϊ�ַ���
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
		//����XMLָ��
		MsgOp oMsgOp;
		oMsgOp.InputMsg((char *)stCbSipmsg.strMsg.c_str());
		string strDeviceID ="";
		strDeviceID = oMsgOp.GetMsgText("DeviceID");
		string  strColourDegree,strBrightness,strContrast,strSaturation;
		unsigned int nParam=0;
		m_pDevCtrl->GetVideoParam(strDeviceID,nParam);
		//mgwlog("CCenterCtrl::DoGetDeviceVideoParamCmd ��ɫ����nParam:0x%x\n",nParam);
        //ת��Ϊ0-255����
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
		mgwlog("GetDeviceVideoParam ɫ�� %s ���� %s �Աȶ� %s ���� %s\n",
			strColourDegree.c_str(),strBrightness.c_str(),strContrast.c_str(),strSaturation.c_str());
		//����ظ���Ϣ
		SIP_MSG_INFO msg_info;
		ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
		sprintf(msg_info.CallerID,"%s",stCbSipmsg.strcallee_id.c_str());  //�ظ���Ϣ����
		sprintf(msg_info.CalleedID,"%s",stCbSipmsg.strcaller_id.c_str());
		
		//����һ��XML���ĵ�����
		TiXmlDocument *myDocument = new TiXmlDocument();
		TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
		myDocument->LinkEndChild(decl);
		
		//����һ����Ԫ��(��Ϣ����)�����ӡ�
		TiXmlElement *RootElement = new TiXmlElement("Response");
		myDocument->LinkEndChild(RootElement);
		
		//����һ��CmdTypeԪ�ز�����
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
		
		//xml�ĵ��������Ϊ�ַ���
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
		//����ת��
		//����XMLָ��
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
