# LoRaSimpleMQTT
# Single Channel LoRaWAN Gateway

LIBS = -lwiringPi -L /usr/include/python3.7 -lpython3.7

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