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

#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>

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
#include <unistd.h>

using namespace std;

#include "base64.h"

#define DEBUG 0

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
string Temp_UMQTT = "ERR";
string Temp_DMQTT = "ERR";
string Humidity_UMQTT = "ERR";
string Humidity_DMQTT = "ERR";
string FrameCountMQTT = "ERR";
string RSSIMQTT = "ERR";

// Weather System variables
string AmbientTempMQTT = "ERR";
string RaspiTempMQTT = "ERR";
string WindSpeedMQTT = "ERR";

// Topic and Payload Structure
string TOPIC = "channels/" + ChannelID1 + "/publish";
string PAYLOAD = "field1=" + Temp_UMQTT + "&field2=" + Humidity_UMQTT + "&field3=" + FrameCountMQTT + "&field4=" + RSSIMQTT;

// Connection Parameters
#define QOS         0
#define TIMEOUT     10000L
int rc;

// Number of Fields and Names
string field1 = "Temp_U";
string field2 = "Temp_D";
string field3 = "Humidity_U";
string field4 = "Humidity_D";
string field5 = "Count";
string field6 = "RSSI";

// Number of Fields and Names
string field7 = "HumidityW";

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
#include <wiringPi.h>
#include <wiringPiSPI.h>

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

// Get and Return CPU_TEMP
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
    
    // Local Variables
    string node = message.substr(message.find("N", 0) + 4, 1);  // Identify Node Number
    int check = 0;                                              // Int to store CheckSUM
    string reply = "";                                          // String to store reply message
    ostringstream oss;                                          // String stream to prepare messages

    // Calculate Check Sum from message received
    for (int i = 0; i < message.length(); i++) {
        check += message[i];
    }

    // Handle Situation according to node number
    if (node == "1") {
        // Store Check Sum received
        reply = to_string(check); 

        //#ifdef REPLY_NODE1
        // Update variable to transmit
        AmbientTempMQTT = to_string(updateCPUTEMP()); 
        // Prepare reply message (Send most recent WindSpeed and AmbientTEMP)
        oss.precision(4);                             // Set string stream precision to 4
        oss << "{\"N\":\"G\",\"CheckSum\":\"" << check << "\",\"TempW\":\"" << AmbientTempMQTT << "\",\"Wind\":\"" << WindSpeedMQTT << "\"}";
        reply = oss.str();  // Store string stream
        //#endif

        // Send Packet Reply
        LoRa.beginPacket();                                       // Setup LoRa CHIP
        LoRa.write(reply.c_str(),strlen((char *)reply.c_str()));  // Send Reply String
        LoRa.endPacket();                                         // Finish LoRa Transmit
    }
    else if (node == "2"){
        // Store Check Sum received
        reply = to_string(check);       
        
        //#ifdef REPLY_NODE1
        // Update variable to transmit
        AmbientTempMQTT = to_string(updateCPUTEMP()); 
        // Prepare reply message (Send most recent WindSpeed and AmbientTEMP)
        oss.precision(4);                             // Set string stream precision to 4
        oss << "{\"N\":\"G\",\"CheckSum\":\"" << check << "\",\"TempW\":\"" << AmbientTempMQTT << "\",\"Wind\":\"" << WindSpeedMQTT << "\"}";
        reply = oss.str();  // Store string stream
        //#endif

        // Send Packet Reply
        LoRa.beginPacket();                                         // Setup LoRa CHIP
        LoRa.write(reply.c_str(),strlen((char *)reply.c_str()));    // Send Check Sum
        LoRa.endPacket();                                           // Finish LoRa Transmit
    }
    else {
        // Unknown Message from Node Detected
        //printf("Unknown Node");
        if(DEBUG){
            cout << "Unknown Node";
        }
        //syslog(LOG_NOTICE,"Unknown Node");
    }
    LoRa.receive();
    return;
}

bool setup_MQTT() {
    // Create MQTT Client variables

    // Create Client
    if ((rc = MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS) {

        if(DEBUG){
            cout << "Failed to create client, return code " << rc << endl;
        }
        //(LOG_NOTICE,"Failed to create client, return code %d\n", rc);
        //printf("Failed to create client, return code %d\n", rc);
        //sleep(5);
        return false;
    }

    // Define connection variables
    conn_opts.keepAliveInterval = 120;
    conn_opts.cleansession = true;
    conn_opts.connectTimeout = 30;

    // Connect to MQTT Broker (Thingspeak)
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        //printf("Failed to connect, return code %d\n", rc);
        if(DEBUG){
            cout << "Failed to connect, return code " << rc << endl;
        }
        //syslog(LOG_NOTICE,"Failed to connect, return code %d\n", rc);
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
        //printf("!! Failed to publish message, return code %d\n", rc);
        if(DEBUG){
            cout << "!! Failed to publish message, return code " << rc << endl;
        }
        //syslog(LOG_NOTICE,"!! Failed to publish message, return code %d\n", rc);
        return false;
    }
    else{
        //printf("- Publication Succeeded!\n");
        if(DEBUG){
            cout << "- Publication Succeeded!" << endl;
        }
        //syslog(LOG_NOTICE,"- Publication Succeeded!\n");
        return true;
    }
}

