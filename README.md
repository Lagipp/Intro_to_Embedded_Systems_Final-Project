# Intro to Embedded Systems: final project, group 14
 
This the the final project of LUT University course “BL40A1812 Introduction to Embedded Systems”.

The system is a simplified model of an alarm system with a timer, motion sensor, buzzer, and a keypad to input a password with. Upon starting the program, the system waits in idle state until any movement is detected by the motion sensor. Upon triggering the sensor, the system starts the 10 second timer (not shown to the user) and waits for the user to input the correct password using the keypad. If the user enters the correct password in the 10 second timeframe, the alarm is disarmed. The user is then prompted to either rearm the system, starting the program anew, or exiting the program altogether. 

If the user either enters the wrong password or the timer counts to 10, the alarm is activated, turning the buzzer on. The user is then once again instructed to enter the correct password, but this time with no timer nor a penalty for inputting the wrong password. Upon entering the correct password successfully, the alarm is disarmed, turning the buzzer off and prompting the user to either rearm the system or exit.
 The system also features an LCD screen to give the user instructions on what to do and notifications on what the system is currently doing.

 The ATmega2560 (“Mega”) functions as the master and is responsible for most of the code and functionality for the entire system. The ATmega328p (“Uno”) is a slave, receiving instructions from the master (via SPI) on when to turn on and off the buzzer.


## Circuit diagram
![finalproject_circuit_diagram_commented](https://github.com/Lagipp/Intro_to_Embedded_Systems_Final-Project/assets/122733073/58ac3ed3-b9d1-4831-8ac4-a676ef3a2c44)


### Running the program
* Open both the `UNO/main.c` and `MEGA/main.c` using your preferred IDE (e.g. Microchip studio) in separate instances.
* Build the solutions (F7) and use a flashing tool to upload the code to the micro-controllers.
* (Optional) Open PuTTY to see a more detailed log of the system in function.
* Follow the instructions shown on the LCD. For the passcode, only numbers are valid, `*` and `#` are used for deleting characters and entering the password.
