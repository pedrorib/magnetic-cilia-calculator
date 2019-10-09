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
#define PARALLEL 1

void magCalc(vec *calculatedField, vec **destination, vec *magDipoles, vec *posDipoles, double dim, int inputSize, unsigned int gridSize) {

	vec *r, *currPos;
	double tempX, tempY, tempZ;

	r = (vec *)malloc(inputSize * sizeof(vec));
	currPos = (vec *)malloc(inputSize * sizeof(vec));


	using namespace std;
	for (int i = 0; i < gridSize; i++) {
		cout << "Computation - " << 100 * (double)i / gridSize << "% complete" << endl;
		for (int j = 0; j < gridSize; j++) {
			tempX = 0;
			tempY = 0;
			tempZ = 0;

#if PARALLEL == 1

#pragma omp parallel for 
			for (int k = 0; k < inputSize; k++) {
				currPos[k].x = -dim / 2 + dim * (double)i / (double)(gridSize - 1);
				currPos[k].y = -dim / 2 + dim * (double)j / (double)(gridSize - 1);
				currPos[k].z = 0;
				r[k] = currPos[k] - posDipoles[k];
				calculatedField[k] = ((double)3 * r[k] * (magDipoles[k] * r[k])) / pow(r[k].abs(), 5) - magDipoles[k] / pow(r[k].abs(), 3);
			}
#pragma omp parallel for reduction(+: tempX) reduction(+: tempY) reduction(+: tempZ)
			for (int k = 0; k < inputSize; k++) {
				tempX += calculatedField[k].x;
				tempY += calculatedField[k].y;
				tempZ += calculatedField[k].z;
			}
			destination[i][j].x = tempX;
			destination[i][j].y = tempY;
			destination[i][j].z = tempZ;
		}
	}

#else
			for (int k = 0; k < inputSize; k++) {
				currPos[k].x = -dim / 2 + dim * (double)i / (double)(gridSize - 1);
				currPos[k].y = -dim / 2 + dim * (double)j / (double)(gridSize - 1);
				currPos[k].z = 0;
				r[k] = currPos[k] - posDipoles[k];
				calculatedField[k] = ((double)3 * r[k] * (magDipoles[k] * r[k])) / pow(r[k].abs(), 5) - magDipoles[k] / pow(r[k].abs(), 3);
			}
			for (int k = 0; k < inputSize; k++) {
				tempX += calculatedField[k].x;
				tempY += calculatedField[k].y;
				tempZ += calculatedField[k].z;
			}
			destination[i][j].x = tempX;
			destination[i][j].y = tempY;
			destination[i][j].z = tempZ;
#endif
}

int main(int argc, char* argv[])
{
	using namespace std;

	string dataPath, magMoment, simSize, userInput, resSize, outputFile;

	bool flagSilent = false;

	fstream config;
	config.open("Config.txt", fstream::in | fstream::out);
	getline(config, dataPath);
	getline(config, magMoment);
	getline(config, simSize);
	getline(config, resSize);
	getline(config, outputFile);

	if (argc == 3) {
		if (strcmp(argv[1], "-s"))
			bool flagSilent = true;
		else {
			cout << "Unknown flag (" << argv[1] << ")" << endl;
			return 0;
		}
		dataPath = argv[2];
	}
	else if (argc > 3) {
		cout << "Too many arguments" << endl;
		return 0;
	}


	if (!flagSilent) {
		cout << "Paste path to mechanical data location (" << dataPath << ")" << endl;
		getline(cin, userInput);
		if (!userInput.empty())
			dataPath = userInput;
		cout << "Pillar magnetic moment (" << magMoment << " A/m)" << endl;
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
	}

	cout << "Grid size -> " << resSize << "x" << resSize << endl;
	cout << "Maximum # of dipoles: " << MAXDIPSIZE << endl;

	/*-----------------ALLOCATION----------------*/
	
	cout << "Allocating space...";

	vec *hInc, **pInc, *pDip, *mDip, **hRed;
	vec temp(0,0,0);
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
		pDip[lineCounter].z = pDip[lineCounter].z + .01;
		lineCounter++;
	}

	fileInput.close();

	cout << lineCounter + 1 << " data points read! - Optimizing memory allocation" << endl;
	cout << "Z increases every " << repCounter << " lines." << endl;
	cout << "Magnetization of each dipole: " << stod(magMoment, nullptr) / (double)lineCounter << " A.m2" << endl;

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
		mDip[i] = -stod(magMoment, nullptr)*mDip[i] / (double)lineCounter;
	}

	/*----------DEGUG ONLY -> DUMP POS AND MAG--------------*/

#if DEBUG == 1
	
	ofstream mDipFile, pDipFile;

	pDipFile.open("C:\\Users\\pedro\\Desktop\\pDip.txt");
	mDipFile.open("C:\\Users\\pedro\\Desktop\\mDip.txt");

	for (int i = 0; i < lineCounter; i++) {
		cout << mDip[i] << endl;
		pDipFile << pDip[i] << endl;
	}

	cout << "\n" << "SEPARATOR" << "\n" << endl;

	for (int i = 0; i < lineCounter; i++) {
		cout << pDip[i] << endl;
		mDipFile << mDip[i] << endl;
	}

	getchar();

#endif


	/*-------------COMPUTE INCIDENT MAGNETIC FIELD--------------*/

	cout << "Points to compute: " << intResSize * intResSize * lineCounter << " points" << endl;
	cout << "This may take some time..." << endl;

	clock_t t;
	double t_i, t_f, dSimSize;

	dSimSize = 0.001*stod(simSize, nullptr);
	t_i = clock();
	magCalc(hInc, hRed, mDip, pDip, 0.001*stod(simSize, nullptr), lineCounter, intResSize);
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
			temp.x = -dSimSize / 2 + dSimSize * (double)i / (double)(intResSize - 1);
			temp.y = -dSimSize / 2 + dSimSize * (double)j / (double)(intResSize - 1);
			sendOutput << temp << "\t" << hRed[i][j] << endl;
		}
		//sendOutput << endl;
	}

	cout << "DONE" << endl;

	cout << "Program done!" << endl;
	getchar();

}