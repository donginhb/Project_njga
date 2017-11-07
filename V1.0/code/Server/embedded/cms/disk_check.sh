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

echo "######################################################################"
echo "检测系统是否存在两个硬盘分区..."
blkid | grep sda1
blkid | grep sda2

if [ ! $? -eq 0 ]
then
 echo "系统只存在一个硬盘分区, 不需要执行数据库以及日志文件分区脚本,退出!!!"
 echo "######################################################################"
else
 echo "系统存在两个硬盘分区, 需要执行数据库以及日志文件分区脚本!!!"
 echo "######################################################################"
  
echo "#####执行数据库以及日志文件分区脚本:开始#####"
echo "---------------------------------------------"

echo "1.创建分区数据库目录,开始---"
echo 老分区目录:$OldPartitionDir
if [ ! -d "$OldPartitionDir" ]; 
then
 mkdir "$OldPartitionDir"
else
 echo "创建老分区数据库目录已经存在,不需要创建"
fi

echo 新分区目录:$NewPartitionDBDir
if [ ! -d "$NewPartitionDBDir" ]; 
then
 mkdir "$NewPartitionDBDir"
else
 echo "创建新分区数据库目录已经存在,不需要创建"
fi

echo "1.创建分区数据库目录,结束---"

echo "---------------------------------------------"

echo "2.移动原有老的数据目录到新分区/data1上面,开始---"
echo 2.1 原目录:$LogFolderOld "--->>>" 目的路径:$NewPartitionDir
if [ ! -L "$LogFolderOld" ]; 
then
	if [ ! -d "$LogFolderOld" ]; 
	then
	 echo "原有目录不存在,不需要移动"
	else
	 mv -f $LogFolderOld $NewPartitionDir
	fi
else
 echo "原有目录为软链接,不需要移动"
fi

echo 2.2 原目录:$DBFolderOld1 "--->>>" 目的路径:$NewPartitionDBDir
if [ ! -L "$DBFolderOld1" ]; 
then
	if [ ! -d "$DBFolderOld1" ]; 
	then
	 echo "原有目录不存在,不需要移动"
	else
	 mv -f $DBFolderOld1 $NewPartitionDBDir
	fi
else
 echo "原有目录为软链接,不需要移动"
fi

echo 2.3 原目录:$DBFolderOld2 "--->>>" 目的路径:$NewPartitionDBDir
if [ ! -L "$DBFolderOld2" ]; 
then
	if [ ! -d "$DBFolderOld2" ]; 
	then
	 echo "原有目录不存在,不需要移动"
	else
	 mv -f $DBFolderOld2 $NewPartitionDBDir
	fi
else
 echo "原有目录为软链接,不需要移动"
fi

echo 2.4 原目录:$DBFolderOld3 "--->>>" 目的路径:$NewPartitionDBDir
if [ ! -L "$DBFolderOld3" ]; 
then
	if [ ! -d "$DBFolderOld3" ]; 
	then
	 echo "原有目录不存在,不需要移动"
	else
	 mv -f $DBFolderOld3 $NewPartitionDBDir
	fi
else
 echo "原有目录为软链接,不需要移动"
fi

echo 2.5 原目录:$DBFolderOld4 "--->>>" 目的路径:$NewPartitionDBDir
if [ ! -L "$DBFolderOld4" ]; 
then
	if [ ! -d "$DBFolderOld4" ]; 
	then
	 echo "原有目录不存在,不需要移动"
	else
	 mv -f $DBFolderOld4 $NewPartitionDBDir
	fi
else
 echo "原有目录为软链接,不需要移动"
fi

echo "2.移动原有老的数据目录到新分区/data1上面,结束---"

echo "---------------------------------------------"

echo "3.删除老的数据目录,开始---"
echo 3.1 原目录:$LogFolderOld
if [ ! -L "$LogFolderOld" ]; 
then
	if [ ! -d "$LogFolderOld" ]; 
	then
	 echo "原有目录不存在,不需要删除"
	else
	 rm -rf $LogFolderOld
	fi
else
 echo "原有目录为软链接,不需要删除"
fi

echo 3.2 原目录:$DBFolderOld1
if [ ! -L "$DBFolderOld1" ]; 
then
	if [ ! -d "$DBFolderOld1" ]; 
	then
	 echo "原有目录不存在,不需要删除"
	else
	 rm -rf $DBFolderOld1
	fi
