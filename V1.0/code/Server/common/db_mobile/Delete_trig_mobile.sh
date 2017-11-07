#!/bin/sh
#------------------------------------------------------------main--------------------------------------------
cd /data

echo "#####执行手机MMS删除数据库触发器脚本:开始#####"

echo "*******************************************************************"

sqlfile="/data/del_EV9000_mobile_trig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/del_EV9000_mobile_trig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/del_EV9000_mobile_trig.txt
rm -rf  $sqlfile

echo "*******************************************************************"

echo "#####执行手机MMS删除数据库触发器脚本:结束#####"