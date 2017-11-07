#!/bin/sh
#------------------------------------------------------------main--------------------------------------------
cd /data

echo "#####执行数据库修复脚本:开始#####"

echo "*******************************************************************"

echo "一.删除源数据库中的触发器:开始---"

. /app/Delete_trig.sh

echo "一.删除源数据库中的触发器:结束---"

echo "*******************************************************************"

echo "二.执行数据库修复命令:开始---"

echo "repair_Ev9000DB Begin:---"
sqlfile="/data/repair_Ev9000DB.txt"
rm -rf  $sqlfile
cp /app/update_file/repair_Ev9000DB.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/repair_Ev9000DB.txt
rm -rf  $sqlfile
echo "repair_Ev9000DB End:---"

echo "repair_Ev9000MY Begin:---"
sqlfile="/data/repair_Ev9000MY.txt"
rm -rf  $sqlfile
cp /app/update_file/repair_Ev9000MY.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/repair_Ev9000MY.txt
rm -rf  $sqlfile
echo "repair_Ev9000MY End:---"

echo "repair_Ev9000TSU Begin:---"
sqlfile="/data/repair_Ev9000TSU.txt"
rm -rf  $sqlfile
cp /app/update_file/repair_Ev9000TSU.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/repair_Ev9000TSU.txt
rm -rf  $sqlfile
echo "repair_Ev9000TSU End:---"


echo "repair_Ev9000LOG Begin:---"
sqlfile="/data/repair_Ev9000LOG.txt"
rm -rf  $sqlfile
cp /app/update_file/repair_Ev9000LOG.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/repair_Ev9000LOG.txt
rm -rf  $sqlfile
echo "repair_Ev9000LOG End:---"

echo "二.执行数据库修复命令:结束---"

echo "*******************************************************************"

echo "三.恢复源数据库中的触发器:开始---"

. /app/Create_trig.sh

echo "三.恢复源数据库中的触发器:结束---"

echo "*******************************************************************"

echo "#####执行数据库修复脚本:结束#####"