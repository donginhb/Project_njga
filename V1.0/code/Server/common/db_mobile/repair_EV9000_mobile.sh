#!/bin/sh
#------------------------------------------------------------main--------------------------------------------
cd /data

echo "#####执行手机MMS数据库修复脚本:开始#####"

echo "*******************************************************************"

echo "一.删除手机MMS源数据库中的触发器:开始---"

source /app/Delete_trig_mobile.sh

echo "一.删除手机MMS源数据库中的触发器:结束---"

echo "*******************************************************************"

echo "二.执行手机MMS数据库修复命令:开始---"

echo "repair_app_server_db Begin:---"
sqlfile="/data/repair_app_server_db.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/repair_app_server_db.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/repair_app_server_db.txt
rm -rf  $sqlfile
echo "repair_app_server_db End:---"

echo "repair_Ev9000DB_MOBILE Begin:---"
sqlfile="/data/repair_Ev9000DB_MOBILE.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/repair_Ev9000DB_MOBILE.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/repair_Ev9000DB_MOBILE.txt
rm -rf  $sqlfile
echo "repair_Ev9000DB_MOBILE End:---"

echo "repair_Ev9000MY_MOBILE Begin:---"
sqlfile="/data/repair_Ev9000MY_MOBILE.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/repair_Ev9000MY_MOBILE.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/repair_Ev9000MY_MOBILE.txt
rm -rf  $sqlfile
echo "repair_Ev9000MY_MOBILE End:---"

echo "repair_Ev9000TSU_MOBILE Begin:---"
sqlfile="/data/repair_Ev9000TSU_MOBILE.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/repair_Ev9000TSU_MOBILE.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/repair_Ev9000TSU_MOBILE.txt
rm -rf  $sqlfile
echo "repair_Ev9000TSU_MOBILE End:---"


echo "repair_Ev9000LOG_MOBILE Begin:---"
sqlfile="/data/repair_Ev9000LOG_MOBILE.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/repair_Ev9000LOG_MOBILE.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/repair_Ev9000LOG_MOBILE.txt
rm -rf  $sqlfile
echo "repair_Ev9000LOG_MOBILE End:---"

echo "二.执行手机MMS数据库修复命令:结束---"

echo "*******************************************************************"

echo "三.恢复手机MMS源数据库中的触发器:开始---"

source /app/Create_trig_mobile.sh

echo "三.恢复手机MMS源数据库中的触发器:结束---"

echo "*******************************************************************"

echo "#####执行手机MMS数据库修复脚本:结束#####"