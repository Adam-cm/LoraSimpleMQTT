# LoRaSimpleMQTT
# Single Channel LoRaWAN Gateway

LIBS = -lwiringPi

LoraSimpleMQTT: main.o LoRa.o base64.o Stream.o
	g++ main.o LoRa.o base64.o Stream.o $(LIBS) -o LoraSimpleMQTT

main.o: main.cpp
	g++ -c main.cpp
	
LoRa.o: LoRa.cpp LoRa.h Stream.h
	g++ -c LoRa.cpp
	
base64.o: base64.c base64.h
	g++ -c base64.c
	
Stream.o: Stream.cpp Stream.h
	g++ -c Stream.o

clean:
	rm *.o LoraSimpleMQTT