/***********************************************************************
* Copyright (C) 2013, WISCOM VISION TECHNOLOGY Corporation.
* 
* File Name: 	   EV9000_ManageService.ice
* Description:   ���ļ�����ƽ̨�����ICE�ӿ�
* Others:        ��
* Version:  	   dev-v1.01.01
* Author:        �����������ּ���λ
* Date:  	       2013.05.31
* 
* History 1:  		// �޸���ʷ��¼�������޸����ڡ��޸��ߡ��޸İ汾���޸�����
*                2013.05.31    qyg    dev-v1.01.01   �״δ���  
* History 2: ��
**********************************************************************/



#ifndef EV9000_MANAGE_SERVICE_ICE
#define EV9000_MANAGE_SERVICE_ICE

module EV9000MS
{
    sequence<byte>    BYTEBUFFER;
     
    struct  COMMNMSGHEAD                   //ͬSocketͨ��ģʽ����ͬ
    {
        short   wVersion;
        short   wPayLoadLen;
        int     dwEvent;
        short   wResved1;
        short   wResved2;
        int     dwCommonID; 
        int     dwDestIP;   
    };
    
    struct TSUAlarmMsg
    {
        int iTSUIndex;                  /* TSU���� */
        int iType;                      /* ������� */
	int iLevel;                     /* �������� */
	int iTime;                      /* ����ʱ�� */
        string strInfo;                 /* ��ϸ���� */
    };
	
    struct TSUTaskAttribute
    {
        string sTsuID;
        int type;
        string id;
    };	
    
    sequence<TSUTaskAttribute> TSUTaskAttributeList;

    struct AudioTaskAttribute
    {
        string sReceiveIP; 
        int iReceivePort;
    };	
    
    sequence<AudioTaskAttribute> AudioTaskAttributeList;
	
    struct RTSPTaskAttribute
    {
        string sTaskID; 
    };	
    
    sequence<RTSPTaskAttribute> RTSPTaskAttributeList;	
    
    interface EV9000MSI
    {
        
        /* ֪ͨ�� ��OMMP-->CMS/TSU */
       ["ami","amd"] int  OMMPToCMSInfo(int nSendType, COMMNMSGHEAD tInCommMsg,BYTEBUFFER sInData,out COMMNMSGHEAD tOutCommMsg,out BYTEBUFFER sOutData);    
       
        /* �����࣬OMMP--> CMS/TSU,����ͨ�����д��� */
        int  OMMPToCMSCfg( COMMNMSGHEAD tCommMsg, BYTEBUFFER sConfigPara);
        
        /* ��ѯ�࣬��ѯ���ͨ�����з��� */
        int  OMMPToCMSQry(COMMNMSGHEAD tCommMsg,out BYTEBUFFER sInquriyResult);
        
        /* TSUע�ᣬiRefresh=1��ʾˢ��ע�� */
        int  TSURegister(string sTsuID, int iSlotID, string sTsuVideoIP, int iVideoIPEth, string sTsuDeviceIP, int iDeviceIPEth, int iExpires, int iRefresh, int iTsuType, out int iTsuIndex);
        
        /* TSU��ȡʱ�� */
      	int  TSUGetTime();
      	
        /* TSU֪ͨ������ɣ�¼��طź��ļ�����ʱ��ʹ�� */
      	int  TSUNotifyPlayEnd(TSUTaskAttribute tTSUTaskAttribute);

        /* TSU֪ͨ��ͣ���ţ�¼��طź��ļ�����ʱ��ʹ�� */
      	int  TSUNotifyPausePlay(TSUTaskAttribute tTSUTaskAttribute);

        /* TSU֪ͨ�ָ����ţ�¼��طź��ļ�����ʱ��ʹ�� */
      	int  TSUNotifyResumePlay(TSUTaskAttribute tTSUTaskAttribute);
      	
        /* TSU��ǰ�����ϱ� */
      	int  TSUNotifyCurrentTask(TSUTaskAttributeList tTSUTaskAttribute);
      	
        /* TSU֪ͨCMSǰ��û������ */
      	int  TSUNotifyDeviceNoStream(string sDeviceID);

        /* TSU֪ͨTCP�������ӶϿ� */
      	int TSUNotifyTcpTansferEnd(string strTranferSessionID,int iType);
      	
        /* TSU�ϱ��¶� */
        void TSUNotifyCPUTemperature(int iSlotID, int iTemperature);
		
        /* TSU֪ͨ������*/
      	int  TSUNotifyCreateTaskResult(string sTaskID, int iResult);

        /* TSU֪ͨ������Ϣ */
      	int  TSUNotifyAlarmInfo(TSUAlarmMsg tTSUAlarmMsg);		

        /* TSU���͸�CMS��ȡ��ͼ����
		   ����: int iType(��ʱ�ģ������ġ��ֶ���), int iResult���ɹ���ʧ�ܣ�, ��λID, ��λChannel ID, GUID, �ڼ���, ͼƬ����(Base64) 
        */
      	int TSUSendImageResult(int iType, int iResult, string strDeviceID, int iChannelID, string strGuid, int iPicCount, BYTEBUFFER strPicContent);

      	/* ��ƵTSUע�ᣬiRefresh=1��ʾˢ��ע�� */
        int AudioTSURegister(string sTsuID, int iExpires, int iRefresh, AudioTaskAttributeList tAudioTaskAttribute);
	
      	/* ������ͨ��ʹ�� */  	
      	int GetCmsUsedStatus(); 
      	int GetSlaveCmsStatus();
      	
      	/* wangqichao add ������ͨ�� */
      	["ami","amd"] void GetMyHeart(int nSendType, int Type, int Level, string CMSID, string StrInfo, out int ret, out string rsp);
      	["ami","amd"] void GetMyHAInfo(int nSendType, int Type, int Level, string CMSID, string StrInfo, out int ret, out string rsp);
      	
    };

};

#endif
