/***********************************************************************
* Copyright (C) 2013, WISCOM VISION TECHNOLOGY Corporation.
* 
* File Name: 	   EV9000_ManageService.ice
* Description:   本文件定义平台管理的ICE接口
* Others:        无
* Version:  	   dev-v1.01.01
* Author:        输入作者名字及单位
* Date:  	       2013.05.31
* 
* History 1:  		// 修改历史记录，包括修改日期、修改者、修改版本及修改内容
*                2013.05.31    qyg    dev-v1.01.01   首次创建  
* History 2: …
**********************************************************************/



#ifndef EV9000_MANAGE_SERVICE_ICE
#define EV9000_MANAGE_SERVICE_ICE

module EV9000MS
{
    sequence<byte>    BYTEBUFFER;
     
    struct  COMMNMSGHEAD                   //同Socket通信模式下相同
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
        int iTSUIndex;                  /* TSU索引 */
        int iType;                      /* 报警类别 */
	int iLevel;                     /* 报警级别 */
	int iTime;                      /* 报警时间 */
        string strInfo;                 /* 详细内容 */
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
        
        /* 通知类 ，OMMP-->CMS/TSU */
       ["ami","amd"] int  OMMPToCMSInfo(int nSendType, COMMNMSGHEAD tInCommMsg,BYTEBUFFER sInData,out COMMNMSGHEAD tOutCommMsg,out BYTEBUFFER sOutData);    
       
        /* 配置类，OMMP--> CMS/TSU,参数通过序列传递 */
        int  OMMPToCMSCfg( COMMNMSGHEAD tCommMsg, BYTEBUFFER sConfigPara);
        
        /* 查询类，查询结果通过序列返回 */
        int  OMMPToCMSQry(COMMNMSGHEAD tCommMsg,out BYTEBUFFER sInquriyResult);
        
        /* TSU注册，iRefresh=1表示刷新注册 */
        int  TSURegister(string sTsuID, int iSlotID, string sTsuVideoIP, int iVideoIPEth, string sTsuDeviceIP, int iDeviceIPEth, int iExpires, int iRefresh, int iTsuType, out int iTsuIndex);
        
        /* TSU获取时间 */
      	int  TSUGetTime();
      	
        /* TSU通知播放完成，录像回放和文件下载时候使用 */
      	int  TSUNotifyPlayEnd(TSUTaskAttribute tTSUTaskAttribute);

        /* TSU通知暂停播放，录像回放和文件下载时候使用 */
      	int  TSUNotifyPausePlay(TSUTaskAttribute tTSUTaskAttribute);

        /* TSU通知恢复播放，录像回放和文件下载时候使用 */
      	int  TSUNotifyResumePlay(TSUTaskAttribute tTSUTaskAttribute);
      	
        /* TSU当前任务上报 */
      	int  TSUNotifyCurrentTask(TSUTaskAttributeList tTSUTaskAttribute);
      	
        /* TSU通知CMS前端没有码流 */
      	int  TSUNotifyDeviceNoStream(string sDeviceID);

        /* TSU通知TCP码流链接断开 */
      	int TSUNotifyTcpTansferEnd(string strTranferSessionID,int iType);
      	
        /* TSU上报温度 */
        void TSUNotifyCPUTemperature(int iSlotID, int iTemperature);
		
        /* TSU通知任务结果*/
      	int  TSUNotifyCreateTaskResult(string sTaskID, int iResult);

        /* TSU通知报警信息 */
      	int  TSUNotifyAlarmInfo(TSUAlarmMsg tTSUAlarmMsg);		

        /* TSU发送给CMS截取的图像结果
		   参数: int iType(定时的，报警的、手动的), int iResult（成功，失败）, 点位ID, 点位Channel ID, GUID, 第几张, 图片内容(Base64) 
        */
      	int TSUSendImageResult(int iType, int iResult, string strDeviceID, int iChannelID, string strGuid, int iPicCount, BYTEBUFFER strPicContent);

      	/* 音频TSU注册，iRefresh=1表示刷新注册 */
        int AudioTSURegister(string sTsuID, int iExpires, int iRefresh, AudioTaskAttributeList tAudioTaskAttribute);
	
      	/* 主、备通信使用 */  	
      	int GetCmsUsedStatus(); 
      	int GetSlaveCmsStatus();
      	
      	/* wangqichao add 主、备通信 */
      	["ami","amd"] void GetMyHeart(int nSendType, int Type, int Level, string CMSID, string StrInfo, out int ret, out string rsp);
      	["ami","amd"] void GetMyHAInfo(int nSendType, int Type, int Level, string CMSID, string StrInfo, out int ret, out string rsp);
      	
    };

};

#endif
