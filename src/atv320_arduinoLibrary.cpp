#include "atv320_arduinoLibrary.h"
/*****************************************************
  CONSTRUCTEUR
*****************************************************/
ATV320_modbusTCP_client::ATV320_modbusTCP_client(IPAddress ip_, ModbusTCPClient* modbusTCPClient_, String name_, bool sens, long speed_) {
  ip = ip_;      // Add IP
  name = name_;  // Ajouter le nom

  // Ajouter le client TCP
  modbusTCPClient = modbusTCPClient_;

  // Parametres
  sens_moteur = sens;
  speed = speed_;
}

/*****************************************************
  Connect
 Check connexion True => error
*****************************************************/
bool ATV320_modbusTCP_client::connect(bool verbose) {  // Verifie la connexion : true -> erreur
  if (!modbusTCPClient->connected()) {
    // client not connected, start the Modbus TCP client
    //Serial.println(name + ": Attempting to connect to Modbus TCP server");

    if (!modbusTCPClient->begin(ip, TCP_PORT)) {
      Serial.println(name + ": FAIL TO CONNECT");
      // Serial.println(name + ":" + modbusTCPClient->lastError());
      return true;
    }
  }

  if (verbose) {
    Serial.println(name + ": Modbus TCP Client connected");
  }


  return false;
}

/*****************************************************
  Disconnect
  Check connexion True => error
*****************************************************/
bool ATV320_modbusTCP_client::disconnect(bool verbose) {  // Verifie la connexion : true -> erreur


  modbusTCPClient->stop();
  return false;
}
/*****************************************************
 Verifier la connexion
 Check connexion True => error
*****************************************************/
bool ATV320_modbusTCP_client::checkConnexion(bool verbose) {  // Verifie la connexion : true -> erreur
  connect(verbose);                                           // Connect
  disconnect();                                               // Stop connection
  return false;
}
/*****************************************************
  Effacement des defauts
  @arg closeConnexion : open and close TCP connexion after the command
  @return true : fail
*****************************************************/
bool ATV320_modbusTCP_client::cleanDefault(bool closeConnexion) {
  if (closeConnexion && connect()) {
    Serial.println(name + " : clean default impossible (erreur de connexion)");
    if (closeConnexion)
      disconnect();
    return true;  // Connexion error
  }
  if (!modbusTCPClient->holdingRegisterWrite(0, REGISTER_ADDRESS_CMD, 0x080)) {
    Serial.println(name + ": suppression defaut impossible (write [CMD] register failed)");
    if (closeConnexion)
      disconnect();
    return true;
  }
  if (closeConnexion)
    disconnect();
  return false;
}

