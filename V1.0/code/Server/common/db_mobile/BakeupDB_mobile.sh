#!/bin/sh
#------------------------------------------------------------main--------------------------------------------
cd /data

echo "#####执行手机MMS配置数据数据库备份脚本:开始#####"

echo "*******************************************************************"

echo "一.删除手机MMS源数据库中的触发器:开始---"

source /app/Delete_trig_mobile.sh

echo "一.删除手机MMS源数据库中的触发器:结束---"

echo "*******************************************************************"

echo "二.执行手机MMS备份数据库命令:开始---"

mysqldump -uroot -pwiscom --host=$1 --opt EV9000DB_MOBILE | mysql -uroot -pwiscom --host=$2 -C EV9000DB_MOBILE
mysqldump -uroot -pwiscom --host=$1 --opt app_server_db | mysql -uroot -pwiscom --host=$2 -C app_server_db

echo "二.执行手机MMS备份数据库命令:结束---"

echo "*******************************************************************"

echo "三.恢复手机MMS源数据库中的触发器:开始---"

source /app/Create_trig_mobile.sh

echo "三.恢复手机MMS源数据库中的触发器:结束---"

echo "*******************************************************************"

echo "#####执行手机MMS配置数据数据库备份脚本:结束#####"