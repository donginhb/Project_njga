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

echo "######################################################################"
echo "���ϵͳ�Ƿ��������Ӳ�̷���..."
blkid | grep sda1
blkid | grep sda2

if [ ! $? -eq 0 ]
then
 echo "ϵͳֻ����һ��Ӳ�̷���, ����Ҫִ�����ݿ��Լ���־�ļ������ű�,�˳�!!!"
 echo "######################################################################"
else
 echo "ϵͳ��������Ӳ�̷���, ��Ҫִ�����ݿ��Լ���־�ļ������ű�!!!"
 echo "######################################################################"
  
echo "#####ִ�����ݿ��Լ���־�ļ������ű�:��ʼ#####"
echo "---------------------------------------------"

echo "1.�����������ݿ�Ŀ¼,��ʼ---"
echo �Ϸ���Ŀ¼:$OldPartitionDir
if [ ! -d "$OldPartitionDir" ]; 
then
 mkdir "$OldPartitionDir"
else
 echo "�����Ϸ������ݿ�Ŀ¼�Ѿ�����,����Ҫ����"
fi

echo �·���Ŀ¼:$NewPartitionDBDir
if [ ! -d "$NewPartitionDBDir" ]; 
then
 mkdir "$NewPartitionDBDir"
else
 echo "�����·������ݿ�Ŀ¼�Ѿ�����,����Ҫ����"
fi

echo "1.�����������ݿ�Ŀ¼,����---"

echo "---------------------------------------------"

echo "2.�ƶ�ԭ���ϵ�����Ŀ¼���·���/data1����,��ʼ---"
echo 2.1 ԭĿ¼:$LogFolderOld "--->>>" Ŀ��·��:$NewPartitionDir
if [ ! -L "$LogFolderOld" ]; 
then
	if [ ! -d "$LogFolderOld" ]; 
	then
	 echo "ԭ��Ŀ¼������,����Ҫ�ƶ�"
	else
	 mv -f $LogFolderOld $NewPartitionDir
	fi
else
 echo "ԭ��Ŀ¼Ϊ������,����Ҫ�ƶ�"
fi

echo 2.2 ԭĿ¼:$DBFolderOld1 "--->>>" Ŀ��·��:$NewPartitionDBDir
if [ ! -L "$DBFolderOld1" ]; 
then
	if [ ! -d "$DBFolderOld1" ]; 
	then
	 echo "ԭ��Ŀ¼������,����Ҫ�ƶ�"
	else
	 mv -f $DBFolderOld1 $NewPartitionDBDir
	fi
else
 echo "ԭ��Ŀ¼Ϊ������,����Ҫ�ƶ�"
fi

echo 2.3 ԭĿ¼:$DBFolderOld2 "--->>>" Ŀ��·��:$NewPartitionDBDir
if [ ! -L "$DBFolderOld2" ]; 
then
	if [ ! -d "$DBFolderOld2" ]; 
	then
	 echo "ԭ��Ŀ¼������,����Ҫ�ƶ�"
	else
	 mv -f $DBFolderOld2 $NewPartitionDBDir
	fi
else
 echo "ԭ��Ŀ¼Ϊ������,����Ҫ�ƶ�"
fi

echo 2.4 ԭĿ¼:$DBFolderOld3 "--->>>" Ŀ��·��:$NewPartitionDBDir
if [ ! -L "$DBFolderOld3" ]; 
then
	if [ ! -d "$DBFolderOld3" ]; 
	then
	 echo "ԭ��Ŀ¼������,����Ҫ�ƶ�"
	else
	 mv -f $DBFolderOld3 $NewPartitionDBDir
	fi
else
 echo "ԭ��Ŀ¼Ϊ������,����Ҫ�ƶ�"
fi

echo 2.5 ԭĿ¼:$DBFolderOld4 "--->>>" Ŀ��·��:$NewPartitionDBDir
if [ ! -L "$DBFolderOld4" ]; 
then
	if [ ! -d "$DBFolderOld4" ]; 
	then
	 echo "ԭ��Ŀ¼������,����Ҫ�ƶ�"
	else
	 mv -f $DBFolderOld4 $NewPartitionDBDir
	fi
else
 echo "ԭ��Ŀ¼Ϊ������,����Ҫ�ƶ�"
fi

echo "2.�ƶ�ԭ���ϵ�����Ŀ¼���·���/data1����,����---"

echo "---------------------------------------------"

echo "3.ɾ���ϵ�����Ŀ¼,��ʼ---"
echo 3.1 ԭĿ¼:$LogFolderOld
if [ ! -L "$LogFolderOld" ]; 
then
	if [ ! -d "$LogFolderOld" ]; 
	then
	 echo "ԭ��Ŀ¼������,����Ҫɾ��"
	else
	 rm -rf $LogFolderOld
	fi
else
 echo "ԭ��Ŀ¼Ϊ������,����Ҫɾ��"
fi

echo 3.2 ԭĿ¼:$DBFolderOld1
if [ ! -L "$DBFolderOld1" ]; 
then
	if [ ! -d "$DBFolderOld1" ]; 
	then
	 echo "ԭ��Ŀ¼������,����Ҫɾ��"
	else
	 rm -rf $DBFolderOld1
	fi
