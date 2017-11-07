#!/bin/sh

export LD_LIBRARY_PATH=/app:$LD_LIBRARY_PATH
while :
do

ps | grep -v 'grep' | grep '/app/cms' > /dev/null

if [ ! $? -eq 0 ]
then
find /data/log -mtime +3 -type f -name 'cms_*.*' | xargs rm -rf
/app/cms > /dev/null &
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

ps | grep -v 'grep' | grep 'ntpd' > /dev/null

if [ ! $? -eq 0 ]
then
/config/ntpdate.sh
ntpd -c /config/ntp.conf
hwclock -wu
fi

sleep 1
done
