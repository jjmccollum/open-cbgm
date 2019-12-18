all:
#compiler flags
CXXFLAGS = -O3 -Wall -I ./include -std=c++11
#external libraries
libs = -lstdc++fs
#object files to link
objects = \
	src/roaring.o \
	src/pugixml.o \
	src/local_stemma.o \
	src/variation_unit.o \
	src/apparatus.o \
	src/set_cover_solver.o \
	src/witness.o \
	src/global_stemma.o \

#header files for all object files
includes_extra = include/pugiconfig.h include/roaring.h include/roaring.hh
includes = $(patsubst src/%.o, include/%.h, ${objects}) ${includes_extra}

#compile each object from its implementation file and all header files:
src/roaring.o : src/roaring.c include/roaring.h include/roaring.hh
	g++ ${CXXFLAGS} -c -o $@ $<
src/pugixml.o : src/pugixml.cpp include/pugixml.h include/pugiconfig.h
	g++ ${CXXFLAGS} -c -o $@ $<
src/set_cover_solver.o : src/set_cover_solver.cpp include/set_cover_solver.h include/roaring.hh
	g++ ${CXXFLAGS} -c -o $@ $<
src/local_stemma.o : src/local_stemma.cpp include/local_stemma.h
	g++ ${CXXFLAGS} -c -o $@ $<
src/variation_unit.o : src/variation_unit.cpp include/variation_unit.h include/witness.h include/local_stemma.h
	g++ ${CXXFLAGS} -c -o $@ $<
src/apparatus.o : src/apparatus.cpp include/apparatus.h include/variation_unit.h
	g++ ${CXXFLAGS} -c -o $@ $<
src/witness.o : src/witness.cpp include/witness.h include/set_cover_solver.h include/apparatus.h
	g++ ${CXXFLAGS} -c -o $@ $<
src/global_stemma.o : src/global_stemma.cpp include/global_stemma.h include/witness.h
	g++ ${CXXFLAGS} -c -o $@ $<
#${objects} : src/%.o : src/%.cpp ${includes}
#	g++ ${CXXFLAGS} -c -o $@ $<

#executable files
programs = \
	compare_witnesses \
	find_relatives \
	optimize_substemmata \
	print_graphs \
	
tests = \
	test/local_stemma_test \
	test/variation_unit_test \
	test/apparatus_test \
	test/witness_test \

#generated folders
generated_dirs = \
	local \
	flow \
	attestations \
	variants \
	global \

#compile all executables from their implementation files, all linked objects, and external libraries:
${programs} : % : src/%.cpp ${objects}
	g++ ${CXXFLAGS} -o $@ $< ${objects} ${libs}
	
${tests} : % : %.cpp ${objects}
	g++ ${CXXFLAGS} -o $@ $< ${objects} ${libs}

all: ${programs} ${tests} ${objects}

clean:
	rm -f ${objects}
	rm -f ${tests}
	rm -f ${programs}
	rm -r -f ${generated_dirs}