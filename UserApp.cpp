// ****************************************************************************************************************************
// Entry point for Terminator Arm UserApplication 
// By: 
// Howard University EECE Terminator Arm Senior Design 2015/2016.
// 

// Dependencies:
// 1. MyoConnect/other BLE protocol implementation
// 2. PCIE library

// Description:
// This is used in a larger system. This module provides EMG data streaming and gesture recognition based on 7 pre-defined gesture states
// EMG streaming is only supported for one Myo at a time
// Terminator Myo MAC address: db-fa-0c-69-14-78 (Can be used alternatively instead of MyoConnect to attach our Myo to a Hub)
// http://diagnostics.myo.com/ provides real-time Myo diagnostic data (must be connected in MyoConnect on a pc) 
// NB: Replacing system("cls") is recommended and advised.
// ****************************************************************************************************************************

#include <iostream>
#include <stdio.h>
#include <math.h>
#include <string>
#include <iomanip>
#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>
#include <fstream>
#include "windows.h"   // contains Sleep library
#include <time.h>
#include <process.h>
#include "PCIE.h"      // PCIe libraries
#include <memory.h>
//#include <ios>
//#include <cmath>

#include <myo/myo.hpp>             // Myo default header library
	
#define DEMO_PCIE_USER_BAR			PCIE_BAR0
#define DEMO_PCIE_IO_LED_ADDR		0x00
#define DEMO_PCIE_IO_BUTTON_ADDR	0x20
#define DEMO_PCIE_FIFO_WRITE_ADDR	0x40
#define DEMO_PCIE_FIFO_STATUS_ADDR	0x60
#define DEMO_PCIE_FIFO_READ_ADDR	0x80
#define DEMO_PCIE_FIFO_GPIO_ADDR    0x100
#define DEMO_PCIE_MEM_ADDR			0x20000

#define MEM_SIZE			(128*1024) //128KB
#define FIFO_SIZE			(16*1024) // 2KBx8

// DataCollector class inheriting member functions from myo::DeviceListener
class DataCollector : public myo::DeviceListener {
public:
	// Device listener functions
	DataCollector() : onArm(true), emgSamples()
	{}

	void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection, float rotation, myo::WarmupState warmupState)
	{
	system("cls"); //std::cout << "ARM SYNC!!" << std::endl;
	onArm = true;
	whichArm = arm;
	}
	// onArmUnsync() is called whenever HUTerminator Myo has detected that it was moved from a stable position on a person's arm after
	// it recognized the arm. Typically this happens when someone takes it off of their arm, but it can also happen
	// when it's moved around on the arm.
	void onArmUnsync(myo::Myo* myo, uint64_t timestamp)
	{
		system("cls"); std::cout << "\n\n\n\n\t\t\t\t\t\tTerminator Myo UN-SYNC!!" << std::endl;
		onArm = false;
		Sleep(5000);
	}

	// onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user
	void onUnpair(myo::Myo* myo, uint64_t timestamp)
	{
		// We've lost our Myo.
		// Zeros are seen in output file if there was a disconnect so test can be repeated
		emgSamples.fill(0);
	}

	// onEmgData() is called whenever a paired Myo has provided new EMG data, and EMG streaming is enabled
	// Classifier output data
	void onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg)
	{
		for (int i = 0; i < 8; i++) {
			emgSamples[i] = emg[i];
		}
	}
	
	// There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData()
	// For this application, the functions overridden above are sufficient
	
	// The values of this array is set by onEmgData() above
	std::array<int8_t, 8> emgSamples;
	int no_testDatalines = 30, DataArray[240], no_lines = 0;    // ** SET the desired no. of training & test data lines here. multiply by 8 for dataArray **
	
	std::string update = "*REST/RELAX* position";
	std::fstream DataFile;
	std::string Run_filepath = "C:\\Users\\Terminator\\Documents\\Visual Studio 2013\\Projects\\fann-master\\fann-master\\examples\\RuntimeData.txt";
	double gesture_label, lastdataval;
	bool NewLineflag = 0, title_flag = 0;

	// We define this function to write the current values that were updated by the on...() functions above
	void writeRunData()
	{
		NewLineflag = 0; no_lines = 0;
		while (no_lines != no_testDatalines){       // Check to ensure we're only writing how many data lines we need from the hub
			// Create and open outfile 
			DataFile.open(Run_filepath, std::fstream::app);

			// write out the EMG classifier data
			for (size_t i = 0; i < emgSamples.size(); i++) {
				std::ostringstream oss;
				oss << static_cast<int>(emgSamples[i]);        // convert 8-bit array into int
				std::string emgString = oss.str();

				if (NewLineflag == 1){
					DataFile << "\n";
					NewLineflag = 0;
				}

				// Write to outfile
				DataFile << emgString << " ";
			}
			NewLineflag = 1; no_lines++;
			std::cout << std::flush;
			DataFile.close();       // flush buffer to outfile
		}

        /* Use array mode to write data, here!
		int j = 0;    // Reset Array starting write-index

		while (j < no_testDatalines*emgSamples.size()){
		// write out the EMG classifier data
		for (size_t i = 0; i < emgSamples.size(); i++) {
		std::ostringstream oss;
		oss << static_cast<int>(emgSamples[i]);        // convert 8-bit array into int
		std::string emgString = oss.str();
		DataArray[j] = static_cast<int>(emgSamples[i]); j++;
		}
		}*/	
	}

	// These values are set by onArmSync() and onArmUnsync() above.
	bool onArm;
	myo::Arm whichArm;
};

