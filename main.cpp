/*******************************************************************************
 *
 * Copyright (c) 2015 Thomas Telkamp
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 *******************************************************************************/

#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <cstring>
#include <sys/ioctl.h>
#include <net/if.h>

using namespace std;

#include "base64.h"

#include <wiringPi.h>
#include <wiringPiSPI.h>

/*******************************************************************************
 *
 * Lora Configurations!
 *
 *******************************************************************************/
#include "LoRa.h"

const long freq = 915E6;
const int SF = 7;
const long bw = 125E3;

int counter, lastCounter;

/*******************************************************************************
 *
 * Configure these values!
 *
 *******************************************************************************/

// SX1272 - Raspberry connections Wiring Pi Connections
int ssPin = 6;
int dio0  = 0;
int RST   = 3;
static const int CHANNEL = 0;

int main () {

    // Setup Wiring Pi
    wiringPiSetup () ;
    wiringPiSPISetup(CHANNEL, 500000);

    printf("Configuring SX1276\n");
    LoRa.setPins(ssPin,dio0,RST);
    
    printf("Starting LoRa Server\n");
    if (!LoRa.begin(freq)) {
    printf("\nStarting LoRa failed!\n");
    while (1);
    }

    LoRa.setSpreadingFactor(SF);
    printf("\nSystem Configured with:\n");
    // LoRa.setSignalBandwidth(bw);
    //printf("LoRa Started");
    printf("Frequency %li\n", freq);
    printf("Bandwidth %li\n",bw);
    printf("SF: %i\n=========\n", SF);
    //System Configured

    while(1){
      // try to parse packet
      int packetSize = LoRa.parsePacket();
      if (packetSize) {
        // received a packet
        string message = "";        // Clear message string
      while (LoRa.available()) {
        message = message + ((char)LoRa.read());
      };
      string rssi = "\"RSSI\":\"" + String(LoRa.packetRssi()) + "\"";
      string jsonString = message;
      jsonString.replace("xxx", rssi);
    
      int ii = jsonString.indexOf("Count", 1);
      string count = jsonString.substring(ii + 8, ii + 11);
      counter = count.toInt();
      // Same Message Received
      if (counter - lastCounter == 0) Serial.println("Repetition");
      lastCounter = counter;

      // Finished recieving send Ack
      sendAck(message);
      Serial.print("Message Recieved and Acknowledged: ");
      Serial.println(jsonString);

      string value1 = jsonString.substring(8, 11);  // Vcc or heighth
      string value2 = jsonString.substring(23, 26); //counter
    };
  };
    return (0);

}

void sendAck(String message) {
  int check = 0;
  // Calculate Check Sum
  for (int i = 0; i < message.length(); i++) {
    check += message[i];
  }
  LoRa.beginPacket();
  LoRa.print(String(check));  // Send Check Sum
  LoRa.endPacket();
  //Serial.print(message);
  //Serial.print(" ");
  //Serial.print("Ack Sent: ");
  //Serial.println(check);
}