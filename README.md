            ---- Arm-Project Control for a prosthetic arm ----

UserApp.cpp is the main entry-point of this application.

This was implemented on the Intel-Altera DE2I-150 board.

- It sets up a PCIe connection between the onboard CPU and Altera FPGA. 
Assumes a streaming method (MyoConnect proprietary software, bluetooth etc) is implemented already.

- It reads off this data and processes it with a method known as "characteristics" [Worth noting this method would not produce results at the level of a machine learning algorithm with tons of data].

- MyoFann.cpp and MyoFann.h are my Myo implementations of a popular FANN library created by a graduate student. This did not produce great results because the data set available was incredibly smaller than meaningful.

The website link provided provides a little more info about this project

Demo: 
https://www.youtube.com/watch?v=_ptuvL7I6f0
