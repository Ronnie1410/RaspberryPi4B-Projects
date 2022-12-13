#Compiler
CC = gcc

#compiler flags:
#-Wall turn on compiler warnings
CFLAGS  = -Wall

#Option to attach POSIX Thread Library
OPTION	= -lpthread

#the build target executable:
TARGET = assignment1

all: 
	$(CC) $(CFLAGS) $(TARGET).c -o $(TARGET) $(OPTION)

clean:
	$(RM) $(TARGET)
