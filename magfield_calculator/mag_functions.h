#pragma once
#include "pch.h"
#include "veclib.h"
#include <string>
#include <iostream>
#include <time.h>
#include <thread>

#define PARALLEL 1
#define PI 3.14159265358979323846

#ifndef MAGCAL
#define MAGCAL

void magCalc(vec *calculatedField, vec **destination, vec *magDipoles, vec *posDipoles, vec *dipoleDisplacement, double dim, int inputSize, unsigned int gridSize, unsigned int numPillars);

void magCalc(vec *calculatedField, vec **destination, vec *magDipoles, vec *posDipoles, double dim, int inputSize, unsigned int gridSize);

#endif
