#!/bin/sh
#------------------------------------------------------------main--------------------------------------------
cd /data

echo "#####ִ�����ݿ��޸��ű�:��ʼ#####"

echo "*******************************************************************"

echo "һ.ɾ��Դ���ݿ��еĴ�����:��ʼ---"

. /app/Delete_trig.sh

echo "һ.ɾ��Դ���ݿ��еĴ�����:����---"

echo "*******************************************************************"

echo "��.ִ�����ݿ��޸�����:��ʼ---"

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

echo "��.ִ�����ݿ��޸�����:����---"

echo "*******************************************************************"

echo "��.�ָ�Դ���ݿ��еĴ�����:��ʼ---"

. /app/Create_trig.sh

echo "��.�ָ�Դ���ݿ��еĴ�����:����---"

echo "*******************************************************************"

echo "#####ִ�����ݿ��޸��ű�:����#####"