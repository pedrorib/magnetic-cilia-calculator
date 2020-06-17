// magfield_calculator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "veclib.h"
#include "mag_functions.h"
#include <string>
#include <iostream>
#include <time.h>
#include <thread>
#include <vector>
#include <filesystem>
#include <sstream>

#define MAXDIPSIZE 32768
#define MBSIZE 1048576
#define DEBUG 0
#define STANDOFF 0.02

using namespace std;
using namespace std::filesystem;

int fileCounter(std::string scannedDir) {
	
	std::size_t number_of_files = 0u;

	namespace fs = std::experimental::filesystem;

	for (auto const& file : directory_iterator(scannedDir))
	{
		++number_of_files;
	}
	return number_of_files;

}

int main(int argc, char* argv[])
{

	string dataPath, magMoment, simSize, userInput, resSize, outputFile, pillarPath, rotation;
	vector<string> dataPathArray;

	bool flagSilent = false;

	fstream config;
	config.open("Config.txt", fstream::in | fstream::out);
	getline(config, dataPath);
	getline(config, pillarPath);
	getline(config, rotation);
	getline(config, magMoment);
	getline(config, simSize);
	getline(config, resSize);
	getline(config, outputFile);

	if (argc == 4) {
		flagSilent = true;
		dataPath = argv[1];
		outputFile = argv[2];
		rotation = argv[3];
	}
	else if (argc > 4) {
		cout << "Too many arguments" << endl;
		return 0;
	}

	if (flagSilent == false) {
		cout << "Paste path to mechanical data location (" << dataPath << ")" << endl;
		getline(cin, userInput);
		if (!userInput.empty())
			dataPath = userInput;
		cout << "Paste path to pillar location (" << pillarPath << ")" << endl;
		getline(cin, userInput);
		if (!userInput.empty())
			pillarPath = userInput;
		cout << "Rotation angle (" << rotation << " deg)" << endl;
		getline(cin, userInput);
		if (!userInput.empty())
			rotation = userInput;
		cout << "Pillar magnetic moment (" << magMoment << " emu)" << endl;
		getline(cin, userInput);
		if (!userInput.empty())
			magMoment = userInput;
		cout << "Square area under study - edge size? (" << simSize << " mm)" << endl;
		getline(cin, userInput);
		if (!userInput.empty())
			simSize = userInput;
		cout << "Grid size (" << resSize << ")" << endl;
		getline(cin, userInput);
		if (!userInput.empty())
			resSize = userInput;
		cout << "Output file (" << outputFile << ")" << endl;
		getline(cin, userInput);
		if (!userInput.empty())
			outputFile = userInput;

		config.clear();
		config.seekg(0, ios::beg);
		config << dataPath << endl;
		config << pillarPath << endl;
		config << rotation << endl;
		config << magMoment << endl;
		config << simSize << endl;
		config << resSize << endl;
		config << outputFile << endl;
		config.close();
	}

	bool passYes = false;

	while (!passYes) {
		
		cout << "Is the mechanical data a folder with an array of files? [Y/N]" << endl;
		getline(cin, userInput);

		if (userInput == "N") {
			dataPathArray.push_back(dataPath);
			passYes = true;
		}
		else if (userInput == "Y") {

			for (int i = 1; i <= fileCounter(dataPath); i++) {
				stringstream ss;
				ss << i;
				dataPathArray.push_back(dataPath + "\\teste_displacement_" + ss.str() + ".txt");
			}
			passYes = true;
		}
	}

	for (int fileNum = 1; fileNum < dataPathArray.size(); fileNum++) {

		cout << "Grid size -> " << resSize << "x" << resSize << endl;
		cout << "Maximum # of dipoles: " << MAXDIPSIZE << endl;

		/*-----------------ALLOCATION----------------*/

		cout << "Allocating space...";

		vec* hInc, ** pInc, * pDip, * mDip, ** hRed, * pPil;
		vec temp(0, 0, 0);
		unsigned int intResSize;

		intResSize = stoi(resSize, nullptr, 10);

		//1st LAYER
		hInc = (vec*)malloc(32768 * sizeof(vec));
		hRed = (vec**)malloc(intResSize * sizeof(vec*));
		pInc = (vec**)malloc(intResSize * sizeof(vec*));
		pDip = (vec*)malloc(32768 * sizeof(vec));
		mDip = (vec*)malloc(32768 * sizeof(vec));
		pPil = (vec*)malloc(9 * sizeof(vec));

		//2nd LAYER
#pragma omp parallel for
		for (int i = 0; i < intResSize; i++) {
			pInc[i] = (vec*)malloc(intResSize * sizeof(vec));
			hRed[i] = (vec*)malloc(intResSize * sizeof(vec));
		}

		cout << "DONE" << endl;
		cout << "Total available size: " << (intResSize * intResSize * 32768 * sizeof(vec) + 2 * intResSize * intResSize * sizeof(vec) + 2 * 32768 * sizeof(vec)) / MBSIZE << "MB" << endl;

		/*------------INPUT STAGE-------------*/

		fstream fileInput;

		cout << "Reading data" << endl;

		fileInput.open(dataPathArray[fileNum].c_str(), fstream::in);
		int lineCounter = 0;
		int repCounter = 0;
		vec bufferPos(0, 0, 0);
		double counterAssistant = 0;
		double appliedAngle = 0;
		string buffer;
		while (getline(fileInput, buffer)) {
			if (buffer.empty() || (buffer.find("%") == 0)) {
				continue;
			}

			istringstream ss(buffer);
			ss >> bufferPos.x >> bufferPos.y >> bufferPos.z >> pDip[lineCounter].x >> pDip[lineCounter].y >> pDip[lineCounter].z;
			if (repCounter == 0)
				counterAssistant = bufferPos.z;
			if (bufferPos.z == counterAssistant)
				repCounter++;
			pDip[lineCounter] = pDip[lineCounter] + bufferPos;
			pDip[lineCounter] = pDip[lineCounter] / 10000; // To cm - INPUT IN MICRON!
			pDip[lineCounter].z = pDip[lineCounter].z + STANDOFF;
			lineCounter++;

			//Apply rotation

			appliedAngle = PI * stod(rotation, nullptr) / 180;
			bufferPos.x = cos(appliedAngle) * pDip[lineCounter].x - sin(appliedAngle) * pDip[lineCounter].y;
			bufferPos.y = sin(appliedAngle) * pDip[lineCounter].x + cos(appliedAngle) * pDip[lineCounter].y;
			bufferPos.z = pDip[lineCounter].z;
			pDip[lineCounter] = bufferPos;
		}

		fileInput.close();

		fileInput.open(pillarPath.c_str(), fstream::in);
		int pilLineCounter = 0;
		while (getline(fileInput, buffer)) {
			istringstream ss(buffer);
			ss >> pPil[pilLineCounter].x >> pPil[pilLineCounter].y >> pPil[pilLineCounter].z;
			pilLineCounter++;
		}

		cout << lineCounter + 1 << " data points read! - Optimizing memory allocation" << endl;
		cout << "Z increases every " << repCounter << " lines." << endl;
		cout << "Magnetization of each dipole: " << stod(magMoment, nullptr) / (double)lineCounter << " emu" << endl;

		pDip = (vec*)realloc(pDip, (lineCounter + 1) * sizeof(vec)); //# of lines determined -> DEFLATE
		mDip = (vec*)realloc(mDip, (lineCounter + 1) * sizeof(vec));
		hInc = (vec*)realloc(hInc, (lineCounter + 1) * sizeof(vec));

		/*---------------COMPUTE MAGNETIZATION VECTOR----------------*/

		for (int i = 0; i < lineCounter - repCounter; i++) {
			mDip[i] = (pDip[i] - pDip[i + repCounter]).norm();
		}
		for (int i = lineCounter - repCounter; i < lineCounter; i++) {
			mDip[i] = mDip[i - repCounter];
		}
		for (int i = 0; i < lineCounter; i++) {
			mDip[i] = -stod(magMoment, nullptr) * mDip[i] / (double)lineCounter;
		}

		/*----------DEGUG ONLY -> DUMP POS AND MAG--------------*/

#if DEBUG == 1

		ofstream mDipFile, pDipFile;

		pDipFile.open("C:\\Users\\pedro\\Desktop\\pDip.txt");
		mDipFile.open("C:\\Users\\pedro\\Desktop\\mDip.txt");

		for (int i = 0; i < lineCounter; i++) {
			cout << pDip[i] << endl;
			pDipFile << pDip[i] << endl;
		}

		cout << "\n" << "SEPARATOR" << "\n" << endl;

		for (int i = 0; i < lineCounter; i++) {
			cout << mDip[i] << endl;
			mDipFile << mDip[i] << endl;
		}

		getchar();

#endif


		/*-------------COMPUTE INCIDENT MAGNETIC FIELD--------------*/

		cout << "Points to compute: " << intResSize * intResSize * lineCounter << " points" << endl;
		cout << "This may take some time..." << endl;

		clock_t t;
		double t_i, t_f, dSimSize;

		dSimSize = 0.001 * stod(simSize, nullptr);
		t_i = clock();
		magCalc(hInc, hRed, mDip, pDip, pPil, 0.1 * stod(simSize, nullptr), lineCounter, intResSize, pilLineCounter);
		//magCalc(hInc, hRed, mDip, pDip, 0.1*stod(simSize, nullptr), lineCounter, intResSize);
		t_f = clock();

#ifdef _WIN32
		cout << "Computation completed in " << (t_f - t_i) / CLOCKS_PER_SEC << " seconds!" << endl;
#else
#if PARALLEL == 1
		cout << "Computation completed in " << (t_f - t_i) / (std::thread::hardware_concurrency() * CLOCKS_PER_SEC) << " seconds!" << endl;
#else
		cout << "Computation completed in " << (t_f - t_i) / CLOCKS_PER_SEC << " seconds!" << endl;
#endif
#endif // WIN32

		/*---------------WRITE OUTPUT-----------------------------*/

		ofstream sendOutput;

		string outputFileFull;

		if (userInput == "N") {
			sendOutput.open(outputFile);
		}
		else {
			stringstream ss;
			ss << fileNum;

			outputFileFull = outputFile + "\\" + "mag_output_" + ss.str() + ".txt";

			sendOutput.open(outputFileFull);
		}


		cout << "Sending data to " << outputFileFull << endl << "...";

		sendOutput << "Magfield calculator - openMP version" << endl;
		sendOutput << "Sensor size: " << simSize << "x" << simSize << endl;
		sendOutput << "Resolution: " << resSize << "x" << resSize << endl;

		for (int i = 0; i < intResSize; i++) {
			for (int j = 0; j < intResSize; j++) {
				temp.x = -dSimSize / 2 + dSimSize * (double)i / (double)(intResSize - 1);
				temp.y = -dSimSize / 2 + dSimSize * (double)j / (double)(intResSize - 1);
				sendOutput << temp << "\t" << hRed[i][j] << endl;
			}
			//sendOutput << endl;
		}
	}

	cout << "DONE" << endl;

	cout << "Program done!" << endl;
	if(argc < 4)
		getchar();

}
