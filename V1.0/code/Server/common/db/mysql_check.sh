#!/bin/sh

USER=root
PASSWORD=wiscom
DBHOST=localhost
CHECKLOGFILE=/data/log/mysql_check_EV9000.log
TEMPFILE=/data/log/mysql_temp.log
REPAIRLOGFILE=/data/log/mysql_repair_EV9000.log
CHECK_TYPE1=   #extra params to CHECK_TABLE ,such as  FAST
CHECK_TYPE2=
REPAIR_TYPE1=
CORRUPT=no  # start by assuming no corruption
DBNAMES="all"  #or a list delimited by space
DBEXCLUDE="information_schema mysql performance_schema test"   #or a list delimited by space

#NOTE: the DBEXCLUDE feature seemed to only work with Linux regex, GNU sed
detect_all_dbnames(){
if test $DBNAMES = "all" ; then
  DBNAMES="`mysql --user=$USER --password=$PASSWORD --batch -N -e "show databases"`"
for i in $DBEXCLUDE
do
  DBNAMES=`echo $DBNAMES | sed "s/\b$i\b//g"`  #repace database name for null
done
fi
}

detect_all_dbnames


check_all_tables(){
rm -rf $CHECKLOGFILE
for i in $DBNAMES
do
  echo -e "\033[31;49;1m Database being checked: \033[39;49;0m" >>$CHECKLOGFILE
  #echo "Database being checked:"
  echo -n " SHOW DATABASES LIKE '$i' " | mysql -t -u $USER -p$PASSWORD $i >>$CHECKLOGFILE

  #Check all tables in one pass, instead of a loop
  #Use GAWK to put in comma separators, use SED to remove trailing comma
  #Modified to only check MyISAM or InnoDB tables
  DBTABLES="`mysql --user=$USER --password=$PASSWORD $i --batch -N -e "show table status;" \
  | awk 'BEGIN {ORS=", " } $2 == "MyISAM" || $2 == "InnoDB"{print $1}' | sed 's/, $//'`"
  #echo $BTABLES

  # output in table form using -t option
  if [ ! "$DBTABLES"  ]
  then
    echo "NOTE:  There are no tables to check in the $i database - skipping..." >> $CHECKLOGFILE
  else
    echo "check table $DBTABLES $CHECK_TYPE1 $CHECK_TYPE2" | mysql -t -u $USER -p$PASSWORD $i >> $CHECKLOGFILE
  fi
done
}

check_all_tables

#fatch CORRUPT table
fatch_corrupt_table(){
  cat $CHECKLOGFILE | grep warning  | awk 'BEGIN {FS="|"} {print $2}' >$TEMPFILE
  cat $CHECKLOGFILE | grep status  | awk 'BEGIN {FS="|"} {print $2}' >>$TEMPFILE
}

fatch_corrupt_table



repair_corrupt_table(){
for i in ` cat $TEMPFILE `
do
  DATABASE=` echo $i | awk 'BEGIN {FS="."} {print $1}' `
  TABLE=` echo $i | awk 'BEGIN {FS="."} {print $2}' `
  echo "`date +'%d%m%y %H:%M:%S'` : " `mysql -u ${USER} -p${PASSWORD}  -N -e "use ${DATABASE}; repair table ${TABLE} ${REPAIR_TYPE1}"` >> ${REPAIRLOGFILE}
done
}

repair_corrupt_table

