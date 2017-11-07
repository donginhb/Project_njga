#!/bin/bash


if [ ! $1 ]
then
	echo "execute as : compile-app.sh V1.0.0"
	exit 1
fi

echo "compile $1 for mfpa-cms"

PJT_DIR=$PWD
APP_DIR=$PJT_DIR
APP_NAME=cn6130_cms_app
BIN_DIR=bin

cd $PJT_DIR/../octeon
source ./set-octeon-env.sh
source ./set-mfpa-env.sh cms

# ================ APP
cd $APP_DIR &&
make clean &&
make &&
make install &&
cd $APP_DIR/$BIN_DIR &&
mips64-octeon-linux-gnu-strip *
chmod +x $APP_DIR/$BIN_DIR/start_app &&
chmod +x $APP_DIR/$BIN_DIR/*.sh &&

# ================ MAKE VERSION
cd $APP_DIR &&
find $APP_DIR/$BIN_DIR -name '.svn' | xargs rm -rf
mkcramfs -r $APP_DIR/$BIN_DIR $APP_NAME &&
mkver_ev9000 -v $1 -a $APP_NAME
