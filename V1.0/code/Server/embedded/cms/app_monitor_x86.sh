#!/bin/sh

export LD_LIBRARY_PATH=/app:$LD_LIBRARY_PATH
while :
do

ps -ef | grep -v 'grep' | grep '/app/cms' > /dev/null

if [ ! $? -eq 0 ]
then
find /data/log -mtime +3 -type f -name 'cms_*.*' | xargs rm -rf
find /data/log -mtime +3 -type f -name 'core_*.*' | xargs rm -rf
ulimit -c unlimited
echo "1" > /proc/sys/kernel/core_uses_pid
echo "/data/log/core-%e-%p-%t" > /proc/sys/kernel/core_pattern
/app/cms > /dev/null &
fi

sleep 1
done
