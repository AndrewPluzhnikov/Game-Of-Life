CXXFLAGS = -g -Og -I ${HOME}/rapidjson/include
PROGS = gtorusgen gcycle genstates shannon

.PHONY: all clean

all: ${PROGS}
clean:
	rm -f *.o ${PROGS}


