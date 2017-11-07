#!/bin/sh

while :
do

ps | grep -v 'grep' | grep '/app/EV9000MediaService' > /dev/null

if [ ! $? -eq 0 ]
then
/app/EV9000MediaService > /dev/null &
fi

sleep 1
done
