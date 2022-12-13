Assignment 1
##################################
CSE-438 Assignment 1
Author:- Roshan Raj Kanakaiprath
ASU ID:- 1222478062
##################################

Overview
********

A simple program that demonstartes the concepts of real-time
scheduling of periodic tasks and aperiodic tasks, conditional variables and
resource sharing using a multi-threaded program.

Requirements
**********************
Linux Machine
GCC Compiler
Make

Compiling and Executing
**********************

Compile Steps:
   : On Linux Terminal navigate to project folder.
   : In thread_model.h, make sure the macro KEYBOARD_EVENT_DEV is defined as "dev/input/eventx" as per your system
   : Enter command "make" on terminal to compile the code and generate the object file with name "assignment1"

Execution Steps:
   : Enter the following command and make sure root asks for password
   			sudo ./assignment1
   : Enter your system password and press Enter
   : While execution is in progress try pressing keys 0 to 9 on keyboard
   : Ensure execution terminates after printing "Exiting from main"
   : Real time tracing can be visualized usig Kernel shark using the following command on terminal
   	
   		"sudo trace-cmd record -esched_switch taskset -c 3 ./assignment1"


Sample Output
=============

Exiting from from main

