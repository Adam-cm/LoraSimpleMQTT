# LoRaSimpleMQTT
# Single Channel LoRaWAN Gateway

LoraSimpleMQTT: LoRa.o main.o base64.o
	g++ LoRa.o main.o base64.o -o LoraSimpleMQTT

LoRa.o: LoRa.cpp LoRa.h
	g++ -c LoRa.cpp

main.o: main.cpp
	g++ -c main.cpp
	
base64.o: base64.c base64.h
	g++ -c base64.c
clean:
	rm *.o LoraSimpleMQTT