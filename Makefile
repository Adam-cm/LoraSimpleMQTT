# LoRaSimpleMQTT
# Single Channel LoRaWAN Gateway

LoraSimpleMQTT: main.o lora.o base64.o
	g++ main.o lora.o base64.o -o LoraSimpleMQTT

main.o: main.cpp
	g++ -c main.cpp
	
lora.o: lora.cpp lora.h
	g++ -c lora.cpp
	
base64.o: base64.c base64.h
	g++ -c base64.c
clean:
	rm *.o LoraSimpleMQTT