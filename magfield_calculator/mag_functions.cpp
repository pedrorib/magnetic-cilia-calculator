#include "pch.h"
#include "veclib.h"
#include "mag_functions.h"
#include <string>
#include <iostream>
#include <time.h>
#include <thread>

//Library containing functions to calculate magnetic fields

void magCalc(vec *calculatedField, vec **destination, vec *magDipoles, vec *posDipoles, vec *dipoleDisplacement, double dim, int inputSize, unsigned int gridSize, unsigned int numPillars) { //OVERLOADED
	//Calculates magnetic field of a cilia over a surface
	//Input vars: calculatedField - pointer to vector where the computed magnetic field values will be stored
	//destination: pointer of pointers with the position vectors of each discretization point over the sensor surface
	//magDipoles: pointer to array of magnetization vector of the cilium dipoles
	//posDipoles: pointer to array of dipole position vectors in the undeformed configuration
	//dipoleDisplacement: pointer to array of deformation vectors for each dipole
	//dim: size of the side of the sensor surface
	//inputSize: number of dipoles
	//gridSize: number of discretization points of the sensor surface
	//numPillars: number of pillars over the sensor
	//Does not return

	vec *r, *currPos;
	double tempX, tempY, tempZ;
	int l;

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

#pragma omp parallel for private(l)
			for (int k = 0; k < inputSize; k++) {
				currPos[k].x = -dim / 2 + dim * (double)i / (double)(gridSize - 1);
				currPos[k].y = -dim / 2 + dim * (double)j / (double)(gridSize - 1);
				currPos[k].z = 0;
				calculatedField[k] = vec(0, 0, 0);
				for (l = 0; l < numPillars; l++) {
					r[k] = currPos[k] - (posDipoles[k] + dipoleDisplacement[l]);
					calculatedField[k] = calculatedField[k] + ((double)3 * r[k] * (magDipoles[k] * r[k]) / pow(r[k].abs(), 5) - magDipoles[k] / pow(r[k].abs(), 3)) / 4 / PI;
				}
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

void magCalc(vec *calculatedField, vec **destination, vec *magDipoles, vec *posDipoles, double dim, int inputSize, unsigned int gridSize) { //REDUCED
	//Calculates magnetic field of a cilia over a surface
	//Input vars: calculatedField - pointer to vector where the computed magnetic field values will be stored
	//destination: pointer of pointers with the position vectors of each discretization point over the sensor surface
	//magDipoles: pointer to array of magnetization vector of the cilium dipoles
	//posDipoles: pointer to array of dipole position vectors in the undeformed configuration
	//dipoleDisplacement: pointer to array of deformation vectors for each dipole
	//dim: size of the side of the sensor surface
	//inputSize: number of dipoles
	//gridSize: number of discretization points of the sensor surface
	//Does not return

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
		calculatedField[k] = ((double)3 * r[k] * (magDipoles[k] * r[k]) / pow(r[k].abs(), 5) - magDipoles[k] / pow(r[k].abs(), 3)) / 4 / PI;
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