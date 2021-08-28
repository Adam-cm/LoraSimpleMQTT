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

#include "mqtt/client.h"

const string SERVER_ADDRESS { "tcp:mqtt3.thingspeak.com:1883" };
const string CLIENT_ID { "AD0yMgE2NSwgFBE0DAY2CAs" };
const string TOPIC { "Temp 1" };

const string PAYLOAD1 { "22.5" };

const int QOS = 1;

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

/////////////////////////////////////////////////////////////////////////////

// Example of a simple, in-memory persistence class.
//
// This is an extremely silly example, because if you want to use
// persistence, you actually need it to be out of process so that if the
// client crashes and restarts, the persistence data still exists.
//
// This is just here to show how the persistence API callbacks work. It maps
// well to key/value stores, like Redis, but only if it's on the local host,
// as it wouldn't make sense to persist data over the network, since that's
// what the MQTT client it trying to do.
//
class sample_mem_persistence : virtual public mqtt::iclient_persistence
{
	// Whether the store is open
	bool open_;

	// Use an STL map to store shared persistence pointers
	// against string keys.
	std::map<std::string, std::string> store_;

public:
	sample_mem_persistence() : open_(false) {}

	// "Open" the store
	void open(const std::string& clientId, const std::string& serverURI) override {
		std::cout << "[Opening persistence store for '" << clientId
			<< "' at '" << serverURI << "']" << std::endl;
		open_ = true;
	}

	// Close the persistent store that was previously opened.
	void close() override {
		std::cout << "[Closing persistence store.]" << std::endl;
		open_ = false;
	}

	// Clears persistence, so that it no longer contains any persisted data.
	void clear() override {
		std::cout << "[Clearing persistence store.]" << std::endl;
		store_.clear();
	}

	// Returns whether or not data is persisted using the specified key.
	bool contains_key(const std::string &key) override {
		return store_.find(key) != store_.end();
	}

	// Returns the keys in this persistent data store.
	mqtt::string_collection keys() const override {
		mqtt::string_collection ks;
		for (const auto& k : store_)
			ks.push_back(k.first);
		return ks;
	}

	// Puts the specified data into the persistent store.
	void put(const std::string& key, const std::vector<mqtt::string_view>& bufs) override {
		std::cout << "[Persisting data with key '"
			<< key << "']" << std::endl;
		std::string str;
		for (const auto& b : bufs)
			str.append(b.data(), b.size());	// += b.str();
		store_[key] = std::move(str);
	}

	// Gets the specified data out of the persistent store.
	std::string get(const std::string& key) const override {
		std::cout << "[Searching persistence for key '"
			<< key << "']" << std::endl;
		auto p = store_.find(key);
		if (p == store_.end())
			throw mqtt::persistence_exception();
		std::cout << "[Found persistence data for key '"
			<< key << "']" << std::endl;

		return p->second;
	}

	// Remove the data for the specified key.
	void remove(const std::string &key) override {
		std::cout << "[Persistence removing key '" << key << "']" << std::endl;
		auto p = store_.find(key);
		if (p == store_.end())
			throw mqtt::persistence_exception();
		store_.erase(p);
		std::cout << "[Persistence key removed '" << key << "']" << std::endl;
	}
};

/////////////////////////////////////////////////////////////////////////////
// Class to receive callbacks

class user_callback : public virtual mqtt::callback
{
	void connection_lost(const std::string& cause) override {
		std::cout << "\nConnection lost" << std::endl;
		if (!cause.empty())
			std::cout << "\tcause: " << cause << std::endl;
	}

	void delivery_complete(mqtt::delivery_token_ptr tok) override {
		std::cout << "\n\t[Delivery complete for token: "
			<< (tok ? tok->get_message_id() : -1) << "]" << std::endl;
	}

public:
};

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

    // Start MQTT Client
    sample_mem_persistence persist;
    mqtt::client client(SERVER_ADDRESS, CLIENT_ID, &persist);
    
    // Configure a callback
    user_callback cb;
	  client.set_callback(cb);

    //Define MQTT options
    mqtt::connect_options connOpts;
	  connOpts.set_keep_alive_interval(20);
	  connOpts.set_clean_session(true);

    try {
    // Connect to MQTT
    client.connect(connOpts);

    // Sening Data through MQTT
    auto pubmsg = mqtt::make_message(TOPIC, PAYLOAD1);
		pubmsg->set_qos(QOS);
		client.publish(pubmsg);

    // Delete MQTT Client
    client.disconnect();
    }
    catch (const mqtt::persistence_exception& exc) {
		  std::cerr << "Persistence Error: " << exc.what() << " ["
			<< exc.get_reason_code() << "]" << std::endl;
		  return 1;
	  }
	  catch (const mqtt::exception& exc) {
		  std::cerr << exc.what() << std::endl;
		  return 1;
	  }

    // Main Loop
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