bool die_MQTT() {
    if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS) {
        //printf("Failed to disconnect, return code %d\n", rc);
        if(DEBUG){
            cout << "Failed to disconnect, return code " << rc << endl;
        }
        //syslog(LOG_NOTICE,"Failed to disconnect, return code %d\n", rc);
        return false;
    }
    MQTTClient_destroy(&client);
    return true;
}

string update_MQTT(string jsonString) {
    string node = jsonString.substr(jsonString.find("N", 0) + 4, 1);
    if(DEBUG == 1){
        cout << "\n\nNode: " << node;
        //syslog(LOG_NOTICE,"\n\nNode: %s", node);
    }

    if (node == "1") {
        // Update Control System Variables
        Temp_UMQTT = jsonString.substr(jsonString.find(field1, 1) + field1.length() + 3, 5);
        Humidity_UMQTT = jsonString.substr(jsonString.find(field3, 1) + field3.length() + 3, 4);
        //FrameCountMQTT = jsonString.substr(jsonString.find(field5, 1) + field5.length() + 3, 3);
        FrameCountMQTT = "0";
        RSSIMQTT = jsonString.substr(jsonString.find(field6, 1) + field6.length() + 3, 3);

        // Update Payload String
        PAYLOAD = "field1=" + Temp_UMQTT + "&field2=" + Humidity_UMQTT + "&field3=" + FrameCountMQTT + "&field4=" + RSSIMQTT;
        //printf("\nMessage sent to MQTT Broker from Upstairs\n");
        if(DEBUG){
            cout << "\nMessage sent to MQTT Broker from Upstairs" << endl;
        }
        //syslog(LOG_NOTICE,"\nMessage sent to MQTT Broker from Upstairs\n");
    }
    else if (node == "2") {
        // Update Weather Station Variables
        Temp_DMQTT = jsonString.substr(jsonString.find(field3, 1) + field3.length() + 10, 5);
        Humidity_DMQTT = jsonString.substr(jsonString.find(field4, 1) + field4.length() + 3, 4);
        //FrameCountMQTT = jsonString.substr(jsonString.find(field5, 1) + field5.length() + 3, 4);
        FrameCountMQTT = "0";
        RSSIMQTT = jsonString.substr(jsonString.find(field6, 1) + field6.length() + 3, 3);

        // Sending CPU Temp as AMBIENT
        RaspiTempMQTT = to_string(updateCPUTEMP()); // Update variable to transmit

        // Update Payload String
        PAYLOAD = "field1=" + Temp_DMQTT + "&field2=" + Humidity_DMQTT + "&field3=" + RaspiTempMQTT + "&field4=" + FrameCountMQTT + "&field5=" + RSSIMQTT;
        //printf("\nMessage sent to MQTT Broker from Downstairs\n");
        if(DEBUG){
            cout << "\nMessage sent to MQTT Broker from Downstairs" << endl;
        }
        //syslog(LOG_NOTICE,"\nMessage sent to MQTT Broker from Downstairs\n");

        
    }
    if(DEBUG == 1){
        //printf("-- DEBUG --\n");
        cout << "-- DEBUG --" << endl;
        cout << "- Message recieved: " << jsonString << "\n";
        cout << "- PAYLOAD: " << PAYLOAD << "\n";
        //syslog(LOG_NOTICE,"-- DEBUG --\n");
        //syslog(LOG_NOTICE,"- Message recieved: %s\n",jsonString);
        //syslog(LOG_NOTICE,"- PAYLOAD: %s\n",PAYLOAD);
    }

    return node;
}

static void skeleton_daemon()
{
    pid_t pid;
    
    /* Fork off the parent process */
    pid = fork();
    
    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);
    
     /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);
    
    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);
    
    /* Catch, ignore and handle signals */
    /*TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    
    /* Fork off for the second time*/
    pid = fork();
    
    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);
    
    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);
    
    /* Set new file permissions */
    umask(0);
    
    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/usr/sbin");
    
    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }
    
    /* Open the log file */
    openlog ("LoraSimpleMQTT", LOG_PID, LOG_DAEMON);
}

// Prepare state machine
enum state{init,scan,respond,slumber};
state c_state = init;

