#!/bin/sh
#------------------------------------------------------------main--------------------------------------------
cd /data

echo "#####ִ���ֻ�MMSɾ�����ݿⴥ�����ű�:��ʼ#####"

echo "*******************************************************************"

sqlfile="/data/del_EV9000_mobile_trig.txt"
rm -rf  $sqlfile
cp /app/update_file_mobile/del_EV9000_mobile_trig.txt /data/
mysql -uroot -pwiscom --host=$1 < /data/del_EV9000_mobile_trig.txt
rm -rf  $sqlfile

echo "*******************************************************************"

echo "#####ִ���ֻ�MMSɾ�����ݿⴥ�����ű�:����#####"