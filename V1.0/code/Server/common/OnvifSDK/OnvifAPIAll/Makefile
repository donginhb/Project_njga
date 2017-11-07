#编译的base路径
BASE_DIR=.

#通过soapcpp2生成文件目录
SOURCE1=$(BASE_DIR)/../src

SOURCE_CODE=../../..

#放stdsoap2.cpp路径
GSOAP_DIR=$(SOURCE_CODE)/ThirdTools/gsoap/gsoap


#源代码路径 源代码只编译一次 对代码进行分批编译
SRC1= $(SOURCE1)/stdsoap2.cpp  $(SOURCE1)/soapC.cpp $(SOURCE1)/soapClient.cpp $(SOURCE1)/test_onvif.cpp $(SOURCE1)/wsaapi.cpp $(SOURCE1)/wsseapi.cpp $(SOURCE1)/threads.cpp $(SOURCE1)/duration.cpp  $(SOURCE1)/smdevp.cpp  $(SOURCE1)/dom.cpp

#目标.o文件生成规则
OBJECTS1 = $(patsubst %.cpp,%.o,$(SRC1))

#生成目录LIB文件路径
LIB_DIR=$(BASE_DIR)/lib

#目标库名
TARGET1=testSoap

#目标头文件
INCLUDE1=-I$(SOURCE1)/onvif

#公共头文件
INCLUDE +=-I$(SOURCE_CODE)/ThirdTools -I$(GSOAP_DIR) -I$(GSOAP_DIR)/src  -I$(GSOAP_DIR)/import  -I$(BASE_DIR) 

LIB= -L../../release/lib -L/nvrs/lib -lssl -lcrypto

CC = g++
CPPFLAG = -Wall -Werror -g -w -fPIC -DWITH_NONAMESPACES -fno-use-cxa-atexit -fexceptions -DWITH_DOM  -DWITH_OPENSSL -DSOAP_DEBUG  

all: $(TARGET1) 
  
$(TARGET1):$(OBJECTS1) 
	$(CC) -o $(TARGET1) $(CPPFLAG) $(OBJECTS1)  $(INCLUDE)  $(LIB)

$(OBJECTS1):%.o : %.cpp
	$(CC) -c $(CPPFLAG) $(INCLUDE) $< -o $@
  
clean:
	rm -rf  $(OBJECTS1) $(TARGET1)

