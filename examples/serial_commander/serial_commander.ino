/*

This code was compiled for an Arduino Opta

The ATV320 needs the following configuration, please check the documentation if needed:

- Menu CONF>FCS->FULL>CTL>CHCF = SIM : ( Profil not separed)
- Menu CONF>FCS->FULL>CTL>FR1 = nEt (ETHERNET COMMAND)
- Menu CONF>FCS->FULL>COM->Cbd->EtHM = MbtP (MODBUS)
- Menu CONF>FCS->FULL>COM->Cbd->IPM = MAnU (IP MANUAL)
- Menu CONF>FCS->FULL>COM->Cbd->IPC-> IPC1=192 IPC2=168 IPC3=1 IPC4=2 Static IP
- Menu CONF>FCS->FULL>COM->Cbd->IPM--> IPM1=255 IPM2=255 IPM3=255 IPM4=0 (IP MASK)
- Menu CONF>FCS->FULL>COM->Cbd->IOSA=On (I/O scanning)
- Menu CONF>FULL>FLT->CLL->CLL=No (Remove error about communication, this is not ideal, but that was a quick fix, if you have a better idea, do not hesitate !!)
    
  This example shows how work the serial commander

*/


#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoRS485.h>  // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>

#include <atv320_arduinoLibrary.h>

// ** EHTERNET CONFIGURATION
// ADRESSE MAC CONFIGURABLE
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 2);                // OPTA IP
IPAddress myATVip(192, 168, 1, 3);           // ATV IP Address
EthernetClient ethClient;                    // Ethernet client
ModbusTCPClient modbusTCPClient(ethClient);  // Modbus TCP client

ATV320_modbusTCP_client myATV(myATVip, &modbusTCPClient, "ATV", false, 1500);  // IP, modbus client, reverse_rotation, Speed [0-1500] rpm

// SERIAL
String inputString = "";      // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete


void setup() {
  // *** SERIAL PORT
  Serial.begin(115200);      // Init serial port
  inputString.reserve(200);  // Gestion de la mémoire des infos du ports serie

  // *** ETHERNET
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1);  // do nothing, no point running without Ethernet hardware
    }
  } else
    Serial.println("Ethernet hardware OK");
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  } else
    Serial.println("Cable hardware Connected");

  // SHOW CONFIGURATION
  Serial.print("\n");
  Serial.print("localIP:\t");
  Serial.println(Ethernet.localIP());
  Serial.print("subnetMask:\t");
  Serial.println(Ethernet.subnetMask());
  Serial.print("gatewayIP:\t");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("dnsServerIP:\t");
  Serial.println(Ethernet.dnsServerIP());
  Serial.println("\n*******************");

  // Check ATV Connexion
  bool fail = false;
  Serial.println("**** MODBUS TCP **** ");
  do {
    if (myATV.checkConnexion()) {
      Serial.println("MODBUS CONNEXION FAILED... RETRYING IN 2S");
    } else
      break;
    delay(2000);
  } while (fail);

  // Init ATV
  myATV.setup();

  Serial.println("Please send a command using this patern: [NAME COMMAND VALUE]");
  Serial.println("-NAME : The name of the ATV320 you want to control, here \"ATV\"");
  Serial.println("- COMMAND: The command to send:");
  Serial.println(" > EN : enable");
  Serial.println(" > SPEED : change speed [0 to 1500] rpm");
  Serial.println(" > REV : reverse rotation");
  Serial.println(" > ETA : informations");
  Serial.println("- VALUE : The value to send");
}

void loop() {

  // Check incomming serial event
  serialEvent();

  if (stringComplete) {

    bool command_executed = false;
    // Name detection
    int separator_index = inputString.indexOf(" ");
    int last_separator = separator_index;
    String name = inputString.substring(0, separator_index);                                                              // Get name
    separator_index = inputString.substring(last_separator + 1, inputString.length()).indexOf(" ") + last_separator + 1;  //
    String command = inputString.substring(last_separator + 1, separator_index);                                          // Get command
    String value = inputString.substring(separator_index + 1, inputString.length());                                      // Get value
    // Check objets commanders
    command_executed = myATV.commander(name, command, value);
    if (!command_executed) {
      Serial.println("COMMAND ERROR");
    }
  }
}


void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:

    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    } else
      inputString += inChar;
  }
}
