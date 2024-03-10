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

#define DEBUG 1

/*******************************************************************************
 *
 * MQTT Configuration
 *
 *******************************************************************************/

#include "MQTTClient.h"

// MQTT Connection definitions
#define ADDRESS "tcp://mqtt3.thingspeak.com:1883"
#define CLIENTID "JC0zDR4uMTgkNDEPLxUnGgM"
#define MQTTUSERNAME "JC0zDR4uMTgkNDEPLxUnGgM"
#define MQTTPASSWORD "ZkTS9ofR8tBvBxihOcfLjxbJ"

#define ID_NODE1 "1488787"
#define ID_NODE2 "1490440"
#define ID_RASPI "2463425"

// Connection Parameters
#define QOS 1
#define TIMEOUT 10000L

int rc;

// MQTT Client Variables
MQTTClient client;
MQTTClient_connectOptions conn_opts = {{'M', 'Q', 'T', 'C'}, 6, 60, 1, 1, NULL, (char *)MQTTUSERNAME, (char *)MQTTPASSWORD, 30, 0, NULL, 0, NULL, MQTTVERSION_DEFAULT, {NULL, 0, 0}, {0, NULL}, -1, 0};

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
bool replace(string &str, const string &from, const string &to)
{
    size_t start_pos = str.find(from);
    if (start_pos == string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

// Get and Return CPU_TEMP
float updateCPUTEMP(void)
{
    float systemp, millideg;
    FILE *thermal;
    int n;
    thermal = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    n = fscanf(thermal, "%f", &millideg);
    fclose(thermal);
    systemp = millideg / 1000;
    return systemp;
}

bool setup_MQTT()
{
    // Create MQTT Client variables

    // Create Client
    if ((rc = MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {

        if (DEBUG)
        {
            cout << "Failed to create client, return code " << rc << endl;
        }
        //(LOG_NOTICE,"Failed to create client, return code %d\n", rc);
        // printf("Failed to create client, return code %d\n", rc);
        // sleep(5);
        return false;
    }

    // Define connection variables
    conn_opts.keepAliveInterval = 120;
    conn_opts.cleansession = true;
    conn_opts.connectTimeout = 30;

    // Connect to MQTT Broker (Thingspeak)
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        // printf("Failed to connect, return code %d\n", rc);
        if (DEBUG)
        {
            cout << "Failed to connect, return code " << rc << endl;
        }
        // syslog(LOG_NOTICE,"Failed to connect, return code %d\n", rc);
        return false;
    }

    return true;
}

bool die_MQTT()
{
    if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
    {
        // printf("Failed to disconnect, return code %d\n", rc);
        if (DEBUG)
        {
            cout << "Failed to disconnect, return code " << rc << endl;
        }
        // syslog(LOG_NOTICE,"Failed to disconnect, return code %d\n", rc);
        return false;
    }
    MQTTClient_destroy(&client);
    return true;
}

int c4letters(string data)
{
    for (int i = 0; i < data.length(); i++)
    {
        if (!isdigit(data[i]))
        {
            if (!((data[i] == '.') || (data[i] == '-')))
            {
                if (DEBUG)
                {
                    cout << "Error! Non-number value of: '" << data[i] << "' found" << endl;
                }
                return 1;
            }
        }
    }
    return 0;
}

string extract_between(string jsonString, string start_str, string end_str)
{
    string mid = "\":\"";
    string beg = "\"";
    string end = "\"";

    start_str = beg + start_str + mid;
    end_str = end + end_str;

    size_t first;
    size_t last;
    int error = 0;

    string value = "ERROR";

    if(jsonString.find(start_str) != std::string::npos){
        first = jsonString.find(start_str) + start_str.length();
    }
    else{
        error += 1;
    }

    if(jsonString.find(end_str) != std::string::npos){
        last = jsonString.find(end_str);
    }else{
        error += 1;
    }

    if (error == 0){
        value = jsonString.substr(first, last - first);
    }

    return value;
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
    for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--)
    {
        close(x);
    }

    /* Open the log file */
    openlog("LoraSimpleMQTT", LOG_PID, LOG_DAEMON);
}

class Node
{
private:
    // Node variables
    string ChannelID = "Not Defined";
    string Temperature = "Not Defined";
    string Humidity = "Not Defiend";
    string Frame_Count = "Not Defined";
    string RSSI = "Not Defined";

    int Check_sum = 0;
    int Node_number = 0;
    // Message variables
    string TOPIC = "Not Defined";
    string PAYLOAD = "Not Defined";

public:
    void set_ChannelID(string ID)
    {
        this->TOPIC = "channels/" + ID + "/publish";
        return;
    }
    void update_packet(string Temperature, string Humidity, string Frame_Count, string RSSI)
    {
        this->PAYLOAD = "field1=" + Temperature + "&field2=" + Humidity + "&field3=" + Frame_Count + "&field4=" + RSSI;
        return;
    }
    void update_packet(void)
    {
        this->PAYLOAD = "field1=" + this->Temperature + "&field2=" + this->Humidity + "&field3=" + this->Frame_Count + "&field4=" + this->RSSI;
        return;
    }
    int get_nodenumber(void)
    {
        return this->Node_number;
    }
    void set_payload(string Payload){
        this->PAYLOAD = Payload;
        return;
    }

    // Message Reply
    void sendAck(string message)
    {
        // Variables
        int check = 0;     // Int to store CheckSUM
        string reply = ""; // String to store reply message

        // Calculate Check Sum from message received
        for (int i = 0; i < message.length(); i++)
        {
            check += message[i];
        }

        this->Check_sum = check;

        // Prepare reply (just prepares to send checksum)
        reply = "{\"N\":\"G\",\"CheckSum\":\"" + to_string(this->Check_sum) + "\"}";

        if (DEBUG == 1)
        {
            cout << "Responding with: " << reply << endl;
        }
        // Send Packet Reply
        while (!LoRa.beginPacket())
            ;                                                     // Setup LoRa CHIP
        LoRa.write(reply.c_str(), strlen((char *)reply.c_str())); // Send Reply String
        LoRa.endPacket();                                         // Finish LoRa Transmit
        LoRa.receive();
        return;
    }

    string update_MQTT(string message)
    {
        // Present Message
        string pktrssi = to_string(LoRa.packetRssi());  // Store RSSI Value
        string rssi = ("\"RSSI\":\"" + pktrssi + "\""); // Construct RSSI String with metadata
        string jsonString = message;                    // Store message in jsonString
        replace(jsonString, "xxx", rssi);               // Replace xxx with RSSI value and metadata

        if (DEBUG == 1)
        {
            cout << "\n\nNode: " << this->Node_number;
        }

        // Update variables
        this->Temperature = extract_between(jsonString, "Temperature", ",\"Humidity");
        this->Humidity = extract_between(jsonString, "Humidity", ",\"Count");
        this->Frame_Count = extract_between(jsonString, "Count", ",\"Lost");
        this->RSSI = extract_between(jsonString, "RSSI", "}");

        // Error checking
        int err = 0;
        err = c4letters(this->Temperature) + c4letters(this->Humidity) + c4letters(this->Frame_Count) + c4letters(this->RSSI);

        if (err >= 1)
        {
            if (DEBUG == 1)
            {
                cout << "\nMessage:" << jsonString << endl;
                cout << "Error: Invalid data format detected! Message not sent!" << endl;
            }
            return "100";
        }

        // Update Payload String
        update_packet();

        if (DEBUG)
        {
            cout << "\nMessage sent to MQTT Broker from Node" << this->Node_number << endl;
            cout << "-- DEBUG --" << endl;
            cout << "- Message recieved: " << jsonString << "\n";
            cout << "- PAYLOAD: " << this->PAYLOAD << "\n";
        }
        return to_string(this->Node_number);
    }

    bool send_MQTT(MQTTClient client)
    {
        MQTTClient_message pubmsg = MQTTClient_message_initializer;
        MQTTClient_deliveryToken token;

        // Format Payload
        pubmsg.payload = (char *)this->PAYLOAD.c_str();
        pubmsg.payloadlen = (int)strlen((char *)this->PAYLOAD.c_str());
        pubmsg.qos = QOS;
        pubmsg.retained = 0;

        // Check if MQTT is still open
            if (!(MQTTClient_isConnected(client)))
            {
                if (DEBUG == 1)
                {
                    cout << "- {MQTT Client Status: OFFLINE}" << endl;
                }

                die_MQTT();
                sleep(30);

                bool status = setup_MQTT();
                sleep(30);

                if (status == true && DEBUG == 1)
                {
                    cout << "- {MQTT Restarted, Client Status: ONLINE}" << endl;
                }
                else{
                    cout << "- {MQTT failed to restarted, Client Status: OFFLINE}" << endl;
                    return false;
                }
            }
            else
            {
                cout << "- {MQTT Client Status: ONLINE}" << endl;
            }

        if ((rc = MQTTClient_publishMessage(client, (const char *)this->TOPIC.c_str(), &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
        {
            // printf("!! Failed to publish message, return code %d\n", rc);
            if (DEBUG)
            {
                cout << "!! Failed to publish message, return code " << rc << endl;
            }
            // syslog(LOG_NOTICE,"!! Failed to publish message, return code %d\n", rc);
            return false;
        }
        else
        {
            // printf("- Publication Succeeded!\n");
            if (DEBUG)
            {
                cout << "- MQTT TOPIC " << (char *)this->TOPIC.c_str() << endl;
                cout << "- MQTT Payload " << (char *)this->PAYLOAD.c_str() << endl;
                cout << "- MQTT Payload Length " << (int)strlen((char *)this->PAYLOAD.c_str()) << endl;
                cout << "- Publication Succeeded!" << endl;
            }
            // syslog(LOG_NOTICE,"- Publication Succeeded!\n");
            return true;
        }
    }

    Node(string ID, int num)
    {
        set_ChannelID(ID);
        this->Node_number = num;
        return;
    }
};

Node N1(ID_NODE1, 1);
Node N2(ID_NODE2, 2);
Node N3(ID_RASPI, 3);

// Prepare state machine
enum state
{
    init,
    scan,
    respond,
    slumber
};
state c_state = init;

void onReceive(int packetSize)
{
    c_state = respond;
    return;
}

int main()
{

    // skeleton_daemon();

    unsigned int start = millis();

    while (1)
    {
        switch (c_state)
        {
        case init:
        {
            /*******************************************************************************
             *
             * Setup Variables
             *
             *******************************************************************************/
            if (DEBUG)
            {
                cout << "\n======================================================" << endl;
                cout << "\n -  -  - -- IoT Control System: Wetlands -- -  -  - - " << endl;
                cout << "\n======================================================\n"
                     << endl;

                // Setup MQTT
                cout << " Starting MQTT Client " << endl;
            }

            // Setup Wiring Pi
            wiringPiSetup(); // Start wiring Pi

            // Start MQTT Client
            bool status = setup_MQTT();

            if (status == true)
            {
                if (DEBUG)
                {
                    cout << " MQTT Client Status : ONLINE" << endl;
                }
            }
            else
            {
                if (DEBUG)
                {
                    cout << " MQTT Client Status : OFFLINE" << endl;
                }
                break;
            }

            // Setup LoRa Communications
            // Configure Gateway
            if (DEBUG)
            {
                cout << "\n Starting LoRa Gateway" << endl;
            }
            LoRa.setPins(ssPin, RST, dio0); // Set module pins

            // Start LoRa with Freq
            if (!LoRa.begin(freq))
            {
                if (DEBUG)
                {
                    cout << "\n Starting LoRa failed!" << endl;
                }
                c_state = init;
                sleep(60);
                break;
            }
            LoRa.setSpreadingFactor(SF); // Set Spreading Factor
            // LoRa.setSignalBandwidth(bw);

            // Print Console, configuration successful
            if (DEBUG)
            {
                cout << "\n - - LoRa Configuration - - " << endl;
                cout << "  Frequency: " << freq << " Hz" << endl;
                cout << "  Bandwidth: " << bw << endl;
                cout << "  Spreading Factor : " << SF << "\n\n======================================================\n"
                     << endl;
            }

            LoRa.onReceive(onReceive);
            LoRa.receive();

            // System Configured
            c_state = slumber;
            break;
        }
        case scan:
        {
            if (LoRa.parsePacket())
            {
                c_state = respond;
            }
            break;
        }
        case respond:
        {
            // received a packet
            string message = ""; // Clear message string
            // Store Message in string Message
            while (LoRa.available())
            {
                message = message + ((char)LoRa.read());
            }

            // Store Node Value
            string node = "Error";

            for (int position = 0; position < (message.length()); position++)
            {
                if (message[position] == 'N')
                {
                    if (!(position + 4 > message.length()))
                    {
                        node = message[position + 4];
                    }
                }
            }

            if (node == "1")
            {
                N1.sendAck(message);
                N1.update_MQTT(message);
                N1.send_MQTT(client);
            }
            else if (node == "2")
            {
                N2.sendAck(message);
                N2.update_MQTT(message);
                N2.send_MQTT(client);
            }
            else
            {
                if (DEBUG)
                {
                    cout << "Error: Unknown node detected" << endl;
                }
                c_state = slumber;
                break;
            }

            c_state = slumber;
            break;
        }
        case slumber:
        {
            sleep(10);
            break;
        }
        default:
        {
            c_state = slumber;
            break;
        }
        }
        // Update CPUtemp every minute
        if((millis() - start) >= 60000){
            float temp = updateCPUTEMP();
            string payload = "field1=1&field2=" + to_string(temp);
            N3.set_payload(payload);
            N3.send_MQTT(client);
            start = millis();
        }
    }
}