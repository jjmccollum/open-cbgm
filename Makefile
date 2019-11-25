all:
#compiler flags
CXXFLAGS = -O3 -Wall -I ./include -std=c++11
#external libraries
#LIBS = 
#object files to link
objects = \
	src/pugixml.o \
	src/local_stemma.o \
	src/variation_unit.o \
	src/apparatus.o \
	src/witness.o \
	src/global_stemma.o \

#header files for all object files
includes_extra = include/pugiconfig.h include/roaring.h include/roaring.hh
includes = $(patsubst src/%.o, include/%.h, ${objects}) ${includes_extra}

#compile each object from its implementation file and all header files:
src/roaring.o : src/roaring.c include/roaring.hh
	g++ ${CXXFLAGS} -c -o $@ $<
${objects} : src/%.o : src/%.cpp ${includes}
	g++ ${CXXFLAGS} -c -o $@ $<

#executable files
programs = \
	compare_witnesses \
	
tests = \
	test/local_stemma_test \
	test/variation_unit_test \
	test/apparatus_test \
	test/witness_test \

#compile all executables from their implementation files, all linked objects, and external libraries:
${programs} : % : src/%.cpp ${objects} src/roaring.o
	g++ ${CXXFLAGS} -o $@ $< ${objects} src/roaring.o
	
${tests} : % : %.cpp ${objects} src/roaring.o
	g++ ${CXXFLAGS} -o $@ $< ${objects} src/roaring.o

all: ${programs} ${tests} ${objects}

clean:
	rm -f ${objects}
	rm -f ${tests}
	rm -f ${programs}