/*****************************************************
  Configuration du variateur pour passer en mode RDY
  @arg closeConnexion : open and close TCP connexion after the command
  @return true : fail
*****************************************************/
bool ATV320_modbusTCP_client::setup(bool closeConnexion) {

  if (connect()) {
    Serial.println(name + " : setup impossible (erreur de connexion)");
    disconnect();
    return true;  // Connexion error
  }

  setSpeed(speed, false);

  // refresh ETA
  if (refreshETA(false)) {
    Serial.println(name + " :  setup impossible (connexion ETA)");
    disconnect();
    return true;
  }

  // Si defaut ou alarm abandonner
  if (register_ETA.bit.defaut) {
    Serial.println(name + " :  setup (DEFAUT)");
    delay(100);
    // Envoyer l'effacement de defaut
    Serial.println(name + " : Effacement defaut");
    cleanDefault(false);
    delay(250);
    //return true;
  }
  if (register_ETA.bit.alarme) {
    Serial.println(name + " :  setup impossible (ALARM)");
    disconnect();
    return true;
  }

  /*Serial.println("=================");
  showETA(false);
  Serial.println("=================");*/
  delay(250);

  // Si en NLP, le faire passer en NLP ou RDY
  if (register_ETA.bit.mise_en_service_desactive) {

    // delay(100);
    // Envoyer la commande shutdown
    //Serial.println("ENVOI CMD 0x6");
    if (!modbusTCPClient->holdingRegisterWrite(0, REGISTER_ADDRESS_CMD, 0x6)) {
      Serial.println(name + ":   setup impossible (write [CMD] register failed)");
      disconnect();
      return true;
    }

    delay(100);
    // refresh ETA
    if (refreshETA(false)) {
      Serial.println(name + " :  setup impossible (connexion ETA)");
      disconnect();
      return true;
    }
  }



  // delay(250);
  /*Serial.println("=================");
  showETA();
  Serial.println("=================");*/
  // Doit être mis dans l'état sous tension
  if (register_ETA.bit.pret_au_service && register_ETA.bit.arret_rapide) {
    // delay(100);
    //Serial.println("ENVOI CMD 0x7");
    // Envoyer commande Switch ON
    if (!modbusTCPClient->holdingRegisterWrite(0, REGISTER_ADDRESS_CMD, 0x7)) {
      Serial.println(name + ":   setup impossible (write [CMD] register failed)");
      disconnect();
      return true;
    }

    // delay(250);
    // refresh ETA
    if (refreshETA(false)) {
      Serial.println(name + " :  setup impossible (connexion ETA)");
      disconnect();
      return true;
    }

    if (register_ETA.bit.pret_au_service
        && register_ETA.bit.mis_en_service
        && register_ETA.bit.tension_active
        && register_ETA.bit.arret_rapide) {
      Serial.println(name + " : RDY");

      if (closeConnexion)
        disconnect();
      return false;  // => OK RDY
    } else if (register_ETA.bit.pret_au_service
               && !register_ETA.bit.mis_en_service
               && !register_ETA.bit.tension_active
               && register_ETA.bit.arret_rapide) {
      Serial.println(name + " : NLP : HORS TENSION)");
      disconnect();
      return true;
    } else {

      Serial.println(name + " :  setup impossible (moteur non RDY)\nETA:" + String(register_ETA.ETA, HEX));
      showETA(false);
      Serial.println("*****************");
      disconnect();
      return true;
    }

  } else if (!register_ETA.bit.pret_au_service || !register_ETA.bit.arret_rapide) {
    Serial.println(name + " :  setup impossible (Etat ni NLP ni RDY)");
    disconnect();
    return true;
  }

  Serial.println(name + " :  setup impossible ");
  disconnect();
  return true;
}



