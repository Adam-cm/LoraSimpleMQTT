# LoraSimpleMQTT
This program is developed for a Raspbery Pi and is intended for use with LoRa Dragino modules. It is responsible for receiving various LoRa messages and uploading them to Thingspeak using a MQTT Client. It includes data management and reliability checks on both LoRa and MQTT interfaces.

![image](https://drive.google.com/uc?export=view&id=1IyFwkndXneeQ8KbWKOkJTJl4UHYygkWe)
Figure 1: The final product.

# MQTT
This project uses an MQTT client (Paho.mqtt c library) to transmit and share information to Thingspeak. The Gateway can receive information via LoRa and then determine which MQTT subscriber the information should be sent to (which Thingspeak channel it should be sent to). The operation of the gateway can be observed below:

![image](https://drive.google.com/uc?export=view&id=1XWNL8NdCEkjr6fpAYRsgic4OdOn5ZbUE)
Figure 2: The network topology used and the role of the gateway.

# LoRa
The LoRa aspect of this project uses a single channel server to receive and interpret LoRa signals using a SX1276 Dragino module. It builds on-top of the Arduino LoRa library and converts it to something applicable on the Raspberry Pi. The LoRa communication uses an acknowledge structure utilising the RX1 window of the protocol (it will attempt to send the message 5 times if no acknowledge is received before quitting). This will also be used to send information regarding the weather station variables to the control system. The system will ultimately be capable of receiving messages, deciphering it and identifying which node the message came from, from here the program will then act accordingly. The program used for the LoRa communications was derived from [here](https://github.com/sandeepmistry/arduino-LoRa), a LoRa library intended for the Arduino environment, this was re-written to work and operate on the Raspberry Pi.

# Wiring Diagram
![image](https://drive.google.com/uc?export=view&id=1YM_XTfLAoK2r2KCQsAAmkSWHfu61cNFR)
Figure 3: Circuit diagram of the interface between Dragino shield and Raspberry Pi.

# TO DO:
- ...
# Operation/Installation Instructions
The program utilises a simple make file for both building and installing the program on the Raspberry Pi. This can be achieved by using the following functions:
```
sudo make

sudo make install LoRaSimpleMQTT
```
This builds and allows the application to run in the background of the Raspberry Pi, it also assists with reliability where if the application was to experience an error or issue it will attempt to reboot. If you do not wish for the program to run in the background you can run the application using:
```
sudo make

sudo ./LoRaSimpleMQTT
```
This will build and run the application in an single instance, this is less reliable as faults and crashes within the program will result in the program terminating and not rebooting. It does however assist in bug testing and building of the application providing a more verbose output. To monitor the application in real time while installed on the Raspberry Pi, using the first method, you can monitor the application using the following command:
```
sudo journalctl -f -u LoRaSimpleMQTT
```
This will provide information regarding the application and information passing through the Gateway.

# Limitations:
The structure of the application only uses one channel to receive information using LoRa. This means that there is a possibility of packets colliding during transmission. This could be accommodated by adding an additional Dragino module listening on a different frequency, this would allow information to be transmitted on different frequencies for different nodes. Although the GPIO on the Raspberry Pi does support this, this was not implemented in this test/proof of concept scenario.
