CPP = g++
DEBUG = -g

CFLAGS = -std=c++11
CFLAGS += `pkg-config --cflags opencv`
LIBS = -lopencv_core -lopencv_highgui -lopencv_imgproc

SRCS = trafficLight.cpp demo.cpp
OBJS = trafficLight.o demo.o
TARGET = trafficLightDetect

all: $(OBJS)
	$(CPP) $^ $(LIBS) -o $(TARGET)

.cpp.o:
	$(CPP) $< $(CFLAGS) -c -o $@

libtrafficLight.so:
	$(CPP) -shared -fPIC $(CFLAGS) $(LIBS) -o libtrafficLight.so trafficLight.cpp

libtrafficLight32.so:
	$(CPP) -shared -fPIC -m32 $(CFLAGS) $(LIBS) -o libtrafficLight.so trafficLight.cpp

clean:
	rm -f *.a $(OBJS) $(TARGET)
