RM = -rm -rf

DB_SRCS = DBOper.cpp DBOper.h cppmysql.cpp cppmysql.h DBOperMysql.cpp DBOperMysql.h DBOperBase.h

all: $(DB_SRCS)

$(DB_SRCS):
		cp -rf ../../../common/db/DBOper.h .
		cp -rf ../../../common/db/DBOper.cpp .		
		cp -rf ../../../common/db/cppmysql.h .
		cp -rf ../../../common/db/cppmysql.cpp .	
		cp -rf ../../../common/db/DBOperBase.h .	
		cp -rf ../../../common/db/DBOperMysql.h .
		cp -rf ../../../common/db/DBOperMysql.cpp .	

.PHONY: clean
clean:
	$(RM) $(DB_SRCS) *.bak *~
