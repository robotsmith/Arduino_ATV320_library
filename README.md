# Arduino_ATV320_library
This Arduino library (tested on OPTA) is dedicated to control the ATV320 from SE with Modbus TCP


## DISCLAMER
This hardware/software is provided "as is", and you use the hardware/software at your own risk. 
Under no circumstances shall any author be liable for direct, indirect, special, incidental, 
or consequential damages resulting from the use, misuse, or inability to use this hardware/software, 
even if the authors have been advised of the possibility of such damages.

This library is just a share of my work, ATV320 is used on quite big motors, it can be very dangerous, so please be cautious when using 
this library. Use it at your own risk.


## DOCUMENTATION
Please check the ATV320 documentation on the SE website.

## ATV CONFIGURATION
### ATV320 CONFIGURATION 
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

### SERIAL MONITOR COMMANDS
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
## Examples and explanations
Please check the examples for more informations. You can also check the .h or .cpp

## Note
This is a WIP, this is absolutely not perfect but should globaly work.

## Author
RobotSmith 2024
