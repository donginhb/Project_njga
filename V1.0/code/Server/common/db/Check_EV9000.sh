#!/bin/sh
#------------------------------------------------------------main--------------------------------------------

echo "#####ִ�����ݿ���ű�:��ʼ#####"

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

echo "#####ִ�����ݿ���ű�:����#####"