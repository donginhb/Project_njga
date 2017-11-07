#!/bin/sh

#数据库文件夹定义
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

echo "#####执行硬盘分区脚本:开始#####"
echo "*******************************************************************"

echo "一.关闭应用程序进程:开始---"

echo "1.关闭FTP进程:---"
source /etc/stop_ftp

echo "2.关闭MySQL进程:---"
source /etc/stop_mysql

echo "3.关闭TSU相关进程:---"
source /app/kill.sh

echo "4.关闭RTSP相关进程:---"
killall monitor_rtsp.sh rtspserver

echo "5.关闭MMS相关进程:---"
killall app_monitor_mobile.sh mms phddns

echo "6.关闭CMS相关进程:---"
killall app_monitor.sh cms KeyBoardCtrl ntpd Ver_Update EV9000MediaService rinetd

echo "6.关闭Web服务相关进程:---"
killall start_webserver appweb

echo "关闭应用程序进程:结束---"

sleep 5

echo "*******************************************************************"

echo "二.卸载原有系统硬盘分区挂载:开始---"

umount /data/

if [ $? -eq 0 ]
then
    echo "卸载原有系统硬盘分区/data/成功"
else
    echo "卸载原有系统硬盘分区/data/失败"
    exit
fi

umount /home/ftpuser/

if [ $? -eq 0 ]
then
    echo "卸载原有系统硬盘分区/home/ftpuser/成功"
else
    echo "卸载原有系统硬盘分区/home/ftpuser/失败"
    exit
fi

echo "卸载原有系统硬盘分区挂载:结束---"

sleep 5

echo "*******************************************************************"

echo "三.执行分区脚本:开始---"

source /etc/init_ssd

if [ $? -eq 0 ]
then
    echo "执行分区脚本成功"
else
    echo "执行分区脚本失败"
    exit
fi

echo "执行分区脚本:结束---"

sleep 5

echo "*******************************************************************"

echo "四.重新挂载系统硬盘分区:开始---"

mount /dev/sda1 /data/

if [ $? -eq 0 ]
then
    echo "挂载硬盘分区/data/成功"
else
    echo "挂载硬盘分区/data/失败"
    exit
fi

mount /dev/sda1 /home/ftpuser/

if [ $? -eq 0 ]
then
    echo "挂载硬盘分区/home/ftpuser/成功"
else
    echo "挂载硬盘分区/home/ftpuser/失败"
    exit
fi

mount /dev/sda2 /data1/

if [ $? -eq 0 ]
then
    echo "挂载硬盘分区/data1/成功"
else
    echo "挂载硬盘分区/data1/失败"
    exit
fi

echo "重新挂载系统硬盘分区:结束---"

sleep 5

echo "*******************************************************************"

echo "五.建立系统软连接:开始---"

echo "1.创建分区数据库目录---"
echo 老分区目录:$OldPartitionDBDir
if [ ! -d "$OldPartitionDBDir" ]; 
then
 mkdir "$OldPartitionDBDir"
else
 echo "创建老分区数据库目录已经存在,不需要创建"
fi

if [ $? -eq 0 ]
then
    echo "创建老分区数据库目录成功"
else
    echo "创建老分区数据库目录失败"
    exit
fi

echo 新分区目录:$NewPartitionDBDir
if [ ! -d "$NewPartitionDBDir" ]; 
then
 mkdir "$NewPartitionDBDir"
else
 echo "创建新分区数据库目录已经存在,不需要创建"
fi

if [ $? -eq 0 ]
then
    echo "创建新分区数据库目录成功"
else
    echo "创建新分区数据库目录失败"
    exit
fi

echo "2.增加软链接，将频繁操作的数据目录链接到新分区/data1上面,开始---"
echo 2.1 源目录:$LogFolderNew "--->>>" 链接目录:$LogFolderOld
if [ ! -L "$LogFolderOld" ]; 
then
 ln -s $LogFolderNew $LogFolderOld
else
 echo "软链接已经存在,不需要创建"
fi

if [ $? -eq 0 ]
then
    echo 增加 源目录:$LogFolderNew "--->>>" 链接目录:$LogFolderOld 软连接成功
else
    echo 增加 源目录:$LogFolderNew "--->>>" 链接目录:$LogFolderOld 软连接失败
    exit
fi

echo 2.2 源目录:$DBFolderNew1 "--->>>" 链接目录:$DBFolderOld1
if [ ! -L "$DBFolderOld1" ]; 
then
 ln -s $DBFolderNew1 $DBFolderOld1