void onReceive(int packetSize) {
    c_state = respond;
    return;
}

int main() {

    skeleton_daemon();

    syslog (LOG_NOTICE, "LoraSimpleMQTT daemon started.");

    while(1){
        switch(c_state){
            case init:{
                /*******************************************************************************
                 *
                 * Setup Variables
                 *
                *******************************************************************************/
                if(DEBUG){
                    cout << "\n======================================================" << endl;
                    cout << "\n -  -  - -- IoT Control System: Wetlands -- -  -  - - " << endl;
                    cout << "\n======================================================\n" << endl;

                    // Setup MQTT
                    cout << " Starting MQTT Client " << endl;
                }
                //syslog(LOG_NOTICE,"\n======================================================\n\n");
                //syslog(LOG_NOTICE,"\n -  -  - -- IoT Control System: Wetlands -- -  -  - - \n");
                //syslog(LOG_NOTICE,"\n======================================================\n\n");

                // Setup Wiring Pi
                wiringPiSetup();                      // Start wiring Pi

                // Start MQTT Client
                bool status = setup_MQTT();

                if (status == true) {
                    //syslog(LOG_NOTICE," MQTT Client Status : ONLINE");
                    if(DEBUG){
                        cout << " MQTT Client Status : ONLINE" << endl;
                    }
                }
                else {
                    //syslog(LOG_NOTICE," MQTT Client Status : OFFLINE");
                    if(DEBUG){
                        cout << " MQTT Client Status : OFFLINE" << endl;
                    }
                    break;
                }

                // Setup LoRa Communications
                // Configure Gateway
                if(DEBUG){
                    cout << "\n Starting LoRa Gateway" << endl;
                }
                //syslog(LOG_NOTICE,"\n Starting LoRa Gateway\n");
                LoRa.setPins(ssPin, RST, dio0);             // Set module pins

                // Start LoRa with Freq
                if (!LoRa.begin(freq)) {
                    if(DEBUG){
                        cout << "\n Starting LoRa failed!" << endl;
                    }
                    //syslog(LOG_NOTICE,"\n Starting LoRa failed!\n");
                    c_state = init;
                    sleep(60);
                    break;
                }
                LoRa.setSpreadingFactor(SF);                // Set Spreading Factor
                // LoRa.setSignalBandwidth(bw);

                // Print Console, configuration successful
                if(DEBUG){
                    cout << "\n - - LoRa Configuration - - " << endl;
                    cout << "  Frequency: " << freq << " Hz" << endl;
                    cout << "  Bandwidth: " << bw << endl;
                    cout << "  Spreading Factor : " << SF << "\n\n======================================================\n" << endl;
                }

                LoRa.onReceive(onReceive);
                LoRa.receive();

                //System Configured
                c_state = slumber;
                break;
            }
            case scan:{
                if (LoRa.parsePacket()) {
                    c_state = respond;
                }
                break;
            }
            case respond:{
                if(DEBUG){
                    //cout << "\nPACKET RECIEVED!" << endl;
                    //syslog(LOG_NOTICE,"\nPACKET RECIEVED!\n");
                }
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
                    //printf("Error: Unknown node detected\n");
                    if(DEBUG){
                        cout << "Error: Unknown node detected" << endl;
                    }
                    //syslog(LOG_NOTICE,"Error: Unknown node detected\n");
                    c_state = slumber;
                    break;
                }

                // Check if MQTT is still open
                if(!(MQTTClient_isConnected(client))){
                    if(DEBUG == 1){
                        //printf(" {MQTT Client Status: OFFLINE}\n");
                        cout << " {MQTT Client Status: OFFLINE}" << endl;
                        //syslog(LOG_NOTICE," {MQTT Client Status: OFFLINE}\n");
                    }

                    die_MQTT();
                    sleep(10);

                    bool status = setup_MQTT();
                    sleep(10);

                    if (status == true && DEBUG == 1) {
                        //printf(" {MQTT Restarted, Client Status: ONLINE}\n");
                        cout << " {MQTT Restarted, Client Status: ONLINE}" << endl;
                        //syslog(LOG_NOTICE," {MQTT Restarted, Client Status: ONLINE}\n");
                    }
                }
                else if(DEBUG){
                   //printf(" {MQTT Client Status: ONLINE}\n");
                   cout << " {MQTT Client Status: ONLINE}" << endl;
                   //syslog(LOG_NOTICE," {MQTT Client Status: ONLINE}\n");
                }

                c_state = slumber;
                break;
            }
            case slumber:{
                //sleep(120);
                //LoRa.receive();
                LoRa.receive();
                break;
            }
            default:{
                c_state = slumber;
                break;
            }
        }
    }
}