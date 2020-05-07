TARGET = thread_example
LIBS = -lcurl -lb64 -pthread
CC = g++
CFLAGS = -g -Wall

.PHONY: default

default: $(TARGET)
all: default

HEADERS = signomix.hpp

OBJECTS = thread_example.o

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
