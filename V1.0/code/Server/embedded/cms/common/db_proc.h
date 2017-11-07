#if !defined(_DB_PROC_H_)
#define _DB_PROC_H_

#include <iostream>
#include <string>
#include <queue>
#include <vector>

#include "common/DBOper.h"
#include "common/gbldef.inc"

using namespace std;

extern int add_default_mms_route_info_to_route_info_list();
extern int BakUpDateBase();
extern int ClosedDB();
extern void DBThreadDelete();
extern void DBThreadNew();
extern int db_connect();
extern int db_gDBOperconnect();
extern int DeleteDefaultUserInfo(int iUserType, DBOper* pdboper);
extern void DeleteUnnecessaryDBData();
extern int get_compress_task_count_from_db(char* platform_ip, DBOper* pDbOper);
extern int get_platform_last_task_time_and_status(char* platform_ip, DBOper* pDbOper, int* iTaskTime, int* iTaskStatus);
extern int get_platform_task_mode_and_time(char* platform_ip, DBOper* pDbOper, int* iTaskMode, int* iTaskBeginTime, int* iTaskEndTime);
extern void InitAllGBDeviceRegStatus();
extern void InitAllUserRegStatus();
extern void InitAllZRVDeviceRegStatus();
extern void InsertDefaultDBData();
extern int InsertDefaultUserInfo(char* userName, DBOper* pdboper);
extern int InsertGBCodesDefautConfig(DBOper* pGBCodes_Info_dboper);
extern int load_device_group_cfg_from_db(DBOper* pDbOper);
extern int load_device_group_map_cfg_from_db(DBOper* pDbOper);
extern int SetBoardConfigTable(char* pcBoardID, int iSlotID, int iBoardType, DBOper* ptDBoper, int* iAssignRecord);
extern int SetBoardNetConfigTable(int iBoardIndex, int iIPEth, char* pcIPAddr, int iIPUsedFlag, DBOper* ptDBoper);
extern int SetCMSBoardInfoToDB();
extern int SetMySQLEventOn();
extern int set_db_data_to_compress_task_list(char* platform_ip, DBOper* pDbOper);
extern int set_db_data_to_cruise_srv_list();
extern int set_db_data_to_Device_info_list();
extern int set_db_data_to_LogicDevice_info_list();
extern int set_db_data_to_plan_srv_list();
extern int set_db_data_to_platform_info_list();
extern int set_db_data_to_poll_srv_list();
extern int set_db_data_to_preset_auto_back_list();
extern int set_db_data_to_record_info_list();
extern int set_db_data_to_record_info_list_by_plan_action_info();
extern int set_db_data_to_record_info_list_by_shdb_alarm_upload_pic();
extern int set_db_data_to_record_info_list_by_shdb_daily_upload_pic();
extern int set_db_data_to_route_info_list();
extern int set_db_data_to_tsu_info_list();
extern int set_db_data_to_ZRVDevice_info_list();
extern int UpdateAllBoardConfigTableStatus(int iStatus, DBOper* ptDBoper);
extern int UpdateAllGBDeviceRegStatus2DB(int status, DBOper* pDevice_Srv_dboper);
extern int UpdateAllGBLogicDeviceRegStatus2DB(int status, DBOper* pDevice_Srv_dboper);
extern int UpdateAllUserRegStatus2DB(int status, DBOper* pUser_Srv_dboper);
extern int UpdateAllZRVDeviceRegStatus2DB(int status, DBOper* pDevice_Srv_dboper);
extern int UpdateBoardConfigTableStatus(char* pcBoardID, int iBoardType, int iStatus, DBOper* ptDBoper);
extern int UpdateMgwOptionsIPAddress2DB(char* pcIPAddr, DBOper* pdboper);
extern int WebNotifyDBRefreshProc(char* pcTabName);
extern int WebNotifyDBSyncProc();

#endif //end _DB_PROC_H_
