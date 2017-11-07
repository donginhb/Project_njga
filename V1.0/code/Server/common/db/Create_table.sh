#!/bin/sh
#------------------------------------------------------------main--------------------------------------------
cd /data

echo "#####执行数据库创建脚本:开始#####"

mysql_upgrade --force -uroot -pwiscom

echo "*******************************************************************"

echo "一.创建数据库表:开始---"


echo "1.create_Ev9000DB Begin:---"
sqlfile="/data/create_Ev9000DB.txt"
rm -rf  $sqlfile
cp /app/update_file/create_Ev9000DB.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/create_Ev9000DB.txt
rm -rf  $sqlfile
echo "create_Ev9000DB End:---"


echo "2.create_Ev9000LOG Begin:---"
sqlfile="/data/create_Ev9000LOG.txt"
rm -rf  $sqlfile
cp /app/update_file/create_Ev9000LOG.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/create_Ev9000LOG.txt
rm -rf  $sqlfile
echo "create_Ev9000LOG End:---"


echo "3.create_Ev9000MY Begin:---"
sqlfile="/data/create_Ev9000MY.txt"
rm -rf  $sqlfile
cp /app/update_file/create_Ev9000MY.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/create_Ev9000MY.txt
rm -rf  $sqlfile
echo "create_Ev9000MY End:---"


echo "4.create_Ev9000TSU Begin:---"
sqlfile="/data/create_Ev9000TSU.txt"
rm -rf  $sqlfile
cp /app/update_file/create_Ev9000TSU.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/create_Ev9000TSU.txt
rm -rf  $sqlfile
echo "create_Ev9000TSU End:---"

echo "创建数据库表:结束---"

echo "*******************************************************************"

echo "二.修改数据库表:开始---"


echo "1.alter_AlarmRecord Begin:---"
sqlfile="/data/alter_AlarmRecord.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_AlarmRecord.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_AlarmRecord.txt
rm -rf  $sqlfile
echo "alter_AlarmRecord End:---"


echo "2.alter_BoardConfig Begin:---"	
sqlfile="/data/alter_BoardConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_BoardConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_BoardConfig.txt
rm -rf  $sqlfile
echo "alter_BoardConfig End:---"


echo "3.alter_CruiseActionConfig Begin:---"	
sqlfile="/data/alter_CruiseActionConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_CruiseActionConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_CruiseActionConfig.txt
rm -rf  $sqlfile
echo "alter_CruiseActionConfig End:---"


echo "4.alter_CruiseConfig Begin:---"	
sqlfile="/data/alter_CruiseConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_CruiseConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_CruiseConfig.txt
rm -rf  $sqlfile
echo "alter_CruiseConfig End:---"


echo "5.alter_GBLogicDeviceConfig Begin:---"	
sqlfile="/data/alter_GBLogicDeviceConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_GBLogicDeviceConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_GBLogicDeviceConfig.txt
rm -rf  $sqlfile
echo "alter_GBLogicDeviceConfig End:---"


echo "6.alter_GBPhyDeviceConfig Begin:---"	
sqlfile="/data/alter_GBPhyDeviceConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_GBPhyDeviceConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_GBPhyDeviceConfig.txt
rm -rf  $sqlfile
echo "alter_GBPhyDeviceConfig End:---"


echo "7.alter_IVASHisDiagnosis Begin:---"	
sqlfile="/data/alter_IVASHisDiagnosis.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_IVASHisDiagnosis.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_IVASHisDiagnosis.txt
rm -rf  $sqlfile
echo "alter_IVASHisDiagnosis End:---"


echo "8.alter_IVASRealDiagnosis Begin:---"	
sqlfile="/data/alter_IVASRealDiagnosis.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_IVASRealDiagnosis.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_IVASRealDiagnosis.txt
rm -rf  $sqlfile
echo "alter_IVASRealDiagnosis End:---"


