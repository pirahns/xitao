CC = gcc 
CXX = g++ 

#DEBUG = 1
EXTRAEINC=-I/c3se/users/pirah/Hebbe/Documents/dat400lab3/extrae/include/
EXTRAELIBS=-L/c3se/users/pirah/Hebbe/Documents/dat400lab3/extrae/lib -lpttrace
#include makefile.inc
include makefile.sched
EXAMPLES += benchmarks
CFLAGS += -std=gnu11 ${EXTRAEINC}
CXXFLAGS += --std=c++11 -fPIC ${EXTRAEINC}

ifeq ($(DEBUG),1)
  CFLAGS += -g3 -O0
  CXXFLAGS += -g3 -O0
else
  CFLAGS += -O3
  CXXFLAGS += -O3
endif

INCS = include/
LFLAGS :=

LOIFLAGS := -DMERGEBLOCK=4096

%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LOIFLAGS) -c $< -o $@ $(EXTRAELIBS)

%.o : %.cxx
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LOIFLAGS) -c $< -o $@ $(EXTRAELIBS)


CXXFLAGS += -I$(INCS)
XITAO_LIB = ./lib

LIBS = -pthread -lm -L$(XITAO_LIB) -lxitao ${EXTRAELIBS}

TAO_OBJS = src/tao_sched.o  src/poly_task.o src/xitao_workspace.o src/barriers.o 

all: lib
	cd $(EXAMPLES)/dotproduct && $(MAKE) clean && $(MAKE) 
	cd $(EXAMPLES)/randomDAGs && $(MAKE) clean && $(MAKE)
	cd $(EXAMPLES)/syntheticDAGs && $(MAKE) clean && $(MAKE) 
	cd $(EXAMPLES)/heat && $(MAKE) clean && $(MAKE) 
	cd $(EXAMPLES)/dataparallel && $(MAKE) clean && $(MAKE) 

%.o : %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(EXTRAELIBS)

lib: clean libxitao.a

libxitao.a: $(SORTLIB_OBJS) $(TAO_OBJS)
	@mkdir -p $(XITAO_LIB)
	ar $(ARFLAGS) $(XITAO_LIB)/$@ $^

clean:
	rm -f randombench $(TAO_OBJS) $(XITAO_LIB)/libxitao.a graph.txt
