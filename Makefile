FFMPEG=/usr/local/ffmpeg
CC=g++
CFLAGS=-g -I$(FFMPEG)/include
LDFLAGS = -L$(FFMPEG)/lib/ -lswscale -lswresample -lavformat -lavdevice -lavcodec -lavutil -lavfilter -lm
TARGETS=test
all: $(TARGETS)
transcode.o:transcode.h transcode.cpp
	$(CC) -c $^ $(CFLAGS) $(LDFLAGS) -std=c++11
test:Demotest.cpp transcode.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) -std=c++11
all: $(TARGETS)
clean:
	rm -rf $(TARGETS)