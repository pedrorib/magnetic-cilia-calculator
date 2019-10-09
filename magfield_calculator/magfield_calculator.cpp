// magfield_calculator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "veclib.h"
#include <string>
#include <iostream>
#include <time.h>

#define MAXDIPSIZE 32768
#define MBSIZE 1048576
#define DEBUG 0


void magCalc(vec *calculatedField, vec **destination, vec *magDipoles, vec *posDipoles, double dim, int inputSize, unsigned int gridSize) {
	
	vec currPos(0, 0, 0);
	vec r;

	using namespace std;

	for (int i = 0; i < gridSize; i++) {
		cout << "Computation - " << 100 * (double)i / gridSize << "% complete" << endl;
	#pragma omp parallel for 
		for (int j = 0; j < gridSize; j++) {
			destination[i][j].x = 0;
			destination[i][j].y = 0;
			destination[i][j].z = 0;
			currPos.x = dim / 2 + dim * (double)i / (double)127;
			currPos.y = dim / 2 - dim * (double)j / (double)127;
			for (int k = 0; k < inputSize; k++) {
				r = posDipoles[k] - currPos;
				calculatedField[k] = 1E-7*((double)3 * r * (magDipoles[k] * r)) / pow(r.abs(), 5) - magDipoles[k] / pow(r.abs(), 3);
			}
			for (int k = 0; k < inputSize; k++) {
				destination[i][j] = destination[i][j] + calculatedField[k];
			}
		}
	}
}

int main()
{
	using namespace std;

	string dataPath, magMoment, simSize, userInput, resSize, outputFile;

	fstream config;
	config.open("Config.txt", fstream::in | fstream::out);
	getline(config, dataPath);
	getline(config, magMoment);
	getline(config, simSize);
	getline(config, resSize);
	getline(config, outputFile);

	cout << "Paste path to mechanical data location (" << dataPath << ")" << endl;
	getline(cin, userInput);
	if (!userInput.empty())
		dataPath = userInput;
	cout << "Pillar magnetic moment (" << magMoment << " T)" << endl;
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
	config << magMoment << endl;
	config << simSize << endl;
	config << resSize << endl;
	config << outputFile << endl;
	config.close();

	cout << "Grid size -> " << resSize << "x" << resSize << endl;
	cout << "Maximum # of dipoles: " << MAXDIPSIZE << endl;

	/*-----------------ALLOCATION----------------*/

	cout << "Allocating space...";

	vec *hInc, **pInc, *pDip, *mDip, **hRed;
	unsigned int intResSize;

	intResSize = stoi(resSize, nullptr, 10);

	//1st LAYER
	hInc = (vec *)malloc(32768 * sizeof(vec));
	hRed = (vec **)malloc(intResSize * sizeof(vec*));
	pInc = (vec **)malloc(intResSize * sizeof(vec *));
	pDip = (vec *)malloc(32768 * sizeof(vec));
	mDip = (vec *)malloc(32768 * sizeof(vec));

	//2nd LAYER
#pragma omp parallel for
	for (int i = 0; i < intResSize; i++) {
		pInc[i] = (vec *)malloc(intResSize * sizeof(vec));
		hRed[i] = (vec *)malloc(intResSize * sizeof(vec));
	}

	cout << "DONE" << endl;
	cout << "Total available size: " << (intResSize * intResSize * 32768 * sizeof(vec) + 2 * intResSize * intResSize * sizeof(vec) + 2 * 32768 * sizeof(vec))/MBSIZE << "MB" << endl;

	/*------------INPUT STAGE-------------*/

	fstream fileInput;

	cout << "Reading data" << endl;

	fileInput.open(dataPath.c_str(), fstream::in);
	int lineCounter = 0;
	int repCounter = 0;
	vec bufferPos(0, 0, 0);
	string buffer;
	while (getline(fileInput, buffer)) {
		if (buffer.empty() || (buffer.find("%") == 0)) {
			continue;
		}
		
		istringstream ss(buffer);
		ss >> bufferPos.x >> bufferPos.y >> bufferPos.z >> pDip[lineCounter].x >> pDip[lineCounter].y >> pDip[lineCounter].z;
		if (bufferPos.z == 0)
			repCounter++;
		pDip[lineCounter] = pDip[lineCounter] + bufferPos;
		lineCounter++;
	}

	fileInput.close();

	cout << lineCounter + 1 << " data points read! - Optimizing memory allocation" << endl;
	cout << "Z increases every " << repCounter << " lines." << endl;

	pDip = (vec *)realloc(pDip, (lineCounter + 1) * sizeof(vec)); //# of lines determined -> DEFLATE
	mDip = (vec *)realloc(mDip, (lineCounter + 1) * sizeof(vec));
	hInc = (vec *)realloc(hInc, (lineCounter + 1) * sizeof(vec));
	
	/*---------------COMPUTE MAGNETIZATION VECTOR----------------*/

	for (int i = 0; i < lineCounter - repCounter; i++) {
		mDip[i] = (pDip[i] - pDip[i + repCounter]).norm();
	}
	for (int i = lineCounter - repCounter; i < lineCounter; i++) {
		mDip[i] = mDip[i - repCounter];
	}
	for (int i = 0; i < lineCounter; i++) {
		mDip[i] = stod(magMoment, nullptr)*mDip[i]/(double) lineCounter;
	}

	/*----------DEGUG ONLY -> DUMP POS AND MAG--------------*/

#if DEBUG == 1

	for (int i = 0; i < lineCounter; i++) {
		cout << mDip[i] << endl;
	}

	cout << "\n" << "SEPARATOR" << "\n" << endl;

	for (int i = 0; i < lineCounter; i++) {
		cout << pDip[i] << endl;
	}

	getchar();

#endif


	/*-------------COMPUTE INCIDENT MAGNETIC FIELD--------------*/

	cout << "Points to compute: " << intResSize * intResSize * lineCounter << " points" << endl;
	cout << "This may take some time..." << endl;

	clock_t t;
	double t_i, t_f;

	t_i = clock();
	magCalc(hInc, hRed, mDip, pDip, 0.001*(double)stoi(simSize, nullptr, 10), lineCounter, intResSize);
	t_f = clock();

#ifdef _WIN32
	cout << "Computation completed in " << (t_f - t_i) / CLOCKS_PER_SEC << " seconds!" << endl;
#else
	cout << "Computation completed in " << (t_f - t_i) / (std::thread::hardware_concurrency() * CLOCKS_PER_SEC) << " seconds!" << endl;
#endif // WIN32

	/*---------------WRITE OUTPUT-----------------------------*/

	ofstream sendOutput;

	sendOutput.open(outputFile);
	
	cout << "Sending data to " << outputFile << endl << "...";

	sendOutput << "Magfield calculator - openMP version" << endl;
	sendOutput << "Sensor size: " << simSize << "x" << simSize << endl;
	sendOutput << "Resolution: " << resSize << "x" << resSize << endl;

	for (int i = 0; i < intResSize; i++) {
		for (int j = 0; j < intResSize; j++) {
			sendOutput << hRed[i][j] << "\t";
		}
		sendOutput << endl;
	}

	cout << "DONE" << endl;

	cout << "Program done!" << endl;
	getchar();

}