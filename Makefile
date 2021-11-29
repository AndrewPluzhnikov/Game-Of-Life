CXXFLAGS = -g -I ${HOME}/rapidjson/include
PROGS = gtorusgen gcycle genstates

.PHONY: all clean

all: ${PROGS}
clean:
	rm -f *.o ${PROGS}


