/*******************************************************************************
 *
 * IoT Wetlands Gateway (Raspberry Pi)
 *
 * Provides a LoRa connection operating on a single frequency and spreading
 * factor. It implments a simple point to point communication to allow
 * data managment and buffering.
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
//#include <string.h>
#include <sstream>
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
string ChannelID1 = "1488787";
string ChannelID2 = "1490440";

// Control System variables
string Temp1MQTT = "ERR";
string Temp2MQTT = "ERR";
string TurbidityMQTT = "ERR";
string FrameCountMQTT = "ERR";
string RSSIMQTT = "ERR";

// Weather System variables
string AmbientTempMQTT = "ERR";
string WindSpeedMQTT = "ERR";

// Topic and Payload Structure
string TOPIC = "channels/" + ChannelID1 + "/publish";
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

// Number of Fields and Names
string field6 = "Wind";

// MQTT Client Variables
MQTTClient client;
MQTTClient_connectOptions conn_opts = { {'M', 'Q', 'T', 'C'}, 6, 60, 1, 1, NULL, (char*)MQTTUSERNAME, (char*)MQTTPASSWORD, 30, 0, NULL, 0, NULL, MQTTVERSION_DEFAULT, {NULL, 0, 0}, {0, NULL}, -1, 0 };

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
int dio0 = 0;
int RST = 3;
static const int CHANNEL = 0;

/*******************************************************************************
 *
 * Functions
 *
 *******************************************************************************/

 // Replace string values