else
 echo "ԭ��Ŀ¼Ϊ������,����Ҫɾ��"
fi

echo 3.3 ԭĿ¼:$DBFolderOld2
if [ ! -L "$DBFolderOld2" ]; 
then
	if [ ! -d "$DBFolderOld2" ]; 
	then
	 echo "ԭ��Ŀ¼������,����Ҫɾ��"
	else
	 rm -rf $DBFolderOld2
	fi
else
 echo "ԭ��Ŀ¼Ϊ������,����Ҫɾ��"
fi

echo 3.4 ԭĿ¼:$DBFolderOld3
if [ ! -L "$DBFolderOld3" ]; 
then
	if [ ! -d "$DBFolderOld3" ]; 
	then
	 echo "ԭ��Ŀ¼������,����Ҫɾ��"
	else
	 rm -rf $DBFolderOld3
	fi
else
 echo "ԭ��Ŀ¼Ϊ������,����Ҫɾ��"
fi

echo 3.5 ԭĿ¼:$DBFolderOld4
if [ ! -L "$DBFolderOld4" ]; 
then
	if [ ! -d "$DBFolderOld4" ]; 
	then
	 echo "ԭ��Ŀ¼������,����Ҫɾ��"
	else
	 rm -rf $DBFolderOld4
	fi
else
 echo "ԭ��Ŀ¼Ϊ������,����Ҫɾ��"
fi

echo "3.ɾ���ϵ�����Ŀ¼,����---"

echo "---------------------------------------------"

echo "4.���������ӣ���Ƶ������������Ŀ¼���ӵ��·���/data1����,��ʼ---"
#ln -s a b �е� a ����Դ�ļ���b�������ļ���,�������ǵ�����bĿ¼��ʵ���������ӽ�����aĿ¼
echo 4.1 ԴĿ¼:$LogFolderNew "--->>>" ����Ŀ¼:$LogFolderOld
if [ ! -L "$LogFolderOld" ]; 
then
 ln -s $LogFolderNew $LogFolderOld
else
 echo "�������Ѿ�����,����Ҫ����"
fi

echo 4.2 ԴĿ¼:$DBFolderNew1 "--->>>" ����Ŀ¼:$DBFolderOld1
if [ ! -L "$DBFolderOld1" ]; 
then
 ln -s $DBFolderNew1 $DBFolderOld1
else
 echo "�������Ѿ�����,����Ҫ����"
fi

echo 4.3 ԴĿ¼:$DBFolderNew2 "--->>>" ����Ŀ¼:$DBFolderOld2
if [ ! -L "$DBFolderOld2" ]; 
then
 ln -s $DBFolderNew2 $DBFolderOld2
else
 echo "�������Ѿ�����,����Ҫ����"
fi

echo 4.4 ԴĿ¼:$DBFolderNew3 "--->>>" ����Ŀ¼:$DBFolderOld3
if [ ! -L "$DBFolderOld3" ]; 
then
 ln -s $DBFolderNew3 $DBFolderOld3
else
 echo "�������Ѿ�����,����Ҫ����"
fi

echo 4.5 ԴĿ¼:$DBFolderNew4 "--->>>" ����Ŀ¼:$DBFolderOld4
if [ ! -L "$DBFolderOld4" ]; 
then
 ln -s $DBFolderNew4 $DBFolderOld4
else
 echo "�������Ѿ�����,����Ҫ����"
fi

echo "4.���������ӣ���Ƶ������������Ŀ¼���ӵ��·���/data1����,����---"

echo "---------------------------------------------"

echo "5.����µ�����Ŀ¼�Ƿ����,��ʼ---"

echo 5.1 �·���Ŀ¼:$LogFolderNew
if [ ! -d "$LogFolderNew" ]; 
then
 mkdir "$LogFolderNew"
else
 echo "�·���Ŀ¼�Ѿ�����,����Ҫ����"
fi

echo 5.2 �·���Ŀ¼:$DBFolderNew1
if [ ! -d "$DBFolderNew1" ]; 
then
 mkdir "$DBFolderNew1"
else
 echo "�·���Ŀ¼�Ѿ�����,����Ҫ����"
fi

echo 5.3 �·���Ŀ¼:$DBFolderNew2
if [ ! -d "$DBFolderNew2" ]; 
then
 mkdir "$DBFolderNew2"
else
 echo "�·���Ŀ¼�Ѿ�����,����Ҫ����"
fi

echo 5.4 �·���Ŀ¼:$DBFolderNew3
if [ ! -d "$DBFolderNew3" ]; 
then
 mkdir "$DBFolderNew3"
else
 echo "�·���Ŀ¼�Ѿ�����,����Ҫ����"
fi

echo 5.5 �·���Ŀ¼:$DBFolderNew4
if [ ! -d "$DBFolderNew4" ]; 
then
 mkdir "$DBFolderNew4"
else
 echo "�·���Ŀ¼�Ѿ�����,����Ҫ����"
fi

echo "5.����µ�����Ŀ¼�Ƿ����,����---"

echo "---------------------------------------------"
echo "#####ִ�����ݿ��Լ���־�ļ������ű�:����#####"

fi