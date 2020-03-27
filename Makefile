DIR	= $(shell pwd)
MINISAT	= $(DIR)/custom_minisat
GLUCOSE	= $(DIR)/glucose
MCSMUS	= $(DIR)/mcsmus
CADICAL = /home/xbendik/bin/cadical/src
MSAT	= libr

LIBD 	= -L/usr/lib -L/usr/local/lib -L/home/xbendik/bin/cadical/build
LIBS 	= -lz -lcadical
LIBS	+= -lstdc++fs
USR 	= /usr/include
INC 	= -I $(MCSMUS) -I $(MINISAT) -I $(GLUCOSE) -I $(USR) -I /usr/local/include -I $(DIR) -I $(MCSMUS) -I $(CADICAL)

CSRCS	= $(wildcard *.cpp) $(wildcard $(DIR)/algorithms/*.cpp)
CSRCS	+= $(wildcard $(DIR)/satSolvers/*.cpp) $(wildcard $(DIR)/core/*.cpp)
COBJS	= $(CSRCS:.cpp=.o)

MCSRCS	= $(wildcard $(MINISAT)/*.cc)
MCOBJS	= $(MCSRCS:.cc=.o)

GLSRCS	= $(wildcard $(GLUCOSE)/*.cc)
GLOBJS	= $(GLSRCS:.cc=.o)

MCSMUS_SRCS = $(wildcard $(MCSMUS)/minisat/core/*.cc) $(wildcard $(MCSMUS)/minisat/simp/*.cc) $(wildcard $(MCSMUS)/minisat/utils/*.cc) \
		$(wildcard $(MCSMUS)/glucose/core/*.cc) $(wildcard $(MCSMUS)/glucose/simp/*.cc) $(wildcard $(MCSMUS)/glucose/utils/*.cc) \
		$(wildcard $(MCSMUS)/mcsmus/*.cc)
MCSMUS_OBJS = $(filter-out %Main.o, $(MCSMUS_SRCS:.cc=.o))

USEMCSMUS = YES

CXX	= g++
CFLAGS 	= -w -std=c++17 -g
CFLAGS	+= -O3
CFLAGS	+= -D NDEBUG

ifeq ($(USEMCSMUS),YES)
	CFLAGS += -D UMCSMUS
else
	MCSMUS_OBJS = 
endif

unimus: $(COBJS) $(MCOBJS) $(MCSMUS_OBJS) $(GLOBJS)
	@echo Linking: $@
	$(CXX) -o $@ $(COBJS) $(MCOBJS) $(MCSMUS_OBJS) $(GLOBJS) $(CFLAGS) $(INC) $(LIBD) $(LIBS) 

%.o: %.cpp
	@echo Compiling: $@
	@$(CXX) $(CFLAGS) $(INC) -c -o $@ $<

%.o: %.cc
	@echo Compiling: $@
	@$(CXX) $(CFLAGS) $(INC) -c -o $@ $<

print-%  : ; @echo $* = $($*)

clean:
	rm -f $(MCSMUS_OBJS)
	rm -f $(COBJS)
	rm -f $(MINISAT)/*.o
	rm -f $(GLOBJS)

cleanCore:
	rm -f $(COBJS)

cleanMcsmus:
	rm -f $(MCSMUS_OBJS)
