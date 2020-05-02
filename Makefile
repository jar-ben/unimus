DIR	= $(shell pwd)
MINISAT	= $(DIR)/custom_minisat
MSAT	= libr

LIBD 	= -L/usr/lib -L/usr/local/lib 
LIBS 	= -lz
LIBS	+= -lstdc++fs
USR 	= /usr/include
INC 	= -I $(MINISAT) -I $(USR) -I /usr/local/include -I $(DIR)

CSRCS	= $(wildcard *.cpp) $(wildcard $(DIR)/algorithms/*.cpp)
CSRCS	+= $(wildcard $(DIR)/satSolvers/*.cpp) $(wildcard $(DIR)/core/*.cpp)
COBJS	= $(CSRCS:.cpp=.o)

MCSRCS	= $(wildcard $(MINISAT)/*.cc)
MCOBJS	= $(MCSRCS:.cc=.o)

CXX	= g++
CFLAGS 	= -w -std=c++17 -g
CFLAGS	+= -O3
CFLAGS	+= -D NDEBUG


unimus: $(COBJS) $(MCOBJS) 
	@echo Linking: $@
	$(CXX) -o $@ $(COBJS) $(MCOBJS) $(CFLAGS) $(INC) $(LIBD) $(LIBS) 

%.o: %.cpp
	@echo Compiling: $@
	@$(CXX) $(CFLAGS) $(INC) -c -o $@ $<

%.o: %.cc
	@echo Compiling: $@
	@$(CXX) $(CFLAGS) $(INC) -c -o $@ $<

print-%  : ; @echo $* = $($*)

clean:
	rm -f $(COBJS)
	rm -f $(MINISAT)/*.o

cleanCore:
	rm -f $(COBJS)
