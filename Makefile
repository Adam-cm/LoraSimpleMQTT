# LoRaSimpleMQTT
# Single Channel LoRaWAN Gateway

CC=g++
CFLAGS=-c -Wall
LIBS=-lwiringPi

all: LoraSimpleMQTT

LoraSimpleMQTT: base64.o main.o
	$(CC) main.o base64.o $(LIBS) -o LoraSimpleMQTT

main.o: main.cpp
	$(CC) $(CFLAGS) main.cpp

base64.o: base64.c
	$(CC) $(CFLAGS) base64.c

clean:
	rm *.o LoraSimpleMQTT	