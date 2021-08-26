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
#include <LoRa.h>

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

// Executed Interrupt
void myInterrupt(void) {
    printf("LoRa Message Received!\n");
}

int main () {

    // Setup Wiring Pi
    wiringPiSetup () ;
    wiringPiSPISetup(CHANNEL, 500000);

    printf("Configuring SX1276\n");
    Lora.setPins(ssPin,dio0,RST);
    
    printf("Starting LoRa Server\n");
    if (!LoRa.begin(freq)) {
    Serial.println("Starting LoRa failed!");
    while (1);
    }

    LoRa.setSpreadingFactor(SF);
    printf("System Configured with:\n");
    // LoRa.setSignalBandwidth(bw);
    //printf("LoRa Started");
    printf("Frequency ");
    printf(freq);
    printf(" Bandwidth ");
    printf(bw);
    printf(" SF ");
    printf(SF);

    //System Configured
    

    // Assigning Interrupt
    if (wiringPiISR(0, INT_EDGE_RISING, &myInterrupt) < 0) {
        fprintf(stderr, "Unable to setup ISR: %s\n", strerror(errno));
        return 1;
    }

    while(1);

    return (0);

}

