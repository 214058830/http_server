bin=httpServer
cc=g++ -std=c++11
LDFLAGS=-lpthread

.PHONY:all
all:$(bin) cal

$(bin):httpServer.cc
	$(cc) -g -o $@ $^ $(LDFLAGS)
cal:cal.cc
	$(cc) -o $@ $^
	mv cal wwwroot/

.PHONY:clean
clean:
	rm -f $(bin)
