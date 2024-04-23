/*

# DISCLAMER
This hardware/software is provided "as is", and you use the hardware/software at your own risk. 
Under no circumstances shall any author be liable for direct, indirect, special, incidental, 
or consequential damages resulting from the use, misuse, or inability to use this hardware/software, 
even if the authors have been advised of the possibility of such damages.

This library is just a share of my work, ATV320 is used on quite big motors, it can be very dangerous, so please be cautious when using 
this library. Use it at your own risk.

# CONFIGURATION
## ATV320 CONFIGURATION 
Check if the subboard ModbusTCP VW3A3616 is connected to the network and with ethernet led blinking.

The ATV320 needs the following configuration, please check the documentation if needed:

- Menu CONF>FCS->FULL>CTL>CHCF = SIM : ( Profil not separed)
- Menu CONF>FCS->FULL>CTL>FR1 = nEt (ETHERNET COMMAND)
- Menu CONF>FCS->FULL>COM->Cbd->EtHM = MbtP (MODBUS)
- Menu CONF>FCS->FULL>COM->Cbd->IPM = MAnU (IP MANUAL)
- Menu CONF>FCS->FULL>COM->Cbd->IPC-> IPC1=192 IPC2=168 IPC3=1 IPC4=23 (for example) / Static IP
- Menu CONF>FCS->FULL>COM->Cbd->IPM--> IPM1=255 IPM2=255 IPM3=255 IPM4=0 (IP MASK)
- Menu CONF>FCS->FULL>COM->Cbd->IOSA=On (I/O scanning)
- Menu CONF>FULL>FLT->CLL->CLL=No (Remove error about communication, this is not ideal, but that was a quick fix, if you have a better idea, do not hesitate !!)
    
Reboot the controller to apply the configuration and be sure it was saved.

## SERIAL MONITOR COMMANDS
This commands are used to control the atv320 with the serial monitor.
It follows a simple logic: [NAME_OF_ATV320 COMMAND VALUE]

- NAME : The name of the ATV320 you want to control
- COMMAND: The command to send:
    - "EN" : enable 
    - "SPEED" : change speed \[0 to 1500\] rpm
    - "REV" : reverse rotation
    - "ETA" : informations 
- VALUE : The value to send  
    example : "VV1DEP EN 1" 


*/


#ifndef __ATV320_ARDUINO_LIBRARY_H__
#define __ATV320_ARDUINO_LIBRARY_H__
#pragma once
#include <Arduino.h>
#include <Ethernet.h>
#include <ArduinoRS485.h>  // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>
#define TCP_PORT 502

#define REGISTER_ADDRESS_CMD 0x31D9   // Address of IOscanner for CMD (write)
#define REGISTER_ADDRESS_FREQ 0x31DA  // Address of IOscanner for the speed (read)

#define POWER_ON_RETRY 3  // Nb of atempt if the motor doesn't run directly

// INFORMATION D'ETAT (ETA) contenu dans la discussion avec le variateur ATV320
typedef union {
  uint16_t ETA;
  struct {
    bool pret_au_service : 1;            // 0 , « Prêt à mettre en service », en attente de l'alimentation puissance
    bool mis_en_service : 1;             // 1 , pret
    bool fonctionnement_actif : 1;       // 2 , « Fonctionnement activé », en fonctionnement
    bool defaut : 1;                     // 3 , « Défaut » = 0 : pas de défaut = 1 : défaut
    bool tension_active : 1;             // 4 , « Tension activée », alimentation puissance présente = 0 : alimentation puissance absente = 1 : alimentation puissance présente
    bool arret_rapide : 1;               // 5 , arrêt rapide/arrêt d'urgence
    bool mise_en_service_desactive : 1;  // 6 , alimentation puissance verrouillée
    bool alarme : 1;                     // 7 , = 0 : sans alarme = 1 : alarme
    bool reserve8 : 1;                   // 8 , = 0
    bool remote : 1;                     // 9 , commande ou consigne via le réseau  = 0 : commande ou consigne via le terminal graphique ou le terminal déporté  = 1 : commande ou consigne via le réseau
    bool consigne_ateinte : 1;           // 10 , = 0 : consigne non atteinte = 1 : consigne atteinte
    bool limite_interne_active : 1;      // 11 , « Limite interne active », consigne en dehors des limites = 0 : consigne dans les limites = 1 : consigne en dehors des limites . Lorsque le variateur est en mode vitesse, les limites sont définies par les paramètres [Petite vitesse] (LSP) et [Grande vitesse] (HSP)
    bool reserve12 : 1;                  // 12 , Bits 12 et 13 : réservés (= 0)
    bool reserve13 : 1;                  // 13 , Bits 12 et 13 : réservés (= 0)
    bool touche_stop : 1;                // 14 , Arrêt par l'intermédiaire de la touche d'arrêt = 0 : aucun appui sur la touche STOP = 1 : arrêt déclenché par l'appui sur la touche STOP du terminal graphique ou du terminal déporté
    bool sens_rotation : 1;              // 15, Bit 15 : « Sens », sens de rotation = 0 : rotation dans le sens avant au niveau de la sortie = 1 : rotation dans le sens arrière au niveau de la sortie
  } bit;
} ETA_TU;

/*****************************************************
  CLASSE VARIATEUR ATV320 POUR MODBUS TCP
*****************************************************/
class ATV320_modbusTCP_client {
public:
  ATV320_modbusTCP_client(IPAddress ip_, ModbusTCPClient *modbusTCPClient_, String name_, bool sens = false, long speed = 1500);  // Constructor

  // Methodes
  bool connect(bool verbose = false);                                                // Connect the TCP client
  bool disconnect(bool verbose = false);                                             // Disconnect the TCP client
  bool checkConnexion(bool verbose = false);                                         // Verifie la connexion : true -> erreur
  bool cleanDefault(bool closeConnexion = true);                                     // Effacement des defauts
  bool setup(bool closeConnexion = true);                                            // Configuration du variateur pour passer en mode RDY
  bool activate(bool enable, bool clean_reverse = true, bool closeConnexion = true);  // Start motor
  bool setSpeed(long speed_, bool closeConnexion = true);                            // Configuration de la vitesse
  long getSpeed();                                                                   // getter de la vitesse dans arduino
  bool setReverse(bool reverse, bool closeConnexion = true);                         // CHanger de sens

  //*** FLAGS ET ETA *** 
  bool refreshETA(bool closeConnexion = true);       // Mis à jour locale du vecteur Etat ETA
  long getETA(bool closeConnexion = true);           // Renvoi le vecteur ETA
  void showETA(bool closeConnexion = true);          // Affiche ETA en clair sur une ligne
  void showETAdetail(bool closeConnexion = true);    // Affiche ETA en clair en détail
  bool showCNFRegister(bool closeConnexion = true);  // Affiche le vecteur CNF

  // Commander
  bool commander(String name_, String command, String value);
private:
  IPAddress ip;
  String name;

  ModbusTCPClient *modbusTCPClient;
  ETA_TU register_ETA;
  unsigned int speed;
  bool sens_moteur;    // Sans du moteur absolu
  bool reverse_actif;  // Marche arriere temporaire
};


#endif