class GestureRec {
public:
	std::ifstream RunData;
	std::string gesture;
	static const int datasize = 240;
	int input_Char[datasize];

	void ParseData(std::string GR_method)
	{
		RunData.open("C:\\Users\\Terminator\\Documents\\Visual Studio 2013\\Projects\\fann-master\\fann-master\\examples\\RuntimeData.txt");
		
		// Grab test data 
		for (int i = 0; i < datasize; i++){
			RunData >> input_Char[i];           //Here, using .txt files           // input_Net[i] = Data[i]; //(Using Arrays)
			if (input_Char[i] < 0) input_Char[i] = sqrt(input_Char[i] * input_Char[i]);     // compute absolute values 
		}

		// Delete RuntimeData.txt
		RunData.close();
		Sleep(2000);
		std::remove("C:\\Users\\Terminator\\Documents\\Visual Studio 2013\\Projects\\fann-master\\fann-master\\examples\\RuntimeData.txt");
	}

	int Translate_Characteristics(int Data[datasize])
	{
		// Function-specific Variables
		//const int dataSize = 240;
		const int no_pods = 8; std::string gesture = "Rest";

		float Rest = 0.0, Thumb = 0.0, Index = 0.0, Middle = 0.0, Ring = 0.0, Pinky = 0.0, Wrist = 0.0;
		int Pod0 = 0, Pod1 = 0, Pod2 = 0, Pod3 = 0, Pod4 = 0, Pod5 = 0, Pod6 = 0, Pod7 = 0;
		int checkvalues[] = { 4, 5, 6, 7, 8 };
		float Percents[5][no_pods]; float finalvalue[7];

		// GR Approach
		// 2. Compute percentages for values between 4 through 8
		for (int i = 0; i < sizeof(checkvalues) / sizeof(checkvalues[0]); i++){
			for (int j = 0; j < datasize; j++){            // Score no_times data is less than 8
				if (input_Char[j] < checkvalues[i]){
					if (j == 0 || j % 8 == 0)  Pod0++;                 // Pod 0
					else if (j == 1 || (j - 1) % 8 == 0)  Pod1++;        // Pod 1
					else if (j == 2 || (j - 2) % 8 == 0)  Pod2++;        // Pod 2
					else if (j == 3 || (j - 3) % 8 == 0)  Pod3++;        // Pod 3
					else if (j == 4 || (j - 4) % 8 == 0)  Pod4++;        // Pod 4
					else if (j == 5 || (j - 5) % 8 == 0)  Pod5++;        // Pod 5
					else if (j == 6 || (j - 6) % 8 == 0)  Pod6++;        // Pod 6
					else Pod7++;                                         // Pod 7
				}
			}

			// Compute percent based off no_times
			Percents[i][0] = (Pod0*no_pods * 100.0) / datasize; Percents[i][1] = (Pod1*no_pods * 100.0) / datasize; Percents[i][2] = (Pod2*no_pods * 100.0) / datasize;
			Percents[i][3] = (Pod3*no_pods * 100.0) / datasize; Percents[i][4] = (Pod4*no_pods * 100.0) / datasize; Percents[i][5] = (Pod5*no_pods * 100.0) / datasize;
			Percents[i][6] = (Pod6*no_pods * 100.0) / datasize; Percents[i][7] = (Pod7*no_pods * 100.0) / datasize;
			
			// ** Print Percents **
			system("cls");
			std::cout << "\n\n\t\t\t\t";
			for (int i = 0; i < no_pods; i++)
				std::cout << "Pod" << i << ": " << Percents[4][i] << "|";

			// Reset Pod values
			Pod0 = 0, Pod1 = 0, Pod2 = 0, Pod3 = 0, Pod4 = 0, Pod5 = 0, Pod6 = 0, Pod7 = 0;
		}


		// 3. Weight/Decision Branches
		bool Flag = 0;
		// Branch 1 (Rest)
		for (int i = 0; i < 6; i++){
			if (Percents[4][i + 1] < 90.0) Flag = 1;
		} if (!Flag) Rest = 4.0;


		// Branch 2  (Thumb, Index, Middle, Ring, Pinky, Wrist)
		if (Percents[4][3] >= 84.0){

			if (Percents[4][5]> 85.0) Thumb += 1.0;
			
			else if (Percents[4][5] < 65.0) Ring += 1.0;                   

			else{
				if (Percents[4][0] > 45.0 && Percents[4][6] > 80.0){       // Changed up stuff in this nest
						if (Percents[4][4] > 84.0) {
							if (Percents[4][1] < 84.0) Index += 1.0;
						}
						else
							if (Percents[4][1] > 84.0) Middle += 1.0;
				}
				else{
					Pinky += 0.8; Middle += 0.1; Index += 0.1;
				}
			}
		}
		else{
			Wrist += 1.0;
		}

		// ** Print Branch 2 results **
		std::cout << "\n\n\t\t\tBranch 2: \n";
		std::cout << "\t\t\t\tRest: " << Rest << " |Thumb: " << Thumb << " |Index: " << Index << " |Middle: " << Middle << " |Ring: " << Ring << " |Pinky: " << Pinky << " |Wrist: " << Wrist;

		// Branch 3 (Ring, Wrist) ** Methinks this oscillates Ring/Wrist result a little!
		if (Percents[4][5] <= 61)
		{
			if (Percents[4][0] > 58) Wrist += 1.0;
			else
				Ring += 1.0;
		}

		// ** Print Branch 3 results **
		std::cout << "\n\n\t\t\tBranch 3: \n";
		std::cout << "\t\t\t\tRest: " << Rest << " |Thumb: " << Thumb << " |Index: " << Index << " |Middle: " << Middle << " |Ring: " << Ring << " |Pinky: " << Pinky << " |Wrist: " << Wrist;

		// Branch 4 (Thumb, Index, Middle, Ring, Pinky, Wrist)
		bool PodFlag = 0;
		for (int i = 1; i < no_pods; i++){
			if (Percents[4][0] > Percents[4][i])
				PodFlag = 1;    // Flag set if Pod 0 is not the smallest
		}
		if (!PodFlag){
			if (abs(Percents[4][5] - Percents[4][0]) <= 10) Ring += 1.0;
            if (abs(Percents[4][5] - Percents[4][0]) > 10)Wrist += 1.0;
		}

		// ** Print Branch 4 results **
		std::cout << "\n\n\t\t\tBranch 4: \n";
		std::cout << "\t\t\t\tRest: " << Rest << " |Thumb: " << Thumb << " |Index: " << Index << " |Middle: " << Middle << " |Ring: " << Ring << " |Pinky: " << Pinky << " |Wrist: " << Wrist;


		// Insert more Branches here


		// 4. Select based on final Weight values
		float temp = 0.0; int largestindex = 0, gpio;
		finalvalue[0] = Rest; finalvalue[1] = Thumb; finalvalue[2] = Index; finalvalue[3] = Middle; finalvalue[4] = Ring; finalvalue[5] = Pinky; finalvalue[6] = Wrist;
		for (int i = 0; i < 7; i++)
		{
			if (temp < finalvalue[i]){
				temp = finalvalue[i]; largestindex = i;
			}
		}

		if (largestindex == 0) gesture = "Rest"; if (largestindex == 1) gesture = "Thumb"; if (largestindex == 2) gesture = "Index";
		if (largestindex == 3) gesture = "Middle"; if (largestindex == 4) gesture = "Ring"; if (largestindex == 5) gesture = "Pinky";
		if (largestindex == 6) gesture = "Wrist";

		// Print gesture
		// Display weights
		//std::cout << "\n\n\n      Final values:";
		//for (int i = 0; i < no_gestures; i++)
		//std::cout << "   " <<  _gestures[i] << ": " << finalvalue[i] << " |";
		//std::cout << "\n\n\n\n\n\n\n\n\t\t\t\t\t\t\t\tGesture detected: " << gesture << "!\n";
		//Sleep(2000);

		switch (largestindex){
		case 0:
			gpio = 0;      // Rest
			break;
		case 1:
			gpio = 64;     // Thumb
			break;
		case 2:
			gpio = 32;     // Index
			break;
		case 3:
			gpio = 16;     // Middle
			break;
		case 4:
			gpio = 8;      // Ring
			break; 
		case 5:
			gpio = 4;      // Pinky 
			break;
		case 6:
			gpio = 2;      // Wrist
			break;
		default:
			gpio = 0;
		} 

		return gpio;
	}

};

