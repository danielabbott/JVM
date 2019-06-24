CC=gcc
CFLAGS=-c -m32 -std=gnu99
LDFLAGS=-m32
SOURCES=jvm.c classloader.c encoder.c runtime.c native.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=java

all: $(SOURCES) $(EXECUTABLE) Test.class

Test.class: Test.java
	javac Test.java

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.machinecode
	rm *.bytecode
	rm *.constants.txt
	rm ./java
	rm *.o

