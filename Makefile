CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 $(shell pkg-config --cflags opencv)

CPP_FILES := $(wildcard *.cpp)
CPP_FILES += $(wildcard */*.cpp)

OBJ_FILES := $(CPP_FILES:.cpp=.o)

LIBS = $(shell pkg-config --libs opencv)

TARGET =	First3574VisionSensor

$(TARGET):	$(OBJ_FILES)
	$(CXX) -o $(TARGET) $(OBJ_FILES) $(LIBS)


echo:
	echo $(CPP_FILES)

all:	$(TARGET)

clean:
	rm -f $(OBJ_FILES) $(TARGET)
