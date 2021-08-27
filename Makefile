# LoRaSimpleMQTT
# Single Channel LoRaWAN Gateway

LIBS = -lwiringPi -lmosquitto

LoraSimpleMQTT: main.o LoRa.o base64.o raspberry_osio_client.o
	g++ main.o LoRa.o base64.o raspberry_osio_client.o $(LIBS) -o LoraSimpleMQTT

main.o: main.cpp
	g++ -c main.cpp
	
LoRa.o: LoRa.cpp LoRa.h
	g++ -c LoRa.cpp
	
base64.o: base64.c base64.h
	g++ -c base64.c
	
raspberry_osio_client.o: raspberry_osio_client.cpp raspberry_osio_client.h
	g++ -c raspberry_osio_client.cpp
	
clean:
	rm *.o LoraSimpleMQTT