#!/bin/sh
#------------------------------------------------------------main--------------------------------------------
cd /data

echo "#####ִ���ֻ�MMS���ݿⴴ���ű�:��ʼ#####"

echo "*******************************************************************"

echo "һ.�����ֻ�MMS���ݿ��:��ʼ---"


echo "1.create_Ev9000DB_MOBILE Begin:---"
sqlfile="/data/create_Ev9000DB_MOBILE.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/create_Ev9000DB_MOBILE.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/create_Ev9000DB_MOBILE.txt
rm -rf  $sqlfile
echo "create_Ev9000DB_MOBILE End:---"


echo "2.create_Ev9000LOG_MOBILE Begin:---"
sqlfile="/data/create_Ev9000LOG_MOBILE.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/create_Ev9000LOG_MOBILE.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/create_Ev9000LOG_MOBILE.txt
rm -rf  $sqlfile
echo "create_Ev9000LOG_MOBILE End:---"


echo "3.create_Ev9000MY_MOBILE Begin:---"
sqlfile="/data/create_Ev9000MY_MOBILE.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/create_Ev9000MY_MOBILE.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/create_Ev9000MY_MOBILE.txt
rm -rf  $sqlfile
echo "create_Ev9000MY_MOBILE End:---"


echo "4.create_Ev9000TSU_MOBILE Begin:---"
sqlfile="/data/create_Ev9000TSU_MOBILE.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/create_Ev9000TSU_MOBILE.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/create_Ev9000TSU_MOBILE.txt
rm -rf  $sqlfile
echo "create_Ev9000TSU_MOBILE End:---"


echo "4.create_app_server_db Begin:---"
sqlfile="/data/create_app_server_db.sql"
rm -rf  $sqlfile
cp /app/update_file_mobile/create_app_server_db.sql /data/
mysql -uroot -pwiscom < /data/create_app_server_db.sql
rm -rf  $sqlfile
echo "create_app_server_db End:---"


echo "�����ֻ�MMS���ݿ��:����---"

echo "*******************************************************************"

echo "��.�޸��ֻ�MMS���ݿ��:��ʼ---"


echo "1.alter_AlarmRecord Begin:---"
sqlfile="/data/alter_AlarmRecord.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_AlarmRecord.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_AlarmRecord.txt
rm -rf  $sqlfile
echo "alter_AlarmRecord End:---"


sqlfile="alter_app_server_db.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_app_server_db.txt /data/
mysql -uroot -pwiscom < /data/alter_app_server_db.txt
rm -rf  $sqlfile


echo "2.alter_BoardConfig Begin:---"	
sqlfile="/data/alter_BoardConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_BoardConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_BoardConfig.txt
rm -rf  $sqlfile
echo "alter_BoardConfig End:---"


echo "3.alter_CruiseActionConfig Begin:---"	
sqlfile="/data/alter_CruiseActionConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_CruiseActionConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_CruiseActionConfig.txt
rm -rf  $sqlfile
echo "alter_CruiseActionConfig End:---"


echo "4.alter_CruiseConfig Begin:---"	
sqlfile="/data/alter_CruiseConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_CruiseConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_CruiseConfig.txt
rm -rf  $sqlfile
echo "alter_CruiseConfig End:---"


echo "5.alter_GBLogicDeviceConfig Begin:---"	
sqlfile="/data/alter_GBLogicDeviceConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_GBLogicDeviceConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_GBLogicDeviceConfig.txt
rm -rf  $sqlfile
echo "alter_GBLogicDeviceConfig End:---"


echo "6.alter_GBPhyDeviceConfig Begin:---"	
sqlfile="/data/alter_GBPhyDeviceConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_GBPhyDeviceConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_GBPhyDeviceConfig.txt
rm -rf  $sqlfile
echo "alter_GBPhyDeviceConfig End:---"


echo "7.alter_IVASHisDiagnosis Begin:---"	
sqlfile="/data/alter_IVASHisDiagnosis.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_IVASHisDiagnosis.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_IVASHisDiagnosis.txt
rm -rf  $sqlfile
echo "alter_IVASHisDiagnosis End:---"


echo "8.alter_IVASRealDiagnosis Begin:---"	
sqlfile="/data/alter_IVASRealDiagnosis.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_IVASRealDiagnosis.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_IVASRealDiagnosis.txt
rm -rf  $sqlfile
echo "alter_IVASRealDiagnosis End:---"


echo "9.alter_LogicDeviceMapGroupConfig Begin:---"
sqlfile="/data/alter_LogicDeviceMapGroupConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_LogicDeviceMapGroupConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_LogicDeviceMapGroupConfig.txt
rm -rf  $sqlfile
echo "alter_LogicDeviceGroupConfig End:---"


echo "10.alter_MgwLogicDeviceConfig Begin:---"	
sqlfile="/data/alter_MgwLogicDeviceConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_MgwLogicDeviceConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_MgwLogicDeviceConfig.txt
rm -rf  $sqlfile
echo "alter_MgwLogicDeviceConfig End:---"


echo "11.alter_PlanActionConfig Begin:---"	
sqlfile="/data/alter_PlanActionConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_PlanActionConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_PlanActionConfig.txt
rm -rf  $sqlfile
echo "alter_PlanActionConfig End:---"


echo "12.alter_PlanConfig Begin:---"	
sqlfile="/data/alter_PlanConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_PlanConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_PlanConfig.txt
rm -rf  $sqlfile
echo "alter_PlanConfig End:---"


echo "13.alter_PlanLinkageConfig Begin:---"	
sqlfile="/data/alter_PlanLinkageConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_PlanLinkageConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_PlanLinkageConfig.txt
rm -rf  $sqlfile
echo "alter_PlanLinkageConfig End:---"


echo "14.alter_PollActionConfig Begin:---"	
sqlfile="/data/alter_PollActionConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_PollActionConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_PollActionConfig.txt
rm -rf  $sqlfile
echo "alter_PollActionConfig End:---"


echo "15.alter_PollConfig Begin:---"	
sqlfile="/data/alter_PollConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_PollConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_PollConfig.txt
rm -rf  $sqlfile
echo "alter_PollConfig End:---"


echo "16.alter_RecordSchedConfig Begin:---"	
sqlfile="/data/alter_RecordSchedConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_RecordSchedConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_RecordSchedConfig.txt
rm -rf  $sqlfile
echo "alter_RecordSchedConfig End:---"


echo "17.alter_RecordSchedConfig_one Begin:---"	
sqlfile="/data/alter_RecordSchedConfig_one.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_RecordSchedConfig_one.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_RecordSchedConfig_one.txt
rm -rf  $sqlfile
echo "alter_RecordSchedConfig_one End:---"


echo "18.alter_RecordTimeSchedConfig Begin:---"	
sqlfile="/data/alter_RecordTimeSchedConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_RecordTimeSchedConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_RecordTimeSchedConfig.txt
rm -rf  $sqlfile
echo "alter_RecordTimeSchedConfig End:---"


echo "19.alter_RouteNetConfig Begin:---"	
sqlfile="/data/alter_RouteNetConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_RouteNetConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_RouteNetConfig.txt
rm -rf  $sqlfile
echo "alter_RouteNetConfig End:---"

echo "21.alter_SystemLogRecord Begin:---"	
sqlfile="/data/alter_SystemLogRecord.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_SystemLogRecord.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_SystemLogRecord.txt
rm -rf  $sqlfile
echo "alter_SystemLogRecord End:---"


echo "22.alter_UserConfig Begin:---"	
sqlfile="/data/alter_UserConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_UserConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_UserConfig.txt
rm -rf  $sqlfile
echo "alter_UserConfig End:---"


echo "23.alter_UserLogicDeviceMapGroupConfig Begin:---"	
sqlfile="/data/alter_UserLogicDeviceMapGroupConfig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_UserLogicDeviceMapGroupConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_UserLogicDeviceMapGroupConfig.txt
rm -rf  $sqlfile
echo "alter_UserLogicDeviceMapGroupConfig End:---"


echo "24.alter_UserLogRecord Begin:---"	
sqlfile="/data/alter_UserLogRecord.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/alter_UserLogRecord.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_UserLogRecord.txt
rm -rf  $sqlfile
echo "alter_UserLogRecord End:---"


echo "�޸��ֻ�MMS���ݿ��:����---"

echo "*******************************************************************"

echo "��.�����ֻ�MMS���ݿ������:��ʼ---"
echo "1.create_Ev9000DB_MOBILE_Index Begin:---"	
sqlfile="/data/create_Ev9000DB_MOBILE_Index.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/create_Ev9000DB_MOBILE_Index.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/create_Ev9000DB_MOBILE_Index.txt
rm -rf  $sqlfile
echo "create_Ev9000DB_MOBILE_Index End:---"


echo "2.create_Ev9000LOG_MOBILE_Index Begin:---"	
sqlfile="/data/create_Ev9000LOG_MOBILE_Index.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/create_Ev9000LOG_MOBILE_Index.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/create_Ev9000LOG_MOBILE_Index.txt
rm -rf  $sqlfile
echo "create_Ev9000LOG_MOBILE_Index End:---"


echo "�����ֻ�MMS���ݿ������:����---"

echo "*******************************************************************"

echo "��.�����ֻ�MMS���ݿ���ͼ:��ʼ---"


echo "view_LogicDevUserPerm Begin:---"	
sqlfile="/data/view_LogicDevUserPerm.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/view_LogicDevUserPerm.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/view_LogicDevUserPerm.txt
rm -rf  $sqlfile
echo "view_LogicDevUserPerm End:---"


echo "�����ֻ�MMS���ݿ���ͼ:����---"

echo "*******************************************************************"

echo "��.�����ֻ�MMS���ݿⴥ����:��ʼ---"

source /app/Create_trig_mobile.sh

echo "�����ֻ�MMS���ݿⴥ����:����---"

echo "*******************************************************************"

echo "��.�����ֻ�MMS���ݿ��¼�:��ʼ---"


echo "1.event_HisDiagnosis_daily Begin:---"
sqlfile="/data/event_HisDiagnosis_daily.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/event_HisDiagnosis_daily.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/event_HisDiagnosis_daily.txt
rm -rf  $sqlfile
echo "event_HisDiagnosis_daily End:---"


echo "2.Event_ManageRecord Begin:---"
sqlfile="/data/Event_ManageRecord.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/Event_ManageRecord.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/Event_ManageRecord.txt
rm -rf  $sqlfile
echo "Event_ManageRecord End:---"


echo "3.Event_ststemlog Begin:---"
sqlfile="/data/Event_ststemlog.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/Event_ststemlog.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/Event_ststemlog.txt
rm -rf  $sqlfile
echo "Event_ststemlog End:---"


echo "4.Event_userlog Begin:---"
sqlfile="/data/Event_userlog.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/Event_userlog.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/Event_userlog.txt
rm -rf  $sqlfile
echo "Event_userlog End:---"


echo "�����ֻ�MMS���ݿ��¼�:����---"

echo "*******************************************************************"

echo "��.�����ֻ�MMS���ݿ�Ĭ������:��ʼ---"
echo "�����ֻ�MMS���ݿ�Ĭ������:����---"

echo "*******************************************************************"

echo "#####ִ���ֻ�MMS���ݿⴴ���ű�:����#####"
