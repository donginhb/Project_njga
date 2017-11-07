// IceCom.cpp: implementation of the CIceCom class.
//
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include "stdafx.h"
#endif
#include "IceCom.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIceCom::CIceCom()
{
	m_strServId = "TestAdapter";
	m_iServPort = 10000;
    m_iTimeOut = 3;  //默认3s
	m_bThreadIceFlag = FALSE;
#ifdef WIN32
	m_hThreadIce = 0;
	m_dwThreadIceID = 0;
#else
	m_threadHandle=0;
#endif

	icClient =NULL;
	prxClient =NULL;

}

CIceCom::~CIceCom()
{
	Close();
}

BOOL CIceCom::Start(string strServId,int iServPort)
{
	m_strServId = strServId;
	m_iServPort = iServPort;

#ifdef WIN32
	if (!m_bThreadIceFlag)
	{
		m_bThreadIceFlag = TRUE;
		m_hThreadIce = CreateThread(NULL, 0, ThreadIce, (LPVOID)this, 0, &m_dwThreadIceID);
	}
#else
	int err=0;
	if (!m_bThreadIceFlag)
	{
		m_bThreadIceFlag = TRUE;
		err = pthread_create(&m_threadHandle,NULL,ThreadIce,(void*)this);
		if(err!=0)
		{
			return FALSE;
		}
	}
#endif

	return TRUE;
}

BOOL CIceCom::Close()
{
	//added by chenyu 130805
	if (icClient)  //释放资源   
	{   
		try  
		{   
			icClient->destroy ();   
		}   
		catch (const Ice::Exception & e)   
		{   
			//cout<<e.what()<<endl;  
		}   
	} 

#ifdef WIN32
	if(m_hThreadIce)
    {
		m_bThreadIceFlag = FALSE;
		
		WaitForSingleObject(m_hThreadIce, INFINITE);
		
        CLOSE_HANDLE(m_hThreadIce);
		m_dwThreadIceID = 0;
	}
#else
	if(m_threadHandle)
    {
		m_bThreadIceFlag = FALSE;
		pthread_cancel(m_threadHandle);
		pthread_join(m_threadHandle, NULL);
	}
#endif

	return TRUE;
}

int	CIceCom::OnIceServ()
{
	int status = 0;   
	Ice::CommunicatorPtr ic = NULL; 
	string strPos="";
	try  
	{   
		int argc=0;   
        char* a = (char*)"";   
        char** argv=&a;   
		char szPara[200]={0};
        sprintf(szPara,"default -h 0.0.0.0 -p %d",m_iServPort);  //"default -p 10000"
		ic = Ice::initialize (argc, argv);  
		cout<<"Ice服务启动参数:"<<m_strServId<<" "<<szPara<<endl;
		Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints ("TheClientAdapter",szPara);
        _workQueue = new WorkQueue();
		EV9000MS::EV9000MSIPtr object= new ServerI(_workQueue);
		adapter->add (object, ic->stringToIdentity (m_strServId)); //m_strServId在客户端请求时用到
		_workQueue->start();
		adapter->activate (); 
		//ic->waitForShutdown ();
	    _workQueue->getThreadControl().join();
	} 
	catch (const Ice::Exception & e)   	
	{   
		cout<<"error:"<<e.what()<<"  pos:"<<strPos<<endl;   
		status = 1;   
	} 
	catch (const char *msg)   
	{   
		cout <<"msg:"<<msg<<endl;   
		status = 1;   
	}   
	while ((0 == status) && m_bThreadIceFlag)
	{
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	}
    cout<<"Ice服务退出"<<endl;
	if (ic)  //释放资源   
	{   
		try  
		{   
			ic->destroy ();   
		}   
		catch (const Ice::Exception & e)   
		{   
			cerr << e << endl;   
			status = 1;   
		}   
	} 

	return 0;
}

