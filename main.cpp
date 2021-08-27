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

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

void sendAck(string message) {
  int check = 0;
  // Calculate Check Sum
  for (int i = 0; i < message.length(); i++) {
    check += message[i];
  }
  
  string checksum = to_string(check);

  //printf("\nCheck sum reply: %s\n",checksum.c_str());

  LoRa.beginPacket();
  
  LoRa.write(checksum.c_str(),4);  // Send Check Sum

  LoRa.endPacket();
  //Serial.print(message);
  //Serial.print(" ");
  //Serial.print("Ack Sent: ");
  //Serial.println(check);
}

int main () {
  // Setup Wiring Pi
    wiringPiSetup () ;
    //wiringPiSPISetup(CHANNEL, 500000);

    printf("Configuring SX1276\n");
    LoRa.setPins(ssPin,RST,dio0);
    
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
    printf("SF: %i\n=================\n", SF);
    //System Configured

    while(1){
      int packetSize = LoRa.parsePacket();
      if (packetSize) {
        // received a packet
        printf("Packet Received\n");
      }
    }

    while(1){
      // try to parse packet
      int packetSize = LoRa.parsePacket();
      //printf("Waiting to Receive pakcets: %i\n",packetSize);

      if (packetSize) {
        // received a packet
        printf("Packet Received\n");
        string message = "";        // Clear message string
        while (LoRa.available()) {
          message = message + ((char)LoRa.read());
        }
      //printf("Message: %s\n",message.c_str());

      string pktrssi = to_string(LoRa.packetRssi());
      string rssi = ("\"RSSI\":\"" + pktrssi + "\"");
      string jsonString = message;
      //jsonString.replace("xxx", rssi);
      replace(jsonString, "xxx", rssi);

      //printf("Message: %s\n",jsonString.c_str());
      
      int ii = jsonString.find("Count", 1);
      string count = jsonString.substr(ii + 8, ii + 11);
      counter = stoi(count);
      // Same Message Received
      if (counter - lastCounter == 0){
        printf("Repetition");
      } 
      lastCounter = counter;

      // Finished recieving send Ack
      sendAck(message);
      printf("Message Recieved and Acknowledged: %s\n",jsonString.c_str());
      //string value1 = jsonString.substring(8, 11);  // Vcc or heighth
      //string value2 = jsonString.substring(23, 26); //counter*/
      }
    }
  return (0);
}