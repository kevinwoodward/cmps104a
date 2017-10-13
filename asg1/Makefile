GCC        = g++-6 -g -O0 -Wall -Wextra -std=gnu++14
MKDEP      = g++-6 -MM -std=gnu++14
VALGRIND   = valgrind --leak-check=full --show-reachable=yes

MKFILE   = Makefile
DEPFILE  = Makefile.dep
SOURCES  = string_set.cpp cppstrtok.cpp auxlib.cpp
HEADERS  = string_set.h auxlib.h
OBJECTS  = ${SOURCES:.cpp=.o}
EXECBIN  = oc
SRCFILES = ${HEADERS} ${SOURCES} ${MKFILE}
LISTING    = Listing.ps

all : ${EXECBIN}

${EXECBIN} : ${OBJECTS}
	${GCC} -o${EXECBIN} ${OBJECTS}

%.o : %.cpp
	${GCC} -c $<

ci :
	cid + ${SRCFILES}

clean :
	-rm ${OBJECTS} ${DEPFILE}

spotless : clean
	- rm ${EXECBIN} *.str

${DEPFILE} :
	${MKDEP} ${SOURCES} >${DEPFILE}

dep :
	- rm ${DEPFILE}
	${MAKE} --no-print-directory ${DEPFILE}

include ${DEPFILE}

test : ${EXECBIN}
	${EXECBIN} * * * >test.out 2>&1

lis : test
	mkpspdf Listing.ps ${SRCFILES} ${DEPFILE} test.out

again : ${SRCFILES}
	make --no-print-directory spotless dep ci test lis