else
 echo "软链接已经存在,不需要创建"
fi

if [ $? -eq 0 ]
then
    echo 增加 源目录:$DBFolderNew1 "--->>>" 链接目录:$DBFolderOld1 软连接成功
else
    echo 增加 源目录:$DBFolderNew1 "--->>>" 链接目录:$DBFolderOld1 软连接失败
    exit
fi

echo 2.3 源目录:$DBFolderNew2 "--->>>" 链接目录:$DBFolderOld2
if [ ! -L "$DBFolderOld2" ]; 
then
 ln -s $DBFolderNew2 $DBFolderOld2
else
 echo "软链接已经存在,不需要创建"
fi

if [ $? -eq 0 ]
then
    echo 增加 源目录:$DBFolderNew2 "--->>>" 链接目录:$DBFolderOld2 软连接成功
else
    echo 增加 源目录:$DBFolderNew2 "--->>>" 链接目录:$DBFolderOld2 软连接失败
    exit
fi

echo 2.4 源目录:$DBFolderNew3 "--->>>" 链接目录:$DBFolderOld3
if [ ! -L "$DBFolderOld3" ]; 
then
 ln -s $DBFolderNew3 $DBFolderOld3
else
 echo "软链接已经存在,不需要创建"
fi

if [ $? -eq 0 ]
then
    echo 增加 源目录:$DBFolderNew3 "--->>>" 链接目录:$DBFolderOld3 软连接成功
else
    echo 增加 源目录:$DBFolderNew3 "--->>>" 链接目录:$DBFolderOld3 软连接失败
    exit
fi

echo 2.5 源目录:$DBFolderNew4 "--->>>" 链接目录:$DBFolderOld4
if [ ! -L "$DBFolderOld4" ]; 
then
 ln -s $DBFolderNew4 $DBFolderOld4
else
 echo "软链接已经存在,不需要创建"
fi

if [ $? -eq 0 ]
then
    echo 增加 源目录:$DBFolderNew4 "--->>>" 链接目录:$DBFolderOld4 软连接成功
else
    echo 增加 源目录:$DBFolderNew4 "--->>>" 链接目录:$DBFolderOld4 软连接失败
    exit
fi

echo "3.检测新的数据目录是否存在,开始---"

echo 3.1 新分区目录:$LogFolderNew
if [ ! -d "$LogFolderNew" ]; 
then
 mkdir "$LogFolderNew"
else
 echo "新分区目录已经存在,不需要创建"
fi

if [ $? -eq 0 ]
then
    echo 增加 新分区目录:$LogFolderNew 成功
else
    echo 增加 新分区目录:$LogFolderNew 失败
    exit
fi

echo 3.2 新分区目录:$DBFolderNew1
if [ ! -d "$DBFolderNew1" ]; 
then
 mkdir "$DBFolderNew1"
else
 echo "新分区目录已经存在,不需要创建"
fi

if [ $? -eq 0 ]
then
    echo 增加 新分区目录:$DBFolderNew1 成功
else
    echo 增加 新分区目录:$DBFolderNew1 失败
    exit
fi

echo 3.3 新分区目录:$DBFolderNew2
if [ ! -d "$DBFolderNew2" ]; 
then
 mkdir "$DBFolderNew2"
else
 echo "新分区目录已经存在,不需要创建"
fi

if [ $? -eq 0 ]
then
    echo 增加 新分区目录:$DBFolderNew2 成功
else
    echo 增加 新分区目录:$DBFolderNew2 失败
    exit
fi

echo 3.4 新分区目录:$DBFolderNew3
if [ ! -d "$DBFolderNew3" ]; 
then
 mkdir "$DBFolderNew3"
else
 echo "新分区目录已经存在,不需要创建"
fi

if [ $? -eq 0 ]
then
    echo 增加 新分区目录:$DBFolderNew3 成功
else
    echo 增加 新分区目录:$DBFolderNew3 失败
    exit
fi

echo 3.5 新分区目录:$DBFolderNew4
if [ ! -d "$DBFolderNew4" ]; 
then
 mkdir "$DBFolderNew4"
else
 echo "新分区目录已经存在,不需要创建"
fi

if [ $? -eq 0 ]
then
    echo 增加 新分区目录:$DBFolderNew4 成功
else
    echo 增加 新分区目录:$DBFolderNew4 失败
    exit
fi

echo "建立系统软连接:结束---"

echo "*******************************************************************"
echo "#####执行硬盘分区脚本:结束#####"