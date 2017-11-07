#!/bin/sh

#���ݿ��ļ��ж���
OldPartitionDir="/data/"
OldPartitionDBDir="/data/db"

NewPartitionDir="/data1/"
NewPartitionDBDir="/data1/db"

LogFolderOld="/data/log"
LogFolderNew="/data1/log"

DBFolderOld1="/data/db/EV9000LOG"
DBFolderOld2="/data/db/EV9000LOG_MOBILE"
DBFolderOld3="/data/db/EV9000TSU"
DBFolderOld4="/data/db/EV9000TSU_MOBILE"

DBFolderNew1="/data1/db/EV9000LOG"
DBFolderNew2="/data1/db/EV9000LOG_MOBILE"
DBFolderNew3="/data1/db/EV9000TSU"
DBFolderNew4="/data1/db/EV9000TSU_MOBILE"

echo "#####ִ��Ӳ�̷����ű�:��ʼ#####"
echo "*******************************************************************"

echo "һ.�ر�Ӧ�ó������:��ʼ---"

echo "1.�ر�FTP����:---"
source /etc/stop_ftp

echo "2.�ر�MySQL����:---"
source /etc/stop_mysql

echo "3.�ر�TSU��ؽ���:---"
source /app/kill.sh

echo "4.�ر�RTSP��ؽ���:---"
killall monitor_rtsp.sh rtspserver

echo "5.�ر�MMS��ؽ���:---"
killall app_monitor_mobile.sh mms phddns

echo "6.�ر�CMS��ؽ���:---"
killall app_monitor.sh cms KeyBoardCtrl ntpd Ver_Update EV9000MediaService rinetd

echo "6.�ر�Web������ؽ���:---"
killall start_webserver appweb

echo "�ر�Ӧ�ó������:����---"

sleep 5

echo "*******************************************************************"

echo "��.ж��ԭ��ϵͳӲ�̷�������:��ʼ---"

umount /data/

if [ $? -eq 0 ]
then
    echo "ж��ԭ��ϵͳӲ�̷���/data/�ɹ�"
else
    echo "ж��ԭ��ϵͳӲ�̷���/data/ʧ��"
    exit
fi

umount /home/ftpuser/

if [ $? -eq 0 ]
then
    echo "ж��ԭ��ϵͳӲ�̷���/home/ftpuser/�ɹ�"
else
    echo "ж��ԭ��ϵͳӲ�̷���/home/ftpuser/ʧ��"
    exit
fi

echo "ж��ԭ��ϵͳӲ�̷�������:����---"

sleep 5

echo "*******************************************************************"

echo "��.ִ�з����ű�:��ʼ---"

source /etc/init_ssd

if [ $? -eq 0 ]
then
    echo "ִ�з����ű��ɹ�"
else
    echo "ִ�з����ű�ʧ��"
    exit
fi

echo "ִ�з����ű�:����---"

sleep 5

echo "*******************************************************************"

echo "��.���¹���ϵͳӲ�̷���:��ʼ---"

mount /dev/sda1 /data/

if [ $? -eq 0 ]
then
    echo "����Ӳ�̷���/data/�ɹ�"
else
    echo "����Ӳ�̷���/data/ʧ��"
    exit
fi

mount /dev/sda1 /home/ftpuser/

if [ $? -eq 0 ]
then
    echo "����Ӳ�̷���/home/ftpuser/�ɹ�"
else
    echo "����Ӳ�̷���/home/ftpuser/ʧ��"
    exit
fi

mount /dev/sda2 /data1/

if [ $? -eq 0 ]
then
    echo "����Ӳ�̷���/data1/�ɹ�"
else
    echo "����Ӳ�̷���/data1/ʧ��"
    exit
fi

echo "���¹���ϵͳӲ�̷���:����---"

sleep 5

echo "*******************************************************************"

echo "��.����ϵͳ������:��ʼ---"

echo "1.�����������ݿ�Ŀ¼---"
echo �Ϸ���Ŀ¼:$OldPartitionDBDir
if [ ! -d "$OldPartitionDBDir" ]; 
then
 mkdir "$OldPartitionDBDir"
else
 echo "�����Ϸ������ݿ�Ŀ¼�Ѿ�����,����Ҫ����"
fi

if [ $? -eq 0 ]
then
    echo "�����Ϸ������ݿ�Ŀ¼�ɹ�"
else
    echo "�����Ϸ������ݿ�Ŀ¼ʧ��"
    exit
fi

echo �·���Ŀ¼:$NewPartitionDBDir
if [ ! -d "$NewPartitionDBDir" ]; 
then
 mkdir "$NewPartitionDBDir"
else
 echo "�����·������ݿ�Ŀ¼�Ѿ�����,����Ҫ����"
fi

if [ $? -eq 0 ]
then
    echo "�����·������ݿ�Ŀ¼�ɹ�"
else
    echo "�����·������ݿ�Ŀ¼ʧ��"
    exit
fi

echo "2.���������ӣ���Ƶ������������Ŀ¼���ӵ��·���/data1����,��ʼ---"
echo 2.1 ԴĿ¼:$LogFolderNew "--->>>" ����Ŀ¼:$LogFolderOld
if [ ! -L "$LogFolderOld" ]; 
then
 ln -s $LogFolderNew $LogFolderOld
else
 echo "�������Ѿ�����,����Ҫ����"
fi

if [ $? -eq 0 ]
then
    echo ���� ԴĿ¼:$LogFolderNew "--->>>" ����Ŀ¼:$LogFolderOld �����ӳɹ�
else
    echo ���� ԴĿ¼:$LogFolderNew "--->>>" ����Ŀ¼:$LogFolderOld ������ʧ��
    exit
fi

echo 2.2 ԴĿ¼:$DBFolderNew1 "--->>>" ����Ŀ¼:$DBFolderOld1
if [ ! -L "$DBFolderOld1" ]; 
then
 ln -s $DBFolderNew1 $DBFolderOld1
else
 echo "�������Ѿ�����,����Ҫ����"
fi

if [ $? -eq 0 ]
then
    echo ���� ԴĿ¼:$DBFolderNew1 "--->>>" ����Ŀ¼:$DBFolderOld1 �����ӳɹ�
else
    echo ���� ԴĿ¼:$DBFolderNew1 "--->>>" ����Ŀ¼:$DBFolderOld1 ������ʧ��
    exit
fi

echo 2.3 ԴĿ¼:$DBFolderNew2 "--->>>" ����Ŀ¼:$DBFolderOld2
if [ ! -L "$DBFolderOld2" ]; 
then
 ln -s $DBFolderNew2 $DBFolderOld2
else
 echo "�������Ѿ�����,����Ҫ����"
fi

if [ $? -eq 0 ]
then
    echo ���� ԴĿ¼:$DBFolderNew2 "--->>>" ����Ŀ¼:$DBFolderOld2 �����ӳɹ�
else
    echo ���� ԴĿ¼:$DBFolderNew2 "--->>>" ����Ŀ¼:$DBFolderOld2 ������ʧ��
    exit
fi

echo 2.4 ԴĿ¼:$DBFolderNew3 "--->>>" ����Ŀ¼:$DBFolderOld3
if [ ! -L "$DBFolderOld3" ]; 
then
 ln -s $DBFolderNew3 $DBFolderOld3
else
 echo "�������Ѿ�����,����Ҫ����"
fi

if [ $? -eq 0 ]
then
    echo ���� ԴĿ¼:$DBFolderNew3 "--->>>" ����Ŀ¼:$DBFolderOld3 �����ӳɹ�
else
    echo ���� ԴĿ¼:$DBFolderNew3 "--->>>" ����Ŀ¼:$DBFolderOld3 ������ʧ��
    exit
fi

echo 2.5 ԴĿ¼:$DBFolderNew4 "--->>>" ����Ŀ¼:$DBFolderOld4
if [ ! -L "$DBFolderOld4" ]; 
then
 ln -s $DBFolderNew4 $DBFolderOld4
else
 echo "�������Ѿ�����,����Ҫ����"
fi

if [ $? -eq 0 ]
then
    echo ���� ԴĿ¼:$DBFolderNew4 "--->>>" ����Ŀ¼:$DBFolderOld4 �����ӳɹ�
else
    echo ���� ԴĿ¼:$DBFolderNew4 "--->>>" ����Ŀ¼:$DBFolderOld4 ������ʧ��
    exit
fi

echo "3.����µ�����Ŀ¼�Ƿ����,��ʼ---"

echo 3.1 �·���Ŀ¼:$LogFolderNew
if [ ! -d "$LogFolderNew" ]; 
then
 mkdir "$LogFolderNew"
else
 echo "�·���Ŀ¼�Ѿ�����,����Ҫ����"
fi

if [ $? -eq 0 ]
then
    echo ���� �·���Ŀ¼:$LogFolderNew �ɹ�
else
    echo ���� �·���Ŀ¼:$LogFolderNew ʧ��
    exit
fi

echo 3.2 �·���Ŀ¼:$DBFolderNew1
if [ ! -d "$DBFolderNew1" ]; 
then
 mkdir "$DBFolderNew1"
else
 echo "�·���Ŀ¼�Ѿ�����,����Ҫ����"
fi

if [ $? -eq 0 ]
then
    echo ���� �·���Ŀ¼:$DBFolderNew1 �ɹ�
else
    echo ���� �·���Ŀ¼:$DBFolderNew1 ʧ��
    exit
fi

echo 3.3 �·���Ŀ¼:$DBFolderNew2
if [ ! -d "$DBFolderNew2" ]; 
then
 mkdir "$DBFolderNew2"
else
 echo "�·���Ŀ¼�Ѿ�����,����Ҫ����"
fi

if [ $? -eq 0 ]
then
    echo ���� �·���Ŀ¼:$DBFolderNew2 �ɹ�
else
    echo ���� �·���Ŀ¼:$DBFolderNew2 ʧ��
    exit
fi

echo 3.4 �·���Ŀ¼:$DBFolderNew3
if [ ! -d "$DBFolderNew3" ]; 
then
 mkdir "$DBFolderNew3"
else
 echo "�·���Ŀ¼�Ѿ�����,����Ҫ����"
fi

if [ $? -eq 0 ]
then
    echo ���� �·���Ŀ¼:$DBFolderNew3 �ɹ�
else
    echo ���� �·���Ŀ¼:$DBFolderNew3 ʧ��
    exit
fi

echo 3.5 �·���Ŀ¼:$DBFolderNew4
if [ ! -d "$DBFolderNew4" ]; 
then
 mkdir "$DBFolderNew4"
else
 echo "�·���Ŀ¼�Ѿ�����,����Ҫ����"
fi

if [ $? -eq 0 ]
then
    echo ���� �·���Ŀ¼:$DBFolderNew4 �ɹ�
else
    echo ���� �·���Ŀ¼:$DBFolderNew4 ʧ��
    exit
fi

echo "����ϵͳ������:����---"

echo "*******************************************************************"
echo "#####ִ��Ӳ�̷����ű�:����#####"