void GestureSelect(PCIE_HANDLE hPCIe, int nSel)
{
	BOOL bPass;

	bPass = PCIE_Write32(hPCIe, DEMO_PCIE_USER_BAR, DEMO_PCIE_FIFO_GPIO_ADDR, (BYTE)nSel);
	if (bPass)
		printf("GESTURE control success \r\n");
	else
		printf("GESTURE conrol failed\r\n");
}


int main()
{
	// Define Calibration(Cal.) mode test-gestures
	std::string gestures[12] = { "*REST/RELAX* position", "*(THUMB)* contraction", "*REST/RELAX* position", "*(INDEX fing.)* contraction", "*REST/RELAX* position",
		"*(MIDDLE fing.)* contraction", "*REST/RELAX* position", "*(RING fing.)* contraction", "*REST/RELAX* position",
		"*(PINKY fing.)* contraction", "*REST/RELAX* position", "*(HAND)* contraction" };
	std::string mode, GR_method = "Characteristics";
	const static bool toCalibrate = 0;  // Set user mode i.e calibration w/user data or run on Stored data
	const static int datasize = 240;    // datasize for training and testing

	void *lib_handle;
	PCIE_HANDLE hPCIE;
	int gpio;

	// We catch any exceptions that might occur below -- see the catch statement for more details
	try {
		system("mode 650");   // Full screen mode
		
		// *** Insert Serial Port communication code here ***


		// First, we create a Hub without any application identifier (I deemed it unnecessary), the Hub provides access to one or more Myos
		myo::Hub hub("com.thalmic.myo");
		
		std::cout << "\t\t\t\t\t\t Attempting to find HU Terminator Myo..." << std::endl;

		// Next, we attempt to find our Myo. If it's already paired in Myo Connect, this will return our Myo immediately
		// waitForMyo() takes a timeout value in milliseconds
		myo::Myo* myo = hub.waitForMyo(0);               // Times-out until Myo is found! 

		hub.setLockingPolicy(hub.lockingPolicyNone);      // Keep Terminator Myo unlocked

		// If waitForMyo() returned a null pointer, we failed to find our Myo, so exit with an error message
		if (!myo) {
			throw std::runtime_error("Unable to find HU Terminator Myo! \n Please Check MyoConnect!");
		}

		// We've found our Myo!
		std::cout << "\t\t\t\t\t\t Now connected to HU Terminator Myo! " << std::endl << std::endl;

		// Next we enable EMG streaming on the found Myo
		myo->setStreamEmg(myo::Myo::streamEmgEnabled);
		myo->unlockHold;

		// Next we load the PCIE
		std::cout << "Loading and opening PCIe connection... \n";

		lib_handle = PCIE_Load();
		if (!lib_handle){
			printf("PCIE_Load failed!\r\n");
			return 0;
		}
		hPCIE = PCIE_Open(DEFAULT_PCIE_VID, DEFAULT_PCIE_DID, 0);
		if (!hPCIE){
			printf("PCIE_Open failed\r\n");
			return 0;
		}
		std::cout << "PCIe loaded and opened succesfully!";

		// Next we construct an instance of our DeviceListener, so that we can register it with the Hub
		DataCollector collector;
		GestureRec TerminatorGR;    // Instance of Terminator Myo_FANN

		// Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
		// Hub::run() to send events to all registered device listeners
		hub.addListener(&collector);
		
		// Set console font parameters (change some default cmd properties)
		CONSOLE_FONT_INFOEX cfi;
		cfi.cbSize = sizeof cfi;
		cfi.nFont = 0;
		cfi.dwFontSize.X = 0;
		cfi.dwFontSize.Y = 16;
		cfi.FontFamily = FF_DONTCARE;
		cfi.FontWeight = FW_NORMAL;
		wcscpy_s(cfi.FaceName, L"Consolas");
		SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);

		std::cout << "\n\n\n\n\n\n\t\t\t\t\t\t   Welcome to Terminator Arm Gen. (1.0) !" << std::endl;
		std::cout << "\t\t\t\t\t  Allow a couple seconds while Terminator Myo warms up to arm... " << std::endl << std::endl;
		Sleep(5000);             // suspend execution of current/active thread for time-argument

	   mode = "User_App";
			
		// Finally we enter our Real-time loop.
		
		system("cls");
		std::cout << "\n\n\n\t\t\t\t\t\t\tMode:  " << mode << "\n\n\t\t\t\t\t\t\tGR_method:  " << GR_method << " \n"; 
		std::cout << "\t\t\t\t\t\tTerminator Myo is now ready to listen for gestures! \n"; Sleep(2000);
		system("cls");
		Listen:
		while (collector.onArm){
				
				// Run Myo hub
				hub.run(1000 / 20);

				// After processing events, we call the writeData() function to write new data to our outfile/array
				collector.writeRunData();

				// Parse input data stream
				TerminatorGR.ParseData(GR_method);

				// Call GR_method
			    gpio = TerminatorGR.Translate_Characteristics(collector.DataArray);

				// Write through PCIe
				GestureSelect(hPCIE, gpio);
	}
		
		//while (collector.onArm == false){
			//system("cls");  std::cout << "\n\n\n \t\tPlease re-sync Terminator Myo for best results!";
		//}
		goto Listen;

		// Tidy up & End program
		system("cls"); PCIE_Close(hPCIE);
		std::cout << "\n       *****  Thank you from HU Team Terminator!  *****\n\n";
		Sleep(3000);
		return 0;

	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << "Press enter to continue.";
		PCIE_Unload(lib_handle);
		std::cin.ignore();
		return 1;
	}
}
