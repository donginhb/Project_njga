#include <sys/types.h>
#include <stdio.h>
#include <vector>
#include <string>
using namespace std;

#include "PublicServiceSoapBinding.nsmap"
#include "soapPublicServiceSoapBindingProxy.h"
#include "jwpt_interface/interface_operate.inc"
#include "libxml/Packet.h"
#include "common/log_proc.inc"

/*****************************************************************************
 函 数 名  : interface_queryObjectInfo
 功能描述  : 查询接口
 输入参数  : int iBeginTime        
             int iEndTime          
             string &strResultXML  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2017年6月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int interface_queryObjectInfo(int iBeginTime, int iEndTime, string &strResultXML)
{
    PublicServiceSoapBindingProxy service;
    char strXMLSet[512] = {0};
    char* pcResultXMLSet = NULL;
    char strBeginTime[32] = {0};
    char strEndTime[32] = {0};
    char str_begin_date[12] = {0};
    char str_begin_time[12] = {0};
    char str_end_date[12] = {0};
    char str_end_time[12] = {0};

    time_t utc_begin_time;
    struct tm local_begin_time = { 0 };
    time_t utc_end_time;
    struct tm local_end_time = { 0 };

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    utc_begin_time = iBeginTime;
    localtime_r(&utc_begin_time, &local_begin_time);
    strftime(str_begin_date, sizeof(str_begin_date), "%Y%m%d", &local_begin_time);
    strftime(str_begin_time, sizeof(str_begin_time), "%H%M%S", &local_begin_time);
    snprintf(strBeginTime, 32, "%s%s", str_begin_date, str_begin_time);


    utc_end_time = iEndTime;
    localtime_r(&utc_end_time, &local_end_time);
    strftime(str_end_date, sizeof(str_end_date), "%Y%m%d", &local_end_time);
    strftime(str_end_time, sizeof(str_end_time), "%H%M%S", &local_end_time);
    snprintf(strEndTime, 32, "%s%s", str_end_date, str_end_time);

    outPacket.SetRootTag("xmlSet");

    AccNode = outPacket.CreateElement((char*)"returnFileds");
    outPacket.SetElementValue(AccNode, (char*)"jlbh,wjmc,kzm,wjdx,scdw,scsj,cclj");

    AccNode = outPacket.CreateElement((char*)"condition");
    snprintf(strXMLSet, 512, "scsj between '%s' and '%s'", strBeginTime, strEndTime);
    outPacket.SetElementValue(AccNode, strXMLSet);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "interface_queryObjectInfo:strXMLSet=%s \r\n", (char*)outPacket.GetXml(NULL).c_str());

	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
	printf("interface_queryObjectInfo() strXMLSet:\r\n%s\r\n", (char*)outPacket.GetXml(NULL).c_str());
	printf("--------------------------------------------\r\n");

    if (service.queryObjectInfo((char *)"SHZR", (char *)"JWPT_YSPB", (char*)outPacket.GetXml(NULL).c_str(), (char *&)pcResultXMLSet) == SOAP_OK)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "interface_queryObjectInfo OK\r\n");
        printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\r\n");
        printf("interface_queryObjectInfo() ResultXMLSet:\r\n%s\r\n", pcResultXMLSet);
        printf("--------------------------------------------\r\n");

        strResultXML = pcResultXMLSet;
        return 0;
    }
    else
    {
        printf("interface_queryObjectInfo Error\r\n");
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "interface_queryObjectInfo Error\r\n");
        return FILE_COMPRESS_WEBSERVICE_QUERY_ERROR;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : interface_updateObjectInfo
 功能描述  : 更新接口
 输入参数  : char* jlbh            
             int iyshdx            
             char* pcyshlj         
             string &strResultXML  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2017年6月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int interface_updateObjectInfo(char* jlbh, int iyshdx, char* pcyshlj, string &strResultXML)
{
    PublicServiceSoapBindingProxy service;
    char strXMLSet[512] = {0};
    char* pcResultXMLSet = NULL;

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    outPacket.SetRootTag("xmlSet");

    AccNode = outPacket.CreateElement((char*)"opterateFileds");
    outPacket.SetElementValue(AccNode, (char*)"yshdx,yshlj");

    AccNode = outPacket.CreateElement((char*)"condition");
    snprintf(strXMLSet, 512, "jlbh='%s'", jlbh);
    outPacket.SetElementValue(AccNode, strXMLSet);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "interface_updateObjectInfo:strXMLSet=%s \r\n", (char*)outPacket.GetXml(NULL).c_str());

	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
	printf("interface_updateObjectInfo() strXMLSet:\r\n%s\r\n", (char*)outPacket.GetXml(NULL).c_str());
	printf("--------------------------------------------\r\n");

    if (service.updateObjectInfo((char *)"SHZR", (char *)"JWPT_YSPB", (char *)strXMLSet, (char *&)pcResultXMLSet) == SOAP_OK)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "interface_updateObjectInfo OK\r\n");

		printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\r\n");
        printf("interface_updateObjectInfo() ResultXMLSet:\r\n%s\r\n", pcResultXMLSet);
        printf("--------------------------------------------\r\n");

        strResultXML = pcResultXMLSet;
        return 0;
    }
    else
    {
        printf("interface_updateObjectInfo Error\r\n");
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "interface_updateObjectInfo Error\r\n");
        return FILE_COMPRESS_WEBSERVICE_UPDATE_ERROR;
    }

    return 0;
}
