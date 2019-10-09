#include "pch.h"
#include "veclib.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <cstring>  
#include <string>
#include <sstream>  
#include <math.h>
#include <time.h>
#include <float.h>

using namespace std;

vec::vec() {
	x = 0;
	y = 0;
	z = 0;
}

vec::vec(double x1, double x2, double x3) {
	x = x1;
	y = x2;
	z = x3;
}

double vec::abs() {
	return sqrt(x*x + y * y + z * z);
}

vec vec::norm() {
	vec temp;
	if (x == 0 && y == 0 && z == 0)
		temp.x = 1e-50;
	else {
		temp.x = x / sqrt(x*x + y * y + z * z);
		temp.y = y / sqrt(x*x + y * y + z * z);
		temp.z = z / sqrt(x*x + y * y + z * z);
	}
	return temp;
}

vec vec::operator=(vec seg) {
	x = seg.x;
	y = seg.y;
	z = seg.z;
	return *this;
}

vec vec::operator+=(vec seg) {
	x = x + seg.x;
	y = y + seg.y;
	z = z + seg.z;
	return *this;
}


std::ostream& operator<<(std::ostream& os, const vec& out) {
	os << out.x << "\t" << out.y << "\t" << out.z;
	return os;
}

vec operator+(const vec &pri, const vec &seg) {
	vec temp;
	temp.x = pri.x + seg.x;
	temp.y = pri.y + seg.y;
	temp.z = pri.z + seg.z;
	return(temp);
}

vec operator*(const vec &a, double b)
{
	vec d;

	d.x = a.x*b;
	d.y = a.y*b;
	d.z = a.z*b;

	return d;
}

vec operator*(double a, const vec &b)
{
	vec d;

	d.x = b.x*a;
	d.y = b.y*a;
	d.z = b.z*a;

	return d;
}

double operator*(const vec &a, const vec &b)
{
	double d;

	d = a.x*b.x + a.y*b.y + a.z*b.z;

	return d;
}

vec operator-(const vec &a, const vec &b) {
	vec d;
	d.x = a.x - b.x;
	d.y = a.y - b.y;
	d.z = a.z - b.z;
	return d;
}

vec cross(const vec &a, const vec &b) {
	vec d;
	d.x = a.y*b.z - a.z*b.y;
	d.y = a.z*b.x - a.x*b.z;
	d.z = a.x*b.y - a.y*b.x;
	return d;
}

vec operator/(const vec &a, double b) {
	vec d;
	d.x = a.x / b;
	d.y = a.y / b;
	d.z = a.z / b;
	return d;
}

double operator^(const vec &a, double b) {
	double d;
	d = pow(sqrt(a.x*a.x + a.y*a.y + a.z*a.z), b);
	return(d);
}