/*****************************************************
  Turn the motor ON/OFF
  @arg enable : true = turn on the motor / false = off
  @arg clean_reverse : true = clean reverse flag
  @arg closeConnexion : open and close TCP connexion after the command
  @return bool true -> fail
*****************************************************/
bool ATV320_modbusTCP_client::activate(bool enable, bool clean_reverse, bool closeConnexion) {

  if (clean_reverse) {
    reverse_actif = false;
  }
  // Check connexion
  if (closeConnexion && connect()) {
    Serial.println(name + " : setup impossible (erreur de connexion)");
    disconnect();
    return true;  // Connexion error
  }

  // refresh ETA
  if (refreshETA(false)) {
    Serial.println(name + " :  setup impossible (connexion ETA)");
    disconnect();
    return true;
  }


  if (!register_ETA.bit.tension_active) {
    Serial.println(name + ": PAS DE TENSION SECTEUR");
    disconnect();
    return true;
  }

  long value = 0;

  if (enable) {

    // Check if RDY
    unsigned int retry = POWER_ON_RETRY;
    do {


      if (register_ETA.bit.pret_au_service
          && register_ETA.bit.mis_en_service
          && register_ETA.bit.arret_rapide
          && !register_ETA.bit.defaut) {
        break;  // Sortie de boucle
      } else if (retry == 0) {
        Serial.println(name + " : (driver non RDY)..retry failed");
        disconnect();
        return true;
      }
      Serial.println(name + ": REINIT");
      setup(false);  // Reinit variateur
      // delay(250);
      // setSpeed(speed, closeConnexion);  // Envoi la vitesse sauvegardée
    } while (retry-- > 0);


    value = 0xF;  // Marche
  } else
    value = 0x07;  // arret

  // Envoi de la commande
  //Serial.println(name+":BEF:"+modbusTCPClient->lastError());


  //  if (!modbusTCPClient->holdingRegisterWrite(0, 7132, 0)) {
  //   Serial.println(name + ": write [CMD] register failed");
  //   Serial.println(name + ":" + modbusTCPClient->lastError());
  //   disconnect();
  //   return true;
  // }
  // delay(100);
  if (!modbusTCPClient->holdingRegisterWrite(0, REGISTER_ADDRESS_CMD, value)) {
    Serial.println(name + ": write [CMD] register failed");
    Serial.println(name + ":" + modbusTCPClient->lastError());
    disconnect();
    return true;
  }

  if (enable) {
    Serial.println(name + ": ACTIVE");
  } else {
    Serial.println(name + ": DESACTIVE");
  }

  disconnect();
  return false;
}
/*****************************************************
  Speed config
  @arg speed: [0 - 1500] rpm
  @arg closeConnexion : open and close TCP connexion after the command
  @return bool true -> fail
*****************************************************/
bool ATV320_modbusTCP_client::setSpeed(long speed_, bool closeConnexion) {

  speed = abs(speed_);
  if (closeConnexion && connect()) {
    disconnect();
    return 1;  // Connexion error
  }

  if (!modbusTCPClient->holdingRegisterWrite(0, REGISTER_ADDRESS_FREQ, (reverse_actif ? -1 : 1) * (sens_moteur ? -1 : 1) * speed)) {
    Serial.println(name + ": write [SPEED] register failed");
    if (closeConnexion)
      disconnect();
    return true;
  }
  if (closeConnexion)
    disconnect();
  return false;
}
/*****************************************************
  Get Speed stored in the module
  @return vitesse
*****************************************************/
long ATV320_modbusTCP_client::getSpeed() {
  return speed;
}
/*****************************************************
  INVERSION MOTEUR
  @arg reverse : true = reverse rotation
  @arg closeConnexion : open and close TCP connexion after the command
  @return fail : true
*****************************************************/
bool ATV320_modbusTCP_client::setReverse(bool reverse, bool closeConnexion) {
  if (reverse == reverse_actif)
    return false;           // OK rien a faire
  reverse_actif = reverse;  // Changement de flag reverse

  return setSpeed(speed, closeConnexion);  // Changement vitesse pour inversion
}
/*****************************************************
  Refresh state
  @arg closeConnexion : open and close TCP connexion after the command
  @return bool true -> fail
*****************************************************/
bool ATV320_modbusTCP_client::refreshETA(bool closeConnexion) {
  if (closeConnexion && connect()) {
    disconnect();
    return 1;  // Connexion error
  }
  long register_ = 0;

  unsigned int retry = 3;
  do {
    register_ = modbusTCPClient->holdingRegisterRead(0, 0x31C5);
    if (register_ == -1) {
      cleanDefault(closeConnexion);
    } else
      break;


  } while (retry-- > 0);
  if (retry == 0) {
    Serial.println(name + ": read [ETA] register failed");
    if (closeConnexion)
      disconnect();
    return true;  // Error com
  }

  register_ETA.ETA = register_;
  if (closeConnexion)
    disconnect();
  return 0;
}
/*****************************************************
  Get ETA
  @arg closeConnexion : open and close TCP connexion after the command
  @return long vecteur ETA (16 bits)
*****************************************************/
long ATV320_modbusTCP_client::getETA(bool closeConnexion) {
  if (!refreshETA(closeConnexion))
    return (long int)register_ETA.ETA;
  return -1;
}
/*****************************************************
  Show ETA short
  @arg closeConnexion : open and close TCP connexion after the command
*****************************************************/
void ATV320_modbusTCP_client::showETA(bool closeConnexion) {

  Serial.print(name + " - ETA:[");
  if (!refreshETA(closeConnexion)) {
    if (register_ETA.bit.pret_au_service) {
      Serial.print("PRET_SERV ");
    }
    if (register_ETA.bit.mis_en_service) {
      Serial.print("READY ");
    }
    if (register_ETA.bit.fonctionnement_actif) {
      Serial.print("EN_FONCTION ");
    }
    if (register_ETA.bit.defaut) {
      Serial.print("DEFAUT! ");
    }
    if (!register_ETA.bit.tension_active) {
      Serial.print("HS_TENSION ");
    } else
      Serial.print("SS_TENSION ");

    if (register_ETA.bit.arret_rapide) {
      Serial.print("ARR_RAPIDE ");
    }
    if (register_ETA.bit.mise_en_service_desactive) {
      Serial.print("ALIM_VERR ");
    }
    if (register_ETA.bit.alarme) {
      Serial.print("ALARM! ");
    }
    if (register_ETA.bit.remote) {
      Serial.print("CMD_DIST ");
    }
    if (register_ETA.bit.consigne_ateinte) {
      Serial.print("CONS_ATT ");
    }
    if (!register_ETA.bit.limite_interne_active) {
      Serial.print("CON_H_LIM ");
    }
    if (register_ETA.bit.touche_stop) {
      Serial.print("ARRET_IHM ");
    }
    if (!register_ETA.bit.sens_rotation) {
      Serial.print("AVANT");
    }
    if (register_ETA.bit.sens_rotation) {
      Serial.print("ARRIERE");
    }
    Serial.println("]");
  }
}


