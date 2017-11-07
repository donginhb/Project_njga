#!/bin/sh
#------------------------------------------------------------main--------------------------------------------
cd /data

echo "#####执行创建数据库触发器脚本:开始#####"

echo "*******************************************************************"

echo "1.add_trig_CruiseCon_del Begin:---"
sqlfile="/data/add_trig_CruiseCon_del.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_CruiseCon_del.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_CruiseCon_del.txt
rm -rf  $sqlfile
echo "add_trig_CruiseCon_del End:---"


echo "2.add_trig_GBLogicDev_del Begin:---"
sqlfile="/data/add_trig_GBLogicDev_del.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_GBLogicDev_del.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_GBLogicDev_del.txt
rm -rf  $sqlfile
echo "add_trig_GBLogicDev_del End:---"


echo "3.add_trig_GBLogicDev_enable Begin:---"
sqlfile="/data/add_trig_GBLogicDev_enable.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_GBLogicDev_enable.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_GBLogicDev_enable.txt
rm -rf  $sqlfile
echo "add_trig_GBLogicDev_enable End:---"


echo "4.add_trig_GBLogicDev_update Begin:---"
sqlfile="/data/add_trig_GBLogicDev_update.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_GBLogicDev_update.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_GBLogicDev_update.txt
rm -rf  $sqlfile
echo "add_trig_GBLogicDev_update End:---"


echo "5.add_trig_GBPhyDev_del Begin:---"
sqlfile="/data/add_trig_GBPhyDev_del.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_GBPhyDev_del.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_GBPhyDev_del.txt
rm -rf  $sqlfile
echo "add_trig_GBPhyDev_del End:---"


echo "6.add_trig_GBPhyDev_enable Begin:---"
sqlfile="/data/add_trig_GBPhyDev_enable.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_GBPhyDev_enable.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_GBPhyDev_enable.txt
rm -rf  $sqlfile
echo "add_trig_GBPhyDev_enable End:---"


echo "7.add_trig_LogicDeviceGroup_del Begin:---"
sqlfile="/data/add_trig_LogicDeviceGroup_del.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_LogicDeviceGroup_del.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_LogicDeviceGroup_del.txt
rm -rf  $sqlfile
echo "add_trig_LogicDeviceGroup_del End:---"


echo "8.add_trig_MgwLogicDevice_del Begin:---"	
sqlfile="/data/add_trig_MgwLogicDevice_del.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_MgwLogicDevice_del.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_MgwLogicDevice_del.txt
rm -rf  $sqlfile
echo "add_trig_MgwLogicDevice_del End:---"


echo "9.add_trig_MgwLogicDevice_enable Begin:---"	
sqlfile="/data/add_trig_MgwLogicDevice_enable.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_MgwLogicDevice_enable.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_MgwLogicDevice_enable.txt
rm -rf  $sqlfile
echo "add_trig_MgwLogicDevice_enable End:---"


echo "10.add_trig_PlanCon_del Begin:---"
sqlfile="/data/add_trig_PlanCon_del.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_PlanCon_del.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_PlanCon_del.txt
rm -rf  $sqlfile
echo "add_trig_PlanCon_del End:---"


echo "11.add_trig_PollCon_del Begin:---"
sqlfile="/data/add_trig_PollCon_del.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_PollCon_del.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_PollCon_del.txt
rm -rf  $sqlfile
echo "add_trig_PollCon_del End:---"


echo "12.add_trig_RouteNetConfig_del Begin:---"	
sqlfile="/data/add_trig_RouteNetConfig_del.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_RouteNetConfig_del.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_RouteNetConfig_del.txt
rm -rf  $sqlfile
echo "add_trig_RouteNetConfig_del End:---"


echo "13.add_trig_UserCon_del Begin:---"
sqlfile="/data/add_trig_UserCon_del.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_UserCon_del.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_UserCon_del.txt
rm -rf  $sqlfile
echo "add_trig_UserCon_del End:---"


echo "14.add_trig_UserCon_enable Begin:---"
sqlfile="/data/add_trig_UserCon_enable.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_UserCon_enable.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_UserCon_enable.txt
rm -rf  $sqlfile
echo "add_trig_UserCon_enable End:---"


echo "15.add_trig_PresetConfig_del Begin:---"
sqlfile="/data/add_trig_PresetConfig_del.txt"
rm -rf  $sqlfile
cp /app/update_file/add_trig_PresetConfig_del.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/add_trig_PresetConfig_del.txt
rm -rf  $sqlfile
echo "add_trig_PresetConfig_del End:---"

echo "*******************************************************************"

echo "#####执行创建数据库触发器脚本:结束#####"