TARGET = signomixcpp
LIBS = -lcurl -lb64
CC = g++
CFLAGS = -g -Wall

.PHONY: default

default: $(TARGET)
all: default

HEADERS = signomix.hpp

OBJECTS = http_example.o

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)