/*****************************************************
  Show ETA detailed
  @arg closeConnexion : open and close TCP connexion after the command
*****************************************************/
void ATV320_modbusTCP_client::showETAdetail(bool closeConnexion) {
  if (!refreshETA(closeConnexion)) {
    if (register_ETA.bit.pret_au_service) {
      Serial.println(name + " : PRET AU SERVICE");
    }
    if (register_ETA.bit.mis_en_service) {
      Serial.println(name + " : PRET");
    }
    if (register_ETA.bit.fonctionnement_actif) {
      Serial.println(name + " : EN FONCTIONNEMENT");
    }
    if (register_ETA.bit.defaut) {
      Serial.println(name + " : DEFAUT DETECTE!");
    }
    if (!register_ETA.bit.tension_active) {
      Serial.println(name + " : HORS TENSION");
    } else
      Serial.println(name + " : EN TENSION");

    if (register_ETA.bit.arret_rapide) {
      Serial.println(name + " : ARRET RAPIDE");
    }
    if (register_ETA.bit.mise_en_service_desactive) {
      Serial.println(name + " : ALIM VERROUILLEE");
    }
    if (register_ETA.bit.alarme) {
      Serial.println(name + " : ALARME!");
    }
    if (register_ETA.bit.remote) {
      Serial.println(name + " : CMD A DISTANCE");
    }
    if (register_ETA.bit.consigne_ateinte) {
      Serial.println(name + " : CONSIGNE ATEINTE");
    }
    if (!register_ETA.bit.limite_interne_active) {
      Serial.println(name + " : CONSIGNE HORS LIMITE");
    }
    if (register_ETA.bit.touche_stop) {
      Serial.println(name + " : ARRET IHM DEMANDE");
    }
    if (!register_ETA.bit.sens_rotation) {
      Serial.println(name + " : MARCHE AVANT");
    }
    if (register_ETA.bit.sens_rotation) {
      Serial.println(name + " : MARCHE ARRIERE");
    }
  }
}
/*****************************************************
  Show CNF register (communication error from ethernet board)
  @arg closeConnexion : open and close TCP connexion after the command
  @return fail = true
*****************************************************/
bool ATV320_modbusTCP_client::showCNFRegister(bool closeConnexion) {
  if (closeConnexion && connect()) {
    disconnect();
    return 1;  // Connexion error
  }
  /// READ CNF
  long register_ = modbusTCPClient->holdingRegisterRead(0, 0x1BDC);
  if (register_ == -1) {
    Serial.println(name + ": read [CNF] register failed");
    if (closeConnexion)
      disconnect();
    return true;
  }
  Serial.println(name + " - CNF REG:" + String(register_, HEX));
  if (closeConnexion)
    disconnect();
  return false;
}
/*****************************************************
  Commander : command the ATV with the serial monitor 
  @arg name_ : react if the name is correct
  @arg command : command to send
  @arg value : value to send
  @return fail = true
*****************************************************/
bool ATV320_modbusTCP_client::commander(String name_, String command, String value) {

  // Skip bad name
  if (name_ != name)
    return true;


  int value_int = value.toInt();
  if (command == "EN" && (value_int == 0 || value_int == 1)) {
    activate(value_int, false);
  } else if ((command == "SPEED") && (value_int >= -1500) && (value_int <= 1500)) {
    if (!setSpeed(value_int)) {
      Serial.println(name + " : SPEED=" + value_int);
    }
  } else if (command == "REV") {
    reverse_actif = value_int % 2;
  } else if (command == "CNF") {
    showCNFRegister();
  } else if (command == "ETA") {
    showETA();
  }

  return false;
}