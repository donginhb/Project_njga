#!/bin/sh
#------------------------------------------------------------main--------------------------------------------
cd /data

echo "#####ִ���ֻ�MMS���ݿ��޸��ű�:��ʼ#####"

echo "*******************************************************************"

echo "һ.ɾ���ֻ�MMSԴ���ݿ��еĴ�����:��ʼ---"

source /app/Delete_trig_mobile.sh

echo "һ.ɾ���ֻ�MMSԴ���ݿ��еĴ�����:����---"

echo "*******************************************************************"

echo "��.ִ���ֻ�MMS���ݿ��޸�����:��ʼ---"

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

echo "��.ִ���ֻ�MMS���ݿ��޸�����:����---"

echo "*******************************************************************"

echo "��.�ָ��ֻ�MMSԴ���ݿ��еĴ�����:��ʼ---"

source /app/Create_trig_mobile.sh

echo "��.�ָ��ֻ�MMSԴ���ݿ��еĴ�����:����---"

echo "*******************************************************************"

echo "#####ִ���ֻ�MMS���ݿ��޸��ű�:����#####"