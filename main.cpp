/*******************************************************************************
 *
 * IoT Wetlands Gateway (Raspberry Pi)
 *
 * Provides a LoRa connection operating on a single frequency and spreading
 * factor. It implments a simple point to point communication to allow
 * data managment and buffering.
 * 
 * TO_DO:
 * Need to implement a MQTT server to publish to the thingspeak network
 * 
 * Created By: Adam Hawke
 * Created: 26/08/2021
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
 * MQTT Configuration
 *
 *******************************************************************************/

#include "MQTTClient.h"

#define ADDRESS     "mqtt3.thingspeak.com:1883"
#define CLIENTID    "JC0zDR4uMTgkNDEPLxUnGgM"
#define MQTTUSERNAME "JC0zDR4uMTgkNDEPLxUnGgM"
#define MQTTPASSWORD "xI+jK1cSqSbFwUcLLcMTZJEu"
string ChannelID = "1488787";
string writeapiKey = "F2G2A2ASFRSDM35M";
string TempMQTT = "22.5";
#define readapiKey   "SHJERDVYG0EDGHCH"

string TOPIC = "channels/" + ChannelID + "/publish/" + writeapiKey;
string PAYLOAD = "field1=" + TempMQTT;

#define QOS         0
#define TIMEOUT     10000L

/*******************************************************************************
 *
 * Lora Paramaters
 *
 *******************************************************************************/
#include "LoRa.h"

const long freq = 915E6;
const int SF = 7;
const long bw = 125E3;

int counter, lastCounter;

/*******************************************************************************
 *
 * Wiring Pi Configuration
 *
 *******************************************************************************/

// SX1272 - Raspberry connections Wiring Pi Connections
int ssPin = 6;
int dio0  = 0;
int RST   = 3;
static const int CHANNEL = 0;

/*******************************************************************************
 *
 * Functions
 *
 *******************************************************************************/

// Replace string values
bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

// Message Reply
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
}

/*******************************************************************************
 *
 * Main Program
 * 
 *******************************************************************************/

int main () {
  
    // Console Print
    printf("\n -  -  - -- IoT Control System: Wetlands -- -  -  - - \n");
    printf("\n======================================================\n\n");

    // Setup Wiring Pi
    wiringPiSetup () ;                      // Start wiring Pi
    LoRa.setPins(ssPin,RST,dio0);           // Set module pins
    
    // Configure Gateway
    printf(" Starting LoRa Gateway\n");
    // Start LoRa with Freq
    if (!LoRa.begin(freq)) {
      printf("\nStarting LoRa failed!\n");  
      while (1);
    }
    LoRa.setSpreadingFactor(SF);            // Set Spreading Factor
    // LoRa.setSignalBandwidth(bw);

    // Print Console, configuration successful
    printf("\n - - System Configuration - - \n");
    printf("  Frequency: %li Hz\n", freq);
    printf("  Bandwidth: %li\n",bw);
    printf("  Spreading Factor: %i\n\n======================================================\n\n", SF);
    //System Configured

/*******************************************************************************
 *
 * MQTT
 * 
 *******************************************************************************/
    MQTTClient client;
    MQTTClient_connectOptions conn_opts =  { {'M', 'Q', 'T', 'C'}, 6, 60, 1, 1, NULL, (char *)MQTTUSERNAME, (char *)MQTTPASSWORD, 30, 0, NULL, 0, NULL, MQTTVERSION_DEFAULT, {NULL, 0, 0}, {0, NULL}, -1, 0};
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    if ((rc = MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
         printf("Failed to create client, return code %d\n", rc);
         exit(EXIT_FAILURE);
    }

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    pubmsg.payload = (char *)PAYLOAD.c_str();
    pubmsg.payloadlen = (int)strlen((char *)PAYLOAD.c_str());
    printf("String length =: %i\n",(int)strlen((char *)PAYLOAD.c_str()));
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    if ((rc = MQTTClient_publishMessage(client, (char *)TOPIC.c_str(), &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
    {
         printf("Failed to publish message, return code %d\n", rc);
         exit(EXIT_FAILURE);
    }

    printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(TIMEOUT/1000), (char *)PAYLOAD.c_str(), (char *)TOPIC.c_str(), CLIENTID);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    printf("Message with delivery token %d delivered\n", token);

    if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
    	printf("Failed to disconnect, return code %d\n", rc);
    MQTTClient_destroy(&client);


/*******************************************************************************
 *
 * Main Loop
 * 
 *******************************************************************************/
    while(1){
      // Check for LoRa Message
      int packetSize = LoRa.parsePacket();
      // Message Handling
      if (packetSize){
        // received a packet
        string message = "";                              // Clear message string
        // Store Message in string Message
        while (LoRa.available()) {
          message = message + ((char)LoRa.read());
        }
        // Reply to Node with Ack
        sendAck(message);

        // Present Message
        string pktrssi = to_string(LoRa.packetRssi());    // Store RSSI Value
        string rssi = ("\"RSSI\":\"" + pktrssi + "\"");   // Construct RSSI String with metadata
        string jsonString = message;                      // Store message in jsonString
        replace(jsonString, "xxx", rssi);                 // Replace xxx with RSSI value and metadata

        // Check count value for repeated messages
        int ii = jsonString.find("Count", 1);
        string count = jsonString.substr(ii + 8, ii + 11);
        counter = stoi(count);
        // Same Message Received
        if (counter - lastCounter == 0){
          printf("Repetition\n");
        } 

        // Different Message Received print to console
        else{
          printf("Message: %s\n",jsonString.c_str());   // Print Message Received
        }
        // Update Counter
        lastCounter = counter;
      }
    }
  return (0);
}