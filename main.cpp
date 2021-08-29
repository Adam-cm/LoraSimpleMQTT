/*******************************************************************************
 *
 * IoT Wetlands Gateway (Raspberry Pi)
 *
 * Provides a LoRa connection operating on a single frequency and spreading
 * factor. It implments a simple point to point communication to allow
 * data managment and buffering.
 * 
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

// MQTT Connection definitions
#define ADDRESS     "tcp://mqtt3.thingspeak.com:1883"
#define CLIENTID    "JC0zDR4uMTgkNDEPLxUnGgM"
#define MQTTUSERNAME "JC0zDR4uMTgkNDEPLxUnGgM"
#define MQTTPASSWORD "xI+jK1cSqSbFwUcLLcMTZJEu"
string ChannelID = "1488787";

// Variables Shared/Sent
string Temp1MQTT = "00.0";
string Temp2MQTT = "00.0";
string TurbidityMQTT = "0.0";
string FrameCountMQTT = "000";
string RSSIMQTT = "-00";

// Topic and Payload Structure
string TOPIC = "channels/" + ChannelID + "/publish";
string PAYLOAD = "field1=" + Temp1MQTT + "&field2=" + Temp2MQTT + "&field3=" + TurbidityMQTT + "&field4=" + FrameCountMQTT + "&field5=" + RSSIMQTT;

// Connection Parameters
#define QOS         0
#define TIMEOUT     10000L
int rc;

// Number of Fields and Names
string field1 = "Temp1";
string field2 = "Temp2";
string field3 = "Turbidity";
string field4 = "Count";
string field5 = "RSSI";

// MQTT Client Variables
MQTTClient client;
MQTTClient_connectOptions conn_opts =  { {'M', 'Q', 'T', 'C'}, 6, 60, 1, 1, NULL, (char *)MQTTUSERNAME, (char *)MQTTPASSWORD, 30, 0, NULL, 0, NULL, MQTTVERSION_DEFAULT, {NULL, 0, 0}, {0, NULL}, -1, 0};


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

bool setup_MQTT(){
  // Create MQTT Client variables
  
  // Create Client
  if ((rc = MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS){
    printf("Failed to create client, return code %d\n", rc);
    return false;
  }

  // Define connection variables
  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;

  // Connect to MQTT Broker (Thingspeak)
  if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS){
    printf("Failed to connect, return code %d\n", rc);
    return false;
  }

  return true;
}

bool send_MQTT(string payload){
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  MQTTClient_deliveryToken token;
  // Format Payload
  pubmsg.payload = (char *)PAYLOAD.c_str();
  pubmsg.payloadlen = (int)strlen((char *)PAYLOAD.c_str());
  pubmsg.qos = QOS;
  pubmsg.retained = 0;

  if ((rc = MQTTClient_publishMessage(client, (char *)TOPIC.c_str(), &pubmsg, &token)) != MQTTCLIENT_SUCCESS){
    printf("Failed to publish message, return code %d\n", rc);
    return false;
  }

  // Print output
  //printf("Waiting for up to %d seconds for publication of %s\n" "on topic %s for client with ClientID: %s\n", (int)(TIMEOUT/1000), (char *)PAYLOAD.c_str(), (char *)TOPIC.c_str(), CLIENTID);
  rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
  printf("MQTT Message delivered\n");
  //printf("Message with delivery token %d delivered\n", token);

  return true;
}

bool die_MQTT(){
  if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
  printf("Failed to disconnect, return code %d\n", rc);
  MQTTClient_destroy(&client);
  return true;
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

    // Setup MQTT
    printf(" Starting MQTT Client \n");
    bool status = setup_MQTT();
    if(status == true){
      printf(" MQTT Client Status: ONLINE\n");
    }
    else{
      printf(" MQTT Client Status: OFFLINE\n");
    }

    // Configure Gateway
    printf("\n Starting LoRa Gateway\n");
    // Start LoRa with Freq
    if (!LoRa.begin(freq)) {
      printf("\nStarting LoRa failed!\n");  
      while (1);
    }
    LoRa.setSpreadingFactor(SF);            // Set Spreading Factor
    // LoRa.setSignalBandwidth(bw);

    // Print Console, configuration successful
    printf("\n - - LoRa Configuration - - \n");
    printf("  Frequency: %li Hz\n", freq);
    printf("  Bandwidth: %li\n",bw);
    printf("  Spreading Factor: %i\n\n======================================================\n\n", SF);
    //System Configured

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
          //printf("Message: %s\n",jsonString.c_str());   // Print Message Received
          // Update PAYLOAD VARIABLES
          Temp1MQTT = jsonString.substr(jsonString.find(field1, 1)+field1.length()+3,4);
          Temp2MQTT = jsonString.substr(jsonString.find(field2, 1)+field2.length()+3,4);
          TurbidityMQTT = jsonString.substr(jsonString.find(field3, 1)+field3.length()+3,1);
          FrameCountMQTT = jsonString.substr(jsonString.find(field4, 1)+field4.length()+3,3);
          RSSIMQTT = jsonString.substr(jsonString.find(field5, 1)+field5.length()+3,3);
          //printf("Count find: %i\n",jsonString.find("Count", 0)+strlen("Count"));
          //printf("Count Substring: %s\n",FrameCountMQTT.c_str());

          PAYLOAD = "field1=" + Temp1MQTT + "&field2=" + Temp2MQTT + "&field3=" + TurbidityMQTT + "&field4=" + FrameCountMQTT + "&field5=" + RSSIMQTT;

          // Send Message to Thingspeak
          send_MQTT(PAYLOAD);
        }
        // Update Counter
        lastCounter = counter;
      }
    }
  return (0);
}
