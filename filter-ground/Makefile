SYS			:=	$(shell gcc -dumpmachine)
CC			=	g++
#CC			=	clang++-10

TARGET		=	libfilter-ground.a

SOURCES		=	\
	extra/MariaConnectorTls.cpp \
	extra/MThread.cpp \
	extra/aho_corasick.cpp \
	AppConf.cpp \
	FilterWorker.cpp \
	NatsPublisher.cpp \

INSTALL_DIR	=	./

# 소스에서 참조되는 헤더파일의 위치
# 절대경로는 사용 X, 상대경로로 사용토록 한다. 

######################################## include
INCLUDE	+=	-I./
INCLUDE	+=	-I./thirdparty 
INCLUDE	+=	-I/usr/include/oracle/19.19/client64

NATS_CHECK := $(shell find /usr/local/lib64/libnats_static.a 2>/dev/null | wc -l)
ifeq ($(NATS_CHECK),0)
INCLUDE	+= -I../filter-ground/thirdparty/nats.c
LDFLAGS	+= -L../filter-ground/thirdparty/nats.c/build/lib
endif

OCCI_CHECK := $(shell ldconfig -p | grep -i libocci.so | wc -l)
ifneq ($(OCCI_CHECK),0)
SOURCES += extra/OtlConnectorTls.cpp
LDFLAGS += -L/usr/lib/oracle/19.19/client64/lib/ -lclntsh -locci
endif


######################################## custom unixODBC
#INCLUDE += -I/usr/local/unixODBC/include

# 용량을 줄여주는 옵션(사용하지 않는 오브젝트 제거...)
#CPPFLAGS	+=	-ffunction-sections -fdata-sections -s

# 용량을 줄이기위한 옵션
#ifneq (, $(findstring linux, $(SYS)))
#	ifneq (, $(findstring g++, $(CC)))
#		CPPFLAGS	+= -Wl,--gc-sections -s
#	endif
#endif

# system. fPIE is clang only
#CPPFLAGS += -DBUILD64 -fPIE
#CPPFLAGS += -DBUILD64

# OTL Library 옵션
#CPPFLAGS += -DOTL_ODBC_UNIX -DOTL_STL -DOTL_ADD_NULL_TERMINATOR_TO_STRING_SIZE

# 디버깅정보를 포함시킨다.
CPPFLAGS += -g -D_REENTRANT
#CPPFLAGS += -D_REENTRANT

#	-O2							최적화
#	-std=c++03			c++03 문법 기준으로 검사 및 컴파일
#	-std=c++11			c++11 문법 기준으로 검사 및 컴파일
#	-std=c++17			c++17 문법 기준으로 검사 및 컴파일
#	-Wall -Wextra		모든 경고 킴
#	-Wfloat-equal		C,C++ 소수점 비교연산시 경고 출력(C,C++ 소수점 연산은 정확하지 않음)
#	-m64						64비트로 컴파일
#	-m32						32비트로 컴파일,
#									32비트 컴파일시에는 32비트용 라이브러리가 시스템 내에 설치되어 있어야 한다.
CPPFLAGS	+=	-O2 -std=c++11 -Wall -Wextra -Wfloat-equal -fPIC -m64

OBJECTS	:=	$(SOURCES:.cpp=.o)
DEPENDS	:=	$(SOURCES:.cpp=.d)

all: $(DEPENDS) $(OBJECTS)
	rm -rf core.*
	ar rcv $(TARGET) $(OBJECTS)
#	$(CC) -o $(TARGET) $(OBJECTS) $(CPPFLAGS) $(LDFLAGS)

clean:
	rm -rf $(TARGET) $(OBJECTS)

install: all
#	cp $(TARGET) $(INSTALL_DIR)

.c.o: $(.cpp.o)
.cpp.o:
	$(CC) $(INCLUDE) $(CPPFLAGS) -c $< -o $@
	
%d:%cpp
	$(CC) $(INCLUDE) $(CPPFLAGS) -MM -MP -MT "$(@:.d=.o) $@" -MF $@ $<

.PHONY: all

-include $(DEPENDS)


