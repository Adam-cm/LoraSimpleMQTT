# LoRaSimpleMQTT
# Single Channel LoRaWAN Gateway

LIBS = -lwiringPi

LoraSimpleMQTT: main.o LoRa.o base64.o PubSubClient.o
	g++ main.o LoRa.o base64.o PubSubClient.o $(LIBS) -o LoraSimpleMQTT

main.o: main.cpp
	g++ -c main.cpp
	
LoRa.o: LoRa.cpp LoRa.h
	g++ -c LoRa.cpp
	
base64.o: base64.c base64.h
	g++ -c base64.c

PubSubClient.o: PubSubClient.cpp PubSubClient.h
	g++ -c PubSubClient.cpp
	
clean:
	rm *.o LoraSimpleMQTT