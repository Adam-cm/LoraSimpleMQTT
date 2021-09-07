# LoRaSimpleMQTT
# Single Channel LoRaWAN Gateway

LIBS = -lwiringPi -lpaho-mqttpp3 -lpaho-mqtt3c -lpaho-mqtt3cs -lpaho-mqtt3as -lpaho-mqtt3a

TARGET_EXEC ?= a.out

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP

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

install:
	sudo cp -f ./LoraSimpleMQTT.service /lib/systemd/system/
	sudo systemctl enable LoraSimpleMQTT.service
	sudo systemctl daemon-reload
	sudo systemctl start LoraSimpleMQTT
	sudo systemctl status LoraSimpleMQTT -l
	
uninstall:
	sudo systemctl stop LoraSimpleMQTT
	sudo systemctl disable LoraSimpleMQTT.service
	sudo rm -f /lib/systemd/system/LoraSimpleMQTT.service 