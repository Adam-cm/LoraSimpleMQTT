# LoRaSimpleMQTT
# Single Channel LoRaWAN Gateway

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin # or . if you want it in the current directory

EXE := $(BIN_DIR)/LoraSimpleMQTT

SRC := $(wildcard $(SRC_DIR)/*.c)

LIBS = -lwiringPi -lpaho-mqttpp3 -lpaho-mqtt3c -lpaho-mqtt3cs -lpaho-mqtt3as -lpaho-mqtt3a

LoraSimpleMQTT: main.o LoRa.o base64.o
	g++ main.o LoRa.o base64.o $(LIBS) -o LoraSimpleMQTT

main.o: main.cpp 
	g++ -c main.cpp
	
LoRa.o: LoRa.cpp LoRa.h
	g++ -c LoRa.cpp
	
base64.o: base64.c base64.h
	g++ -c base64.c
	
clean:
	rm *.o LoraSimpleMQTT