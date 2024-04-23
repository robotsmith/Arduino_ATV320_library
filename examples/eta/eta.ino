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
    

  It is an example to show the information in the ETA vector of the ATV320
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


void setup() {
  // *** SERIAL PORT
  Serial.begin(115200);  // Init serial port

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

  //Show init ETA
  Serial.println("ETA BEFORE INIT");
  myATV.showETA();

  // Init ATV
  myATV.setup();

  Serial.println("SETUP DONE");
  delay(2000);
}

void loop() {
  Serial.println("Run forward...");
  myATV.activate(true);
  myATV.showETA();
  delay(5000);
  Serial.println("Run backward...");
  myATV.setReverse(true);
  myATV.showETA();
  delay(5000);
  Serial.println("Stop...");
  myATV.activate(false);
  myATV.showETA();
  delay(5000);
  Serial.println("Changing speed...750rpm");
  myATV.setSpeed(750, false);  // false, to not close connexion because we use another command just after this one. Otherwise we loose time.
  myATV.activate(true);
  myATV.showETA();
  delay(5000);
  Serial.println("Changing speed...1500rpm");
  myATV.setSpeed(1500);
  myATV.showETA();
}
