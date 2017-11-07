#!/bin/sh

echo "Auto add static route info:"
echo "---------------------------"
while read line1
do
    echo $line1
$line1
done < /config/route.cfg
