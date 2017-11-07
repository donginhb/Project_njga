#!/bin/sh
#------------------------------------------------------------main--------------------------------------------

echo "#####执行数据库检测脚本:开始#####"

echo "*******************************************************************"

myisamchk -c -r /data/db/EV9000TSU/*.MYI
myisamchk -c -r /data/db/EV9000DB/*.MYI
myisamchk -c -r /data/db/EV9000DB_MY/*.MYI
myisamchk -c -r /data/db/EV9000LOG/*.MYI

myisamchk -c -r /data/db/app_server_db/*.MYI
myisamchk -c -r /data/db/EV9000DB_MOBILE/*.MYI
myisamchk -c -r /data/db/EV9000DB_MY_MOBILE/*.MYI
myisamchk -c -r /data/db/EV9000LOG_MOBILE/*.MYI
myisamchk -c -r /data/db/EV9000TSU_MOBILE/*.MYI

echo "*******************************************************************"

echo "#####执行数据库检测脚本:结束#####"