else
 echo "原有目录为软链接,不需要删除"
fi

echo 3.3 原目录:$DBFolderOld2
if [ ! -L "$DBFolderOld2" ]; 
then
	if [ ! -d "$DBFolderOld2" ]; 
	then
	 echo "原有目录不存在,不需要删除"
	else
	 rm -rf $DBFolderOld2
	fi
else
 echo "原有目录为软链接,不需要删除"
fi

echo 3.4 原目录:$DBFolderOld3
if [ ! -L "$DBFolderOld3" ]; 
then
	if [ ! -d "$DBFolderOld3" ]; 
	then
	 echo "原有目录不存在,不需要删除"
	else
	 rm -rf $DBFolderOld3
	fi
else
 echo "原有目录为软链接,不需要删除"
fi

echo 3.5 原目录:$DBFolderOld4
if [ ! -L "$DBFolderOld4" ]; 
then
	if [ ! -d "$DBFolderOld4" ]; 
	then
	 echo "原有目录不存在,不需要删除"
	else
	 rm -rf $DBFolderOld4
	fi
else
 echo "原有目录为软链接,不需要删除"
fi

echo "3.删除老的数据目录,结束---"

echo "---------------------------------------------"

echo "4.增加软链接，将频繁操作的数据目录链接到新分区/data1上面,开始---"
#ln -s a b 中的 a 就是源文件，b是链接文件名,其作用是当进入b目录，实际上是链接进入了a目录
echo 4.1 源目录:$LogFolderNew "--->>>" 链接目录:$LogFolderOld
if [ ! -L "$LogFolderOld" ]; 
then
 ln -s $LogFolderNew $LogFolderOld
else
 echo "软链接已经存在,不需要创建"
fi

echo 4.2 源目录:$DBFolderNew1 "--->>>" 链接目录:$DBFolderOld1
if [ ! -L "$DBFolderOld1" ]; 
then
 ln -s $DBFolderNew1 $DBFolderOld1
else
 echo "软链接已经存在,不需要创建"
fi

echo 4.3 源目录:$DBFolderNew2 "--->>>" 链接目录:$DBFolderOld2
if [ ! -L "$DBFolderOld2" ]; 
then
 ln -s $DBFolderNew2 $DBFolderOld2
else
 echo "软链接已经存在,不需要创建"
fi

echo 4.4 源目录:$DBFolderNew3 "--->>>" 链接目录:$DBFolderOld3
if [ ! -L "$DBFolderOld3" ]; 
then
 ln -s $DBFolderNew3 $DBFolderOld3
else
 echo "软链接已经存在,不需要创建"
fi

echo 4.5 源目录:$DBFolderNew4 "--->>>" 链接目录:$DBFolderOld4
if [ ! -L "$DBFolderOld4" ]; 
then
 ln -s $DBFolderNew4 $DBFolderOld4
else
 echo "软链接已经存在,不需要创建"
fi

echo "4.增加软链接，将频繁操作的数据目录链接到新分区/data1上面,结束---"

echo "---------------------------------------------"

echo "5.检测新的数据目录是否存在,开始---"

echo 5.1 新分区目录:$LogFolderNew
if [ ! -d "$LogFolderNew" ]; 
then
 mkdir "$LogFolderNew"
else
 echo "新分区目录已经存在,不需要创建"
fi

echo 5.2 新分区目录:$DBFolderNew1
if [ ! -d "$DBFolderNew1" ]; 
then
 mkdir "$DBFolderNew1"
else
 echo "新分区目录已经存在,不需要创建"
fi

echo 5.3 新分区目录:$DBFolderNew2
if [ ! -d "$DBFolderNew2" ]; 
then
 mkdir "$DBFolderNew2"
else
 echo "新分区目录已经存在,不需要创建"
fi

echo 5.4 新分区目录:$DBFolderNew3
if [ ! -d "$DBFolderNew3" ]; 
then
 mkdir "$DBFolderNew3"
else
 echo "新分区目录已经存在,不需要创建"
fi

echo 5.5 新分区目录:$DBFolderNew4
if [ ! -d "$DBFolderNew4" ]; 
then
 mkdir "$DBFolderNew4"
else
 echo "新分区目录已经存在,不需要创建"
fi

echo "5.检测新的数据目录是否存在,结束---"

echo "---------------------------------------------"
echo "#####执行数据库以及日志文件分区脚本:结束#####"

fi