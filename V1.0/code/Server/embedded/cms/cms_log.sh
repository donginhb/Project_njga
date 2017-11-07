#!/bin/sh
#------------------------------------------------------------main--------------------------------------------
yourdate=`date +%Y_%m_%d_%H_%M_%S`

echo $1 > /data/log/cms_shell_log_$yourdate.log