echo "9.alter_LogicDeviceGroupConfig Begin:---"
sqlfile="/data/alter_LogicDeviceGroupConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_LogicDeviceGroupConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_LogicDeviceGroupConfig.txt
rm -rf  $sqlfile
echo "alter_LogicDeviceGroupConfig End:---"


echo "10.alter_MgwLogicDeviceConfig Begin:---"	
sqlfile="/data/alter_MgwLogicDeviceConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_MgwLogicDeviceConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_MgwLogicDeviceConfig.txt
rm -rf  $sqlfile
echo "alter_MgwLogicDeviceConfig End:---"


echo "11.alter_PlanActionConfig Begin:---"	
sqlfile="/data/alter_PlanActionConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_PlanActionConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_PlanActionConfig.txt
rm -rf  $sqlfile
echo "alter_PlanActionConfig End:---"


echo "12.alter_PlanConfig Begin:---"	
sqlfile="/data/alter_PlanConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_PlanConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_PlanConfig.txt
rm -rf  $sqlfile
echo "alter_PlanConfig End:---"


echo "13.alter_PlanLinkageConfig Begin:---"	
sqlfile="/data/alter_PlanLinkageConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_PlanLinkageConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_PlanLinkageConfig.txt
rm -rf  $sqlfile
echo "alter_PlanLinkageConfig End:---"


echo "14.alter_PollActionConfig Begin:---"	
sqlfile="/data/alter_PollActionConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_PollActionConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_PollActionConfig.txt
rm -rf  $sqlfile
echo "alter_PollActionConfig End:---"


echo "15.alter_PollConfig Begin:---"	
sqlfile="/data/alter_PollConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_PollConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_PollConfig.txt
rm -rf  $sqlfile
echo "alter_PollConfig End:---"


echo "16.alter_RecordSchedConfig Begin:---"	
sqlfile="/data/alter_RecordSchedConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_RecordSchedConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_RecordSchedConfig.txt
rm -rf  $sqlfile
echo "alter_RecordSchedConfig End:---"


echo "17.alter_RecordSchedConfig_one Begin:---"	
sqlfile="/data/alter_RecordSchedConfig_one.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_RecordSchedConfig_one.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_RecordSchedConfig_one.txt
rm -rf  $sqlfile
echo "alter_RecordSchedConfig_one End:---"


echo "18.alter_RecordTimeSchedConfig Begin:---"	
sqlfile="/data/alter_RecordTimeSchedConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_RecordTimeSchedConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_RecordTimeSchedConfig.txt
rm -rf  $sqlfile
echo "alter_RecordTimeSchedConfig End:---"


echo "19.alter_RouteNetConfig Begin:---"	
sqlfile="/data/alter_RouteNetConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_RouteNetConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_RouteNetConfig.txt
rm -rf  $sqlfile
echo "alter_RouteNetConfig End:---"


echo "20.alter_ServerConfig Begin:---"	
sqlfile="/data/alter_ServerConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_ServerConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_ServerConfig.txt
rm -rf  $sqlfile
echo "alter_ServerConfig End:---"


echo "21.alter_SystemLogRecord Begin:---"	
sqlfile="/data/alter_SystemLogRecord.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_SystemLogRecord.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_SystemLogRecord.txt
rm -rf  $sqlfile
echo "alter_SystemLogRecord End:---"


echo "22.alter_UserConfig Begin:---"	
sqlfile="/data/alter_UserConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_UserConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_UserConfig.txt
rm -rf  $sqlfile
echo "alter_UserConfig End:---"


echo "23.alter_UserLogicDeviceMapGroupConfig Begin:---"	
sqlfile="/data/alter_UserLogicDeviceMapGroupConfig.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_UserLogicDeviceMapGroupConfig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_UserLogicDeviceMapGroupConfig.txt
rm -rf  $sqlfile
echo "alter_UserLogicDeviceMapGroupConfig End:---"


