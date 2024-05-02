# Intro_to_Embedded_Systems_Final-Project
 
This the the final project of LUT University course “BL40A1812 Introduction to Embedded Systems”.

The system is a simplified model of an alarm system with a timer, motion sensor, buzzer, and a keypad to input a password with. Upon starting the program, the system waits in idle state until any movement is detected by the motion sensor. Upon triggering the sensor, the system starts the 10 second timer (not shown to the user) and waits for the user to input the correct password using the keypad. If the user enters the correct password in the 10 second timeframe, the alarm is disarmed. The user is then prompted to either rearm the system, starting the program anew, or exiting the program altogether. 

If the user either enters the wrong password or the timer counts to 10, the alarm is activated, turning the buzzer on. The user is then once again instructed to enter the correct password, but this time with no timer nor a penalty for inputting the wrong password. Upon entering the correct password successfully, the alarm is disarmed, turning the buzzer off and prompting the user to either rearm the system or exit.
 The system also features an LCD screen to give the user instructions on what to do and notifications on what the system is currently doing.

 The ATmega2560 (“Mega”) functions as the master and is responsible for most of the code and functionality for the entire system. The ATmega328p (“Uno”) is a slave, receiving instructions from the master (via SPI) on when to turn on and off the buzzer.
