#pragma once
#include <iostream>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <cstring>  
#include <string>
#include <sstream>  
#include <cmath>
#include <ctime>
#include <cfloat>
#ifndef VECLIB
#define VECLIB

//VECLIB - provides a structure for 3D vectors, as well as vector operations

class vec {
public:
	double x, y, z;
	vec();
	vec(double x1, double x2, double x3);
	double abs();
	vec norm();
	friend vec operator+(const vec&, const vec&);
	friend vec operator*(double, const vec&);
	friend vec operator*(const vec&, double);
	friend double operator*(const vec&, const vec&);
	friend vec operator-(const vec&, const vec&);
	friend vec cross(const vec&, const vec&);
	friend double operator^(const vec&, double);
	vec operator=(vec seg);
	vec operator+=(vec seg);
	friend std::ostream& operator<<(std::ostream& os, const vec& out);
	friend vec operator+(const vec &pri, const vec &seg);
	friend vec operator*(const vec &a, double b);
	friend vec operator*(double a, const vec &b);
	friend double operator*(const vec &a, const vec &b);
	friend vec operator-(const vec &a, const vec &b);
	friend vec cross(const vec &a, const vec &b);
	friend vec operator/(const vec &, double);
	friend double operator^(const vec &a, double b);
};
#endif