echo "24.alter_UserLogRecord Begin:---"	
sqlfile="/data/alter_UserLogRecord.txt"
rm -rf  $sqlfile
cp /app/update_file/alter_UserLogRecord.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/alter_UserLogRecord.txt
rm -rf  $sqlfile
echo "alter_UserLogRecord End:---"


echo "修改数据库表:结束---"

echo "*******************************************************************"

echo "三.创建数据库表索引:开始---"


echo "1.create_Ev9000DB_Index Begin:---"	
sqlfile="/data/create_Ev9000DB_Index.txt"
rm -rf  $sqlfile
cp /app/update_file/create_Ev9000DB_Index.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/create_Ev9000DB_Index.txt
rm -rf  $sqlfile
echo "create_Ev9000DB_Index End:---"


echo "2.create_Ev9000LOG_Index Begin:---"	
sqlfile="/data/create_Ev9000LOG_Index.txt"
rm -rf  $sqlfile
cp /app/update_file/create_Ev9000LOG_Index.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/create_Ev9000LOG_Index.txt
rm -rf  $sqlfile
echo "create_Ev9000LOG_Index End:---"


echo "3.create_Ev9000TSU_Index Begin:---"	
sqlfile="/data/create_Ev9000TSU_Index.txt"
rm -rf  $sqlfile
cp /app/update_file/create_Ev9000TSU_Index.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/create_Ev9000TSU_Index.txt
rm -rf  $sqlfile
echo "create_Ev9000TSU_Index End:---"


echo "创建数据库表索引:结束---"

echo "*******************************************************************"

echo "四.创建数据库视图:开始---"


echo "view_LogicDevUserPerm Begin:---"	
sqlfile="/data/view_LogicDevUserPerm.txt"
rm -rf  $sqlfile
cp /app/update_file/view_LogicDevUserPerm.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/view_LogicDevUserPerm.txt
rm -rf  $sqlfile
echo "view_LogicDevUserPerm End:---"


echo "创建数据库视图:结束---"

echo "*******************************************************************"

echo "五.创建数据库触发器:开始---"

. /app/Create_trig.sh

echo "创建数据库触发器:结束---"

echo "*******************************************************************"

echo "六.创建数据库事件:开始---"


echo "1.event_HisDiagnosis_daily Begin:---"
sqlfile="/data/event_HisDiagnosis_daily.txt"
rm -rf  $sqlfile
cp /app/update_file/event_HisDiagnosis_daily.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/event_HisDiagnosis_daily.txt
rm -rf  $sqlfile
echo "event_HisDiagnosis_daily End:---"


echo "2.event_ManageRecord Begin:---"
sqlfile="/data/event_ManageRecord.txt"
rm -rf  $sqlfile
cp /app/update_file/event_ManageRecord.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/event_ManageRecord.txt
rm -rf  $sqlfile
echo "event_ManageRecord End:---"


echo "3.event_ststemlog Begin:---"
sqlfile="/data/event_ststemlog.txt"
rm -rf  $sqlfile
cp /app/update_file/event_ststemlog.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/event_ststemlog.txt
rm -rf  $sqlfile
echo "event_ststemlog End:---"


echo "4.event_userlog Begin:---"
sqlfile="/data/event_userlog.txt"
rm -rf  $sqlfile
cp /app/update_file/event_userlog.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/event_userlog.txt
rm -rf  $sqlfile
echo "event_userlog End:---"


echo "创建数据库事件:结束---"

echo "*******************************************************************"

echo "七.插入数据库默认数据:开始---"


echo "insert_default_data Begin:---"
sqlfile="/data/insert_default_data.txt"
rm -rf  $sqlfile
cp /app/update_file/insert_default_data.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/insert_default_data.txt
rm -rf  $sqlfile
echo "insert_default_data End:---"


echo "插入数据库默认数据:结束---"

echo "*******************************************************************"

echo "#####执行数据库创建脚本:结束#####"
