// IceComEx.cpp: implementation of the CIceComEx class.
//
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include "stdafx.h"
#endif
#include "IceComEx.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIceComEx::CIceComEx()
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

CIceComEx::~CIceComEx()
{
	Close();
}

BOOL CIceComEx::Start(string strServId,int iServPort)
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

BOOL CIceComEx::Close()
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

int	CIceComEx::OnIceServ()
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
		CSysRunStatePtr object= new ServerIf();
		adapter->add (object, ic->stringToIdentity (m_strServId)); //m_strServId在客户端请求时用到
		//_workQueue->start();
		adapter->activate (); 
		//ic->waitForShutdown ();
	    //_workQueue->getThreadControl().join();
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

BOOL CIceComEx::SetConPara(string strRmtIP,int iRmtPort,string strProxy)
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
		cout<<szConPara<<endl;
		int argc=0;   
		char* a= (char*)"";   
		char** argv=&a;   
		
		//Ice::CommunicatorPtr ic;     
		icClient = Ice::initialize(argc, argv); 
		Ice::ObjectPrx base = icClient->stringToProxy(szConPara);
		//prxClient = CSysRunStatePrx::uncheckedCast(base->ice_timeout(m_iTimeOut*1000));   //windows 待调整 chenyu
		prxClient = CSysRunStatePrx::uncheckedCast(base->ice_timeout(10*1000));   //windows 待调整 chenyu
		if(!prxClient)throw "Invalid Proxy!";
	}catch (const Ice::Exception& ex) { 
		cout<<"CIceCom::SetConPara Ice exception:"<<ex.what()<<endl;
		bRet = FALSE;
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

BOOL CIceComEx::SendData(string strInData,string& strOutData)
{
	BOOL bRet = TRUE;  //默认成功
	try {  		
		cout<<"strInData:"<<strInData<<endl;
		//ConverData2Ice(pInBuf,iInBufLen,strInData);
		if(!prxClient)throw "Invalid Proxy!";
		prxClient->SysRunState(strInData,strOutData);
    	//ConverIce2Data(strOutData,pOutBuf,iOutBufLen,iOutDataLen);		
	}catch (const Ice::Exception& ex) { 
		cout<<"CIceCom::SendData Ice exception:"<<ex.what()<<endl;
	    bRet = FALSE;
	}
	return bRet;
}

BOOL CIceComEx::SendData(char *pInBuf,int iInBufLen,char *pOutBuf,int iOutBufLen,int &iOutDataLen)
{
	string strInData,strOutData;
	strInData = pInBuf;
    int bRet = TRUE; 
	do
	{
		if(!SendData(strInData,strOutData))
		{
			bRet = FALSE;
			cout<<"SendData fail"<<endl;
			break;
		}
		
		int iOutDataLen = strOutData.size();
		if(iOutDataLen>iOutBufLen)
		{
			bRet = FALSE;
			cout<<"输出缓冲长度不够"<<endl;
			break;
		}
		if(iOutDataLen>0)
		{
			memset(pOutBuf,0,iOutBufLen);
			memcpy(pOutBuf,strOutData.c_str(),iOutDataLen);
		}
	} while (0);
	return bRet;
}
