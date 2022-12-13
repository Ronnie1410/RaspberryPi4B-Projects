Assignment 2
##################################
CSE-438 Assignment 2
Author:- Roshan Raj Kanakaiprath
ASU ID:- 1222478062
##################################

Overview
********

A simple program that uses the device drivers(PWM and GPIO) features of Raspberry Pi to drive
RGB Led in a sequence. The program also creates a kernel modulewhich dynamically loaded in to
kernel. It allows the access of CYCLE COUNT REGISTER to user.

Requirements
**********************
Linux Machine(Raspberry Pi OS)
Raspberry Pi Model 4B
GCC Compiler
Make

Compiling and Executing
**********************

Compile Steps:
   : On rasberry pi os unzip the ESP-Kanakaiprath-RoshanRaj-assgn02.zip navigate to the folder.
   : Navigate to the location on the terminal.
   : Enter command "make all" in terminal to compile the code and generate the object file with name "assignment2" and kernel object
       cycle_count_mod.ko.

Execution Steps:
   : Enter command "sudo insmod cycle_count_mod.ko" in the terminal to load the kernel object.
   : Enter your password if prompted for and press Enter Key.
   : Enter the following command in the terminal to execute the code.
   			"sudo ./assignment2"
   : Enter your password if prompted for and press Enter Key.
   : While execution is in progress try giving commands "rgb x y z" or "dist n mode ",
      where x,y and z are dutycycle for rgb channels(0 to 100) and "n" is the no. of sensor measurements required
      and "mode" is how to read elapsed time as per Assignment 2 document. 
   : Enter "exit" to stop execution.
   : Unload the kernel object by entering following command.
      ""sudo rmmod cycle_count_mod.ko"


