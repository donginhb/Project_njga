#!/bin/sh
#------------------------------------------------------------main--------------------------------------------
cd /data

sqlfile="/data/sql.txt"
rm -rf  $sqlfile

echo "select count(*) from UserConfig;" >>$sqlfile

dbsql /data/EV9000DB < /data/sql.txt

rm -rf  $sqlfile

echo "good"
