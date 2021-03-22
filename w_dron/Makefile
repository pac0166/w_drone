CC = g++
CFAGS = -W -Wall -g -lpthread -pthread
TARGET = test
OBJECTS = pm_i2c.o i2c.o i2c_wrapper.o serial_port.o ina219.o ads1115.o tcp_port.o

all : $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFAGS) -Wall -o $@ $^

clean :
	rm *.o test
