#!/bin/ash
usage()
{
echo "cmd fomat: ./dbop.sh -o bak/recover/create -p dbpath -f bakfile,exit... "
exit
}

do_dbsql_bak()
{
   dbbakpara="/data/dbbak.para"
   rm -rf  $dbbakpara
   echo ".output $2" >>$dbbakpara
   #添加待备份的表
   echo ".dump AlarmRecord" >>$dbbakpara
   echo ".dump PollActionConfig" >>$dbbakpara
   echo ".dump BoardNetConfig" >>$dbbakpara
   echo ".dump BoardConfig" >>$dbbakpara
   echo ".dump PollConfig" >>$dbbakpara
   echo ".dump CruiseActionConfig" >>$dbbakpara
   echo ".dump PollPermissionConfig" >>$dbbakpara
   echo ".dump CruiseConfig" >>$dbbakpara
   echo ".dump PresetConfig" >>$dbbakpara
   echo ".dump CruisePermissionConfig" >>$dbbakpara
   echo ".dump RecordSchedConfig" >>$dbbakpara
   echo ".dump DeviceStatusRecord" >>$dbbakpara
   echo ".dump RecordTimeSchedConfig" >>$dbbakpara
   #echo ".dump FileRecord" >>$dbbakpara
   echo ".dump RouteNetConfig" >>$dbbakpara
   echo ".dump FileTagRecord" >>$dbbakpara
   #echo ".dump SystemLogRecord" >>$dbbakpara
   echo ".dump GBLogicDeviceConfig" >>$dbbakpara
   echo ".dump UnGBPhyDeviceChannelConfig" >>$dbbakpara
   echo ".dump GBPhyDeviceConfig" >>$dbbakpara
   echo ".dump UnGBPhyDeviceConfig" >>$dbbakpara
   echo ".dump GBPhyDeviceTempConfig" >>$dbbakpara
   echo ".dump UserConfig" >>$dbbakpara
   echo ".dump GroupConfig" >>$dbbakpara
   echo ".dump UserDevicePermConfig" >>$dbbakpara
   echo ".dump LogicDeviceGroupConfig" >>$dbbakpara
   echo ".dump UserGroupConfig" >>$dbbakpara
   echo ".dump LogicDeviceMapGroupConfig" >>$dbbakpara
   echo ".dump UserGroupDevicePermConfig" >>$dbbakpara
   echo ".dump OuterDeviceConfig" >>$dbbakpara
   echo ".dump UserLogRecord" >>$dbbakpara
   echo ".dump PlanActionConfig" >>$dbbakpara
   echo ".dump UserLogicDeviceGroupConfig" >>$dbbakpara
   echo ".dump PlanConfig" >>$dbbakpara
   echo ".dump UserLogicDeviceMapGroupConfig" >>$dbbakpara
   echo ".dump PlanLinkageConfig" >>$dbbakpara
   echo ".dump UserMapGroupConfig" >>$dbbakpara
   echo ".dump PlanPermissionConfig" >>$dbbakpara                           
   echo ".output stdout" >>$dbbakpara
   echo ".quit" >>$dbbakpara
   dbsql $1 <$dbbakpara
   rm -rf  $dbbakpara
}

do_dbsql_recover()
{
   dbsql $1 <$2
}

do_dbsql_create()
{
   echo "create table"
   exit
}

#解析参数
while getopts :o:p:f:h opt; do
    case $opt in
        o) optype=$OPTARG ;;
        p) dbpath=$OPTARG ;;
        f) bakfile=$OPTARG ;;
        h) usage ;;
        \?) usage ;;
    esac
done
 
if [ "$bakfile" == "" ];then
   echo "bakfie is empty, set bakefile =bakfile_ev9000.db"
   bakfile="bakfile_ev9000.db"
fi

case $optype in
     bak)     echo "start bak..."
              do_dbsql_bak $dbpath $bakfile ;; 
     recover) echo "start recover..."
              do_dbsql_recover $dbpath $bakfile ;;
     create)  echo "start create..."
              do_dbsql_create $dbpath ;;
esac
echo "finish"