BOOL CIceCom::SetConPara(string strRmtIP,int iRmtPort,string strProxy)
{
	//远端服务参数
	m_strProxy = strProxy;
	m_strRmtIP = strRmtIP;
	if(m_strRmtIP =="")
	{
		m_strRmtIP = "127.0.0.1";
	}
	m_iRmtPort = iRmtPort;

	//init Client
	BOOL bRet = TRUE;  //默认成功
	try {  
		char szConPara[1024]={0};
		sprintf(szConPara,"%s:tcp -h %s -p %d",m_strProxy.c_str(),m_strRmtIP.c_str(),m_iRmtPort);
		int argc=0;   
		char* a= (char*)"";   
		char** argv=&a;   
		
		//Ice::CommunicatorPtr ic;     
		icClient = Ice::initialize(argc, argv); 
		cout<<"[EV9000Ice]do stringtoproxy"<<endl;
		Ice::ObjectPrx base = icClient->stringToProxy(szConPara);
		cout<<"[EV9000Ice]do checkedcast"<<endl;
		prxClient = EV9000MSIPrx::uncheckedCast(base->ice_timeout(m_iTimeOut*1000)); 
		cout<<"do send"<<endl;
		//prx->ice_timeout(1000*1000);
		if(!prxClient)throw "Invalid Proxy!";
		
	}catch (const Ice::Exception& ex) { 
		//cout<<ex.what()<<endl;
		cout<<"CIceCom::SendData Ice exception"<<endl;
		bRet = FALSE;
		//return FALSE;
		//added by chenyu 130805
		if (icClient)  //释放资源   
		{   
			try  
			{   
				icClient->destroy ();   
			}   
			catch (const Ice::Exception & e)   
			{   
				//cout<<e.what()<<endl;  
			}   
		} 
		return bRet;
	}  
    return bRet;
}

BOOL CIceCom::SendData(int nSendType, EDVHEAD_EX &tInData,EDVHEAD_EX & tOutData)
{
	BOOL bRet = TRUE;  //默认成功
	try {  		
		EV9000MS::COMMNMSGHEAD tInComMsg,tOutComMsg;
		EV9000MS::BYTEBUFFER   sInData,sOutData;
		ConverData2Ice(tInData,tInComMsg,sInData);
		if(nSendType ==0)  //同步
		{
			cout<<"[EV9000Ice]send sync data"<<endl;
			prxClient->OMMPToCMSInfo(nSendType, tInComMsg,sInData, tOutComMsg,sOutData);
			ConverIce2Data(tOutComMsg,sOutData,tOutData);
			cout<<"同步收到数据如下:"<<endl<<tOutData.ucPayLoad<<endl;
		}else{  //异步
			cout<<"[EV9000Ice]send async data"<<endl;
            prxClient->OMMPToCMSInfo_async(new AMI_ClientI,nSendType, tInComMsg,sInData);
		}		
	}catch (const Ice::Exception& ex) { 
		//cout<<ex.what()<<endl;
		cout<<"CIceCom::SendData Ice exception"<<endl;
	    bRet = FALSE;
		//return FALSE;
	}  
	return bRet;
}

BOOL CIceCom::ConverData2Ice(const EDVHEAD_EX &tInData,COMMNMSGHEAD &tComMsg,BYTEBUFFER& sInData)
{
	tComMsg.wVersion = tInData.wVersions;
    tComMsg.wPayLoadLen = tInData.wPayLoadLen;
    tComMsg.dwEvent= tInData.dwEvent;
    tComMsg.wResved1= tInData.wResved1;
    tComMsg.wResved2= tInData.wResved2;
    tComMsg.dwCommonID= tInData.dwComonID;
	tComMsg.dwDestIP = tInData.dwDestIP;
	for (int i=0;i<tInData.wPayLoadLen;i++)
	{
		sInData.push_back(tInData.ucPayLoad[i]);
	}
	return TRUE;	
}

BOOL CIceCom::ConverIce2Data(COMMNMSGHEAD &tOutComMsg,BYTEBUFFER& sOutData,EDVHEAD_EX &tOutData)
{
	
	tOutData.wVersions=tOutComMsg.wVersion; 
	tOutData.wPayLoadLen=tOutComMsg.wPayLoadLen;
	tOutData.dwEvent=tOutComMsg.dwEvent;
	tOutData.wResved1=tOutComMsg.wResved1;
	tOutData.wResved1=tOutComMsg.wResved2;
    tOutData.dwComonID= tOutComMsg.dwCommonID;
	tOutData.dwDestIP = tOutComMsg.dwDestIP;

	int iLen = sOutData.size();
	if(iLen>0)
	{
		memcpy(tOutData.ucPayLoad,static_cast<void*>(&sOutData[0]),iLen);
	}	
	return TRUE;	
}
