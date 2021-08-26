# LoRaSimpleMQTT
# Single Channel LoRaWAN Gateway

LoraSimpleMQTT: main.o LoRa.o base64.o
	g++ main.o LoRa.o base64.o -o LoraSimpleMQTT

main.o: main.cpp
	g++ -c main.cpp
	
LoRa.o: LoRa.cpp LoRa.h
	g++ -c LoRa.cpp
	
base64.o: base64.c base64.h
	g++ -c base64.c
clean:
	rm *.o LoraSimpleMQTT