// ***BEGIN***  ICE修改 wangqichao 2013/8/29 add
//wangqichao add
/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/

#include "ClientI.h"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
extern void do_restart();
#ifndef WIN32
int m_MyHeart_send_recv = 0;
int m_MyHAInfo_send_recv = 0;
int m_HA_send_recv = 0;

bool PrintfSQLToFile_client(const char* c_sql)
{
return true;
char sql[1000]={0};
    strcpy(sql,c_sql);
    //strcat(sql,getFormateCurrentTime("%H:%M:%S"));
    strcat(sql,"\n");
    
    FILE *fd;

    if ((fd = fopen("/app/HAFile_Client", "r+b")) == NULL)
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

AMI_ICE_HA_Client::AMI_ICE_HA_Client(string StrServerIp,int IntServerPort,string OutStrServerIp,int OutIntServerPort)
{
    //删除双机备份配置文件
    system("rm -rf /config/DBHAFILE");
    m_MyHeart_send_recv = 0;
    m_MyHAInfo_send_recv = 0;
    m_HAtimer = time(NULL);


    m_our_status = -1;

    m_innertimer = time(NULL);
    m_outertimer = time(NULL);    

    m_looptimer = time(NULL);
    m_StrServerIp = StrServerIp;
    m_IntServerPort = IntServerPort;
    m_StrProxy = "CmsIceService";

    //外部浮动ICE端口
    m_OutStrServerIp = OutStrServerIp;
    m_OutIntServerPort = OutIntServerPort;
    m_OutStrProxy = "CmsIceService";
    
    m_iTimer = 5;
    ICE_Enable = false;

#ifdef WQCLOG
    printf("\r\n WQCLOG AMI_ICE_HA_Client::AMI_ICE_HA_Client \r\n");
    printf("\r\n WQCLOG m_StrServerIp %s \r\n",StrServerIp.c_str());
    printf("\r\n WQCLOG IntServerPort %d \r\n",IntServerPort);
    printf("\r\n WQCLOG OutStrServerIp %s \r\n",OutStrServerIp.c_str());
    printf("\r\n WQCLOG OutIntServerPort %d \r\n",OutIntServerPort);

#endif
    
    ThreadLog_Exit = false;
    pthread_create(&m_threadHandle,NULL,Thread_IceClient,(void*)this);
    
}

AMI_ICE_HA_Client::~AMI_ICE_HA_Client()
{
#ifdef WQCLOG
printf("\r\n WQCLOG AMI_ICE_HA_Client::~AMI_ICE_HA_Client() \r\n");
#endif
    ThreadLog_Exit = true;
}

void AMI_ICE_HA_Client::setourstatus(int tmpstatus)
{
#ifdef WQCLOG
    printf("\r\n WQCLOG AMI_ICE_HA_Client::setourstatus %d \r\n",tmpstatus);
#endif
    m_MyHeart_send_recv = 0;
    m_MyHAInfo_send_recv = 0;
    m_HAtimer = time(NULL);

    m_our_status = tmpstatus;

    char initstr[50]={0};
    unsigned int nLen = 0;
    FILE *fd;

    if(m_our_status == -1)
    {
        #ifdef WQCLOG
        printf("\r\n WQCLOG m_our_status == -1 \r\n");
        #endif
        return;
    }

    if(m_our_status == 1)
    {
        #ifdef WQCLOG
        printf("\r\n WQCLOG m_our_status == 1 \r\n");
        #endif        
        memset(initstr,0,50);
        sprintf(initstr,"rm -rf /config/DBHAFILE");
        system(initstr);
        do_restart();
        return ;
    }

        #ifdef WQCLOG
        printf("\r\n WQCLOG AMI_ICE_HA_Client::setourstatus=0 \r\n");
        #endif 
    if ((fd = fopen("/config/DBHAFILE", "w+")) == NULL)
    {
        #ifdef WQCLOG
        printf("\r\n WQCLOG /config/DBHAFILE is not \r\n");
        #endif 
        return ;
    }

    fseek(fd,0,SEEK_END);
    memset(initstr,0,50);
    sprintf(initstr,"DBHAFILE\n");
    nLen = strlen(initstr);
    fwrite(initstr, sizeof(char), nLen, fd);
    fclose(fd);
}

int AMI_ICE_HA_Client::OutInitIce()
{
        #ifdef WQCLOG
        printf("\r\n WQCLOG AMI_ICE_HA_Client::OutInitIce \r\n");
        #endif 
    try
    {
        char szConPara[1024]={0};
        sprintf(szConPara,"%s:tcp -h %s -p %d",m_OutStrProxy.c_str(),m_OutStrServerIp.c_str(),m_OutIntServerPort);
        int argc=0;   
        char* a = (char*)"";   
        char** argv=&a;
        Ice::CommunicatorPtr ic;
        ic = Ice::initialize(argc, argv); 
        Ice::ObjectPrx base = ic->stringToProxy(szConPara);
        //连接ICE并设置超时时间
        out_prx = EV9000MSIPrx::checkedCast(base->ice_timeout(m_iTimer*1000));
        if(!out_prx)
        {
            #ifdef WQCLOG
            printf("\r\n WQCLOG out_prx NULL ICE_Enable false \r\n");
            #endif
            throw "Invalid Proxy!";
        }
        else
        {
            #ifdef WQCLOG
            printf("\r\n WQCLOG out_ICE_Enable true \r\n");
            #endif

        }
        return 0;
            
   }
   catch(...)
   {
       ICE_Enable = false;
       #ifdef WQCLOG
       printf("\r\n WQCLOG AMI_ICE_HA_Client not runing! plase check it! \r\n");
       #endif
       return -1;
   }
   return 0;
}

int AMI_ICE_HA_Client::IntIce()
{
       #ifdef WQCLOG
       printf("\r\n WQCLOG AMI_ICE_HA_Client::IntIce \r\n");
       #endif
    
    try
    {
        m_MyHeart_send_recv = 0;
        m_MyHAInfo_send_recv = 0;
        char szConPara[1024]={0};
        sprintf(szConPara,"%s:tcp -h %s -p %d",m_StrProxy.c_str(),m_StrServerIp.c_str(),m_IntServerPort);
        int argc=0;   
        char* a = (char*)"";   
        char** argv=&a;
        Ice::CommunicatorPtr ic;
        ic = Ice::initialize(argc, argv); 
        Ice::ObjectPrx base = ic->stringToProxy(szConPara);
        //连接ICE并设置超时时间
        prx = EV9000MSIPrx::checkedCast(base->ice_timeout(m_iTimer*1000));
        if(!prx)
        {
            #ifdef WQCLOG
            printf("\r\n WQCLOG prx NULL ICE_Enable false \r\n");
            #endif
            ICE_Enable = false;
            throw "Invalid Proxy!";
        }
        else
        {
            #ifdef WQCLOG
            printf("\r\n WQCLOG ICE_Enable true \r\n");
            #endif
            ICE_Enable = true;
        }
        return 0;
            
   }
   catch(...)
   {
       ICE_Enable = false;
            #ifdef WQCLOG
            printf("\r\n WQCLOG AMI_ICE_HA_Client not runing! plase check it! \r\n");
            #endif
       return -1;
   }
   return 0;
}

void AMI_ICE_HA_Client::SendInfo(int Type,int Level,string CMSID,string info)
{
            #ifdef WQCLOG
            printf("\r\n WQCLOG AMI_ICE_HA_Client::SendInfo \r\n");
            #endif
    try
    {
        prx->GetMyHeart_async(new AMI_EV9000MSI_ICE_GetMyHeart,1,Type,Level,CMSID,info);
    }
    catch(...)
    {

    }
}

void AMI_ICE_HA_Client::Out_SendInfo(int Type,int Level,string CMSID,string info)
{
            #ifdef WQCLOG
            printf("\r\n WQCLOG AMI_ICE_HA_Client::Out_SendInfo \r\n");
            #endif
    try
    {
        prx->GetMyHeart_async(new AMI_EV9000MSI_ICE_GetMyHeart,1,Type,Level,CMSID,info);
    }
    catch(...)
    {

    }
}

void AMI_ICE_HA_Client::updatetimeHA()
{
            #ifdef WQCLOG
            printf("\r\n WQCLOG AMI_ICE_HA_Client::updatetimeHA \r\n");
            #endif
    m_HAtimer = time(NULL);
}

void* AMI_ICE_HA_Client::Thread_IceClient(void *p)
{
    #ifdef WQCLOG
    printf("\r\n WQCLOG AMI_ICE_HA_Client::Thread_IceClient \r\n");
    #endif
    
    AMI_ICE_HA_Client* This = (AMI_ICE_HA_Client*)p;
    while(!This->ThreadLog_Exit)
    {
        This->ProgramRuning();
        sleep(5);
    }
    #ifdef WQCLOG
    printf("\r\n WQCLOG Thread_IceClient while close \r\n");
    #endif
    pthread_join(This->m_threadHandle, NULL);
    return NULL;
}
void AMI_ICE_HA_Client::ProgramRuning()
{
    if(m_our_status != 0)
    {
         
        #ifdef WQCLOG
        printf("\r\n WQCLOG AMI_ICE_HA_Client not send message \r\n");
        #endif
        
        if(time(NULL) - m_HAtimer > 30 && m_our_status == 1)
        {

            #ifdef WQCLOG
            printf("\r\n WQCLOG AMI_ICE_HA_Server not rev rsp \r\n");
            #endif

            S2MexchangeProc();
            setourstatus(0);
        }

        sleep(5);
        return;
    }
    if(!ICE_Enable)
    {
        IntIce();
        return;
    }
    if(time(NULL) - m_looptimer < 10)
    {
        return;
    }

    if(check_inner_ice() != 0)
    {
        //双机切换
        
        #ifdef WQCLOG
        printf("\r\n WQCLOG check_inner_ice end : %d HA \r\n",m_MyHeart_send_recv);
        #endif
        
        setourstatus(1);
    }

    if(check_outer_ice() != 0)
    {
        //双机切换

        #ifdef WQCLOG
        printf("\r\n WQCLOG check_outer_ice end : %d HA \r\n",m_MyHAInfo_send_recv);
        #endif
        
        setourstatus(1);
    }

    
    m_looptimer = time(NULL);
}

void AMI_ICE_HA_Client::Close()
{
    ThreadLog_Exit = true;
}

void AMI_ICE_HA_Client::SendInfotoHA(int Type,int Level,string CMSID,string info)
{
    try
    {

            #ifdef WQCLOG
            printf("\r\n WQCLOG AMI_ICE_HA_Client::SendInfotoHA \r\n");
            #endif
        prx->GetMyHAInfo_async(new AMI_EV9000MSI_ICE_GetMyHAInfo,1,Type,Level,CMSID,info);
        m_MyHAInfo_send_recv = 1;
        m_outertimer = time(NULL);
               
    }
    catch(...)
    {
        
    }
}

int AMI_ICE_HA_Client::check_inner_ice()
{
    #ifdef WQCLOG
    printf("\r\n WQCLOG AMI_ICE_HA_Client::check_inner_ice \r\n");
    #endif
    
    if(m_MyHeart_send_recv == 0)
    {
        
        #ifdef WQCLOG
        printf("\r\n WQCLOG AMI_ICE_HA_Client::SendInfo \r\n");
        #endif
        
        SendInfo(0,0,"cms","keep live");
        m_MyHeart_send_recv = 1;
        m_innertimer = time(NULL);
        return 0;
    }
    else
    if(m_MyHeart_send_recv == 1)
    {
        if(time(NULL) - m_innertimer > 30)
        {
            #ifdef WQCLOG
            printf("\r\n WQCLOG AMI_ICE_HA_Client::Out_SendInfo \r\n");
            #endif
            
            Out_SendInfo(0,0,"cms","keep live");
            m_MyHeart_send_recv = 2;
            m_innertimer = time(NULL);
            return 0;
        }
        else
        {
            #ifdef WQCLOG
            printf("\r\n WQCLOG AMI_ICE_HA_Client::runing \r\n");
            #endif
            
            SendInfo(0,0,"cms","keep live");
            return 0;
        }
    }
    else
    if(m_MyHeart_send_recv == 2)
    {
        if(time(NULL) - m_innertimer > 30)
        {
            

            #ifdef WQCLOG
            printf("\r\n WQCLOG Out_SendInfo AMI_EV9000MSI_ICE_GetMyHeart::time_out \r\n");
            #endif
            
            return 100;
        }
        else
        {
            #ifdef WQCLOG
            printf("\r\n WQCLOG Out_SendInfo AMI_EV9000MSI_ICE_GetMyHeart runing \r\n");
            #endif
            Out_SendInfo(0,0,"cms","keep live");
            return 0;
        }
    }
    else
    {
        return -1;
    }
}
int AMI_ICE_HA_Client::check_outer_ice()
{
            #ifdef WQCLOG
            printf("\r\n WQCLOG AMI_ICE_HA_Client::check_outer_ice \r\n");
            #endif

    if(m_MyHAInfo_send_recv == 0)
    {

        #ifdef WQCLOG
        printf("\r\n WQCLOG AMI_EV9000MSI_ICE_GetMyHAInfo runing \r\n");
        #endif

        
        return 0;
    }
    else
    if(m_MyHAInfo_send_recv == 1)
    {
        if(time(NULL) - m_outertimer > 30)
        {
            #ifdef WQCLOG
            printf("\r\n WQCLOG AMI_EV9000MSI_ICE_GetMyHAInfo time_out \r\n");
            #endif

            return 0;
        }
        else
        {
            #ifdef WQCLOG
            printf("\r\n WQCLOG AMI_EV9000MSI_ICE_GetMyHAInfo HA return 100 send_recv = 1 \r\n");
            #endif

            return 100;
        }
    }
    if(m_MyHAInfo_send_recv == 2)
    {
        #ifdef WQCLOG
        printf("\r\n WQCLOG AMI_EV9000MSI_ICE_GetMyHAInfo HA return 100 send_recv = 2 \r\n");
        #endif

        return 100;
    }
    else
    {
        #ifdef WQCLOG
        printf("\r\n WQCLOG AMI_EV9000MSI_ICE_GetMyHAInfo runing default \r\n");
        #endif

        return 0;
    }
}
#endif
// ***END***  ICE修改 wangqichao 2013/8/29 add

