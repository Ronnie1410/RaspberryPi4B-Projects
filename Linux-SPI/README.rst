Assignment 4
##################################
CSE-438 Assignment 4
Author:- Roshan Raj Kanakaiprath
ASU ID:- 1222478062
##################################

Overview
********

A simple program which implements a scrolling application on led matrix based on measurement 
from hcsr-04 driver. A character device file is written to control the operations of the distance 
sensor. The led matrix is controlled in the user application via the SPI interface on raspberry pi. 

Requirements
**********************
Linux Machine(Raspberry Pi OS)
Raspberry Pi Model 4B
GCC Compiler
Make

Compiling and Executing
**********************

Compile Steps:
   : On rasberry pi os unzip the ESP-Kanakaiprath-RoshanRaj-assgn04.zip navigate to the folder.
   : Navigate to the location on the terminal.
   : Enter command "make all" in terminal to compile the code and generate the object file with name "assignment4" and kernel object
       hcsr04_drv.ko

Execution Steps:
   : Enter command "sudo insmod hcsr04_drv.ko" in the terminal to load the kernel object.
   : Enter your password if prompted for and press Enter Key.
   : Enter the following command in the terminal to execute the code.
   			"sudo ./assignment4"
   : Enter your password if prompted for and press Enter Key.
   : While execution is in progress try giving commands "b" or "(0 to 8)" and press ENTER, 
   : Press Ctrl + C to stop execution.
   : Unload the kernel object by entering following command.
      ""sudo rmmod hcsr04_drv.ko"
   : Enter command "make clean" on the terminal to remove unwanted files


