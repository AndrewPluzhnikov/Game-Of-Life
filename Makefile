CXXFLAGS = -g -I ${HOME}/rapidjson/include
PROGS = gtorusgen gcycle

.PHONY: all clean

all: ${PROGS}
clean:
	rm -f *.o ${PROGS}


