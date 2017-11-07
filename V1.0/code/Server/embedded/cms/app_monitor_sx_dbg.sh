#!/bin/sh

export LD_LIBRARY_PATH=/app:$LD_LIBRARY_PATH
while :
do

yourdate=`date +%Y_%m_%d_%H_%M_%S`

ps | grep -v 'grep' | grep '/data/bin/cms_dbg' > /dev/null

if [ ! $? -eq 0 ]
then
find /data/log -mtime +3 -type f -name 'cms_*.*' | xargs rm -rf
ulimit -c unlimited
echo "1" > /proc/sys/kernel/core_uses_pid
echo "/data/log/core-%e-%p-%t" > /proc/sys/kernel/core_pattern
/data/bin/cms_dbg > /data/log/cms_dbg_print_$yourdate.log &
fi

ps | grep -v 'grep' | grep '/app/EV9000MediaService' > /dev/null

if [ ! $? -eq 0 ]
then
/app/EV9000MediaService > /dev/null &
fi

ps | grep -v 'grep' | grep '/app/KeyBoardCtrl' > /dev/null

if [ ! $? -eq 0 ]
then
/app/KeyBoardCtrl > /dev/null &
fi

ps | grep -v 'grep' | grep '/app/rinetd' > /dev/null

if [ ! $? -eq 0 ]
then
/app/rinetd > /dev/null &
fi

ps | grep -v 'grep' | grep '/app/Hc2SysTime.sh' > /dev/null

if [ ! $? -eq 0 ]
then
/app/Hc2SysTime.sh > /dev/null &
fi

ps | grep -v 'grep' | grep 'ntpd' > /dev/null

if [ ! $? -eq 0 ]
then
/config/ntpdate.sh
ntpd -c /config/ntp.conf
hwclock -wu
fi

sleep 1
done