bool replace(string& str, const string& from, const string& to) {
    size_t start_pos = str.find(from);
    if (start_pos == string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

// Get and Return CPUTEMP
float updateCPUTEMP(void){
  float systemp, millideg;
  FILE* thermal;
  int n;
  thermal = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
  n = fscanf(thermal, "%f", &millideg);
  fclose(thermal);
  systemp = millideg / 1000;
  return systemp;
}

// Message Reply
void sendAck(string message) {
    string node = message.substr(message.find("N", 0) + 4, 1);

    int check = 0;
    // Calculate Check Sum
    for (int i = 0; i < message.length(); i++) {
        check += message[i];
    }
    string reply = "";
    ostringstream oss;
    // Convert string into int
    if (node == "1") {
        //sprintf(reply, "{\"N\":\"2\",\"CheckSum\":\"%i\",\"TempW\":\"%s\",\"Wind\":\"%s\"}", check, AmbientTempMQTT, WindSpeedMQTT);
        //reply = "{\"N\":\"2\",\"CheckSum\":\"" + check + "\",\"TempW\":\"" + AmbientTempMQTT + "\",\"Wind\":\"" + WindSpeedMQTT + "\"}";
        oss.precision(4);
        AmbientTempMQTT = to_string(updateCPUTEMP()); // Update variable to transmit
        oss << "{\"N\":\"G\",\"CheckSum\":\"" << check << "\",\"TempW\":\"" << AmbientTempMQTT << "\",\"Wind\":\"" << WindSpeedMQTT << "\"}";
        reply = oss.str();
        //printf("\n Packet Prepared! %s", reply);
        LoRa.beginPacket();
        LoRa.write(reply.c_str(),strlen((char *)reply.c_str()));  // Send Check Sum
        LoRa.endPacket();

        //string reply = to_string(check);
          //printf("\nCheck sum reply: %s\n",checksum.c_str());
        //LoRa.beginPacket();
        //LoRa.write(reply.c_str(), 4);  // Send Check Sum
        //LoRa.endPacket();
    }
    else if (node == "2") {
        string reply = to_string(check);
        //printf("\nCheck sum reply: %s\n",checksum.c_str());
        LoRa.beginPacket();
        LoRa.write(reply.c_str(), 4);  // Send Check Sum
        LoRa.endPacket();
    }
    else {
        printf("Unknown Node");
    }
}

bool setup_MQTT() {
    // Create MQTT Client variables

    // Create Client
    if ((rc = MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to create client, return code %d\n", rc);
        return false;
    }

    // Define connection variables
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    // Connect to MQTT Broker (Thingspeak)
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        return false;
    }

    return true;
}

bool send_MQTT(string payload, string ChannelID) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    TOPIC = "channels/" + ChannelID + "/publish";

    // Format Payload
    pubmsg.payload = (char*)PAYLOAD.c_str();
    pubmsg.payloadlen = (int)strlen((char*)PAYLOAD.c_str());
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    if ((rc = MQTTClient_publishMessage(client, (char*)TOPIC.c_str(), &pubmsg, &token)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to publish message, return code %d\n", rc);
        return false;
    }

    // Print output
    //printf("Waiting for up to %d seconds for publication of %s\n" "on topic %s for client with ClientID: %s\n", (int)(TIMEOUT/1000), (char *)PAYLOAD.c_str(), (char *)TOPIC.c_str(), CLIENTID);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    //printf("MQTT Message delivered\n");
    //printf("Message with delivery token %d delivered\n", token);
    return true;
}

bool die_MQTT() {
    if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
        printf("Failed to disconnect, return code %d\n", rc);
    MQTTClient_destroy(&client);
    return true;
}

string update_MQTT(string jsonString) {
    string node = jsonString.substr(jsonString.find("N", 0) + 4, 1);

    if (node == "1") {
        // Update Control System Variables
        Temp1MQTT = jsonString.substr(jsonString.find(field1, 1) + field1.length() + 3, 4);
        Temp2MQTT = jsonString.substr(jsonString.find(field2, 1) + field2.length() + 3, 4);
        TurbidityMQTT = jsonString.substr(jsonString.find(field3, 1) + field3.length() + 3, 1);
        FrameCountMQTT = jsonString.substr(jsonString.find(field4, 1) + field4.length() + 3, 3);
        RSSIMQTT = jsonString.substr(jsonString.find(field5, 1) + field5.length() + 3, 3);

        // Update Payload String
        PAYLOAD = "field1=" + Temp1MQTT + "&field2=" + Temp2MQTT + "&field3=" + TurbidityMQTT + "&field4=" + FrameCountMQTT + "&field5=" + RSSIMQTT;

        printf("Message sent to MQTT Broker from Control System\n");
    }
    else if (node == "2") {
        // Update Weather Station Variables
        AmbientTempMQTT = jsonString.substr(jsonString.find(field1, 1) + field1.length() + 3, 4);
        WindSpeedMQTT = jsonString.substr(jsonString.find(field6, 1) + field6.length() + 3, 4);
        FrameCountMQTT = jsonString.substr(jsonString.find(field4, 1) + field4.length() + 3, 3);
        RSSIMQTT = jsonString.substr(jsonString.find(field5, 1) + field5.length() + 3, 3);

        // Sending CPU Temp as AMBIENT
        AmbientTempMQTT = to_string(updateCPUTEMP()); // Update variable to transmit

        // Update Payload String
        PAYLOAD = "field1=" + AmbientTempMQTT + "&field2=" + WindSpeedMQTT + "&field3=" + FrameCountMQTT + "&field4=" + RSSIMQTT;

        printf("Message sent to MQTT Broker from Weather Station\n");
    }

    return node;
}

int main() {

    /*******************************************************************************
     *
     * Setup Variables
     *
    *******************************************************************************/

    // Console Print
    printf("\n -  -  - -- IoT Control System: Wetlands -- -  -  - - \n");
    printf("\n======================================================\n\n");

    // Setup Wiring Pi
    wiringPiSetup();                      // Start wiring Pi

    // Setup MQTT
    printf(" Starting MQTT Client \n");
    bool status = setup_MQTT();
    if (status == true) {
        printf(" MQTT Client Status: ONLINE\n");
    }
    else {
        printf(" MQTT Client Status: OFFLINE\n");
    }

    // Setup LoRa Communications
    // Configure Gateway
    printf("\n Starting LoRa Gateway\n");
    LoRa.setPins(ssPin, RST, dio0);             // Set module pins
    // Start LoRa with Freq
    if (!LoRa.begin(freq)) {
        printf("\n Starting LoRa failed!\n");
        exit(EXIT_FAILURE);
    }
    LoRa.setSpreadingFactor(SF);                // Set Spreading Factor
    // LoRa.setSignalBandwidth(bw);

    // Print Console, configuration successful
    printf("\n - - LoRa Configuration - - \n");
    printf("  Frequency: %li Hz\n", freq);
    printf("  Bandwidth: %li\n", bw);
    printf("  Spreading Factor: %i\n\n======================================================\n\n", SF);
    
    //System Configured

    /*******************************************************************************
     *
     * System Logic
     *
    *******************************************************************************/
    while(1) {
        if (LoRa.parsePacket()) {
            // received a packet
            string message = "";                              // Clear message string
            // Store Message in string Message
            while (LoRa.available()) {
                message = message + ((char)LoRa.read());
            }
            //printf("Message Received: %s\n", message.c_str());
            // Reply to Node with Ack
            sendAck(message);

            // Present Message
            string pktrssi = to_string(LoRa.packetRssi());    // Store RSSI Value
            string rssi = ("\"RSSI\":\"" + pktrssi + "\"");   // Construct RSSI String with metadata
            string jsonString = message;                      // Store message in jsonString
            replace(jsonString, "xxx", rssi);                 // Replace xxx with RSSI value and metadata

            string node = update_MQTT(jsonString);

            if (node == "1") {
                // Send Message to Thingspeak 1
                send_MQTT(PAYLOAD, ChannelID1);
            }
            else if (node == "2") {
                // Send Message to Thingspeak 2
                send_MQTT(PAYLOAD, ChannelID2);
            }
            else {
                printf("Error: Unknown node detected\n");
            }

            // Update Counter
            // lastCounter = counter;
        }
    }
}