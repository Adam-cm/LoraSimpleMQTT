# LoRaSimpleMQTT
# Single Channel LoRaWAN Gateway

LIBS = -lwiringPi -lpython3.7

find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})
target_link_libraries(<your exe or lib> ${PYTHON_LIBRARIES})

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