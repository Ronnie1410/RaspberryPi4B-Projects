Assignment 3
##################################
CSE-438 Assignment 3
Author:- Roshan Raj Kanakaiprath
ASU ID:- 1222478062
##################################

Overview
********

In this assignment a linux device driver is developed to create and control two character devices.
These devices implement linux hashtable data structures in the kernel. File operations are also 
written to open, release, write and read from the devices.

Requirements
**********************
Linux Machine(Raspberry Pi OS)
Raspberry Pi Model 4B
GCC Compiler
Make

Compiling and Executing
**********************

Compile Steps:
   : On rasberry pi os unzip the ESP-Kanakaiprath-RoshanRaj-assgn03.zip navigate to the folder.
   : Navigate to the location on the terminal.
   : Enter command "make all" in terminal to compile the code and generate the object file with name "assignment3" and kernel object
       rbt438_drv.ko .

Execution Steps:
   : Enter command "sudo insmod rbt438_drv.ko" in the terminal to load the kernel object.
   : Enter your password if prompted for and press Enter Key.
   : Place the test scripts(t1 and t2) in the present working folder.
   : Enter the following command in the terminal to execute the code.
   			"sudo ./assignment3 t1 t2"
   : Enter your password if prompted for and press Enter Key.
   : Wait for execution to complete
   : Open and analyze t1_out and t2_out files.
   : Unload the kernel object by entering following command.
      ""sudo rmmod rbt438_drv.ko"


