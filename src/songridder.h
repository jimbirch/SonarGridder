/*
    sonargridder: A simple utility for mapping Humminbird SON files
    Copyright © 2021 Angular Fish

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    
    This program is set up to work with files produced by the Humminbird 598ci HD
    side imaging fishfinder (because that's what I have). The basic file
    structure is the same between units, but the header lengths and locations
    of metadata in the binary file may differ. Specs such as frequency and
    wattage may also differ.
*/



#ifndef SONGRIDDER_H
#define SONGRIDDER_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <tiffio.h>
#include <string.h>
#include <iomanip>
#include <time.h>

// Physical constraints
#define SOUNDSPEED 1463
#define PI 3.141592654
#define SAMPLESPERMETER 54.4245

// Physical properties of the down imaging transducer
#define FREQUENCY 200000
#define NEARFIELDRANGE 0.05433008
#define BEAM 20
#define BEAMWIDTH 14.142135642
#define EQUIVBEAM 0.034214216
#define WAVELENGTH 0.007315
#define ALPHA -43.036983702
#define RETURNSTRENCOEFF 0.00761384
#define MAXW 500
#define ARRAYLENGTH 0.039872
#define PINGDURATION 0.000085
#define WAVENUMBER 858.945359945

// File properties
#define LENPOS 62
#define NORTHINGLOC 20
#define EASTINGLOC 15
#define DEPTHLOC 35
#define HEADERLEN 67
#define LENLOC 62
#define HEADINGLOC 27

using namespace std;
// File processing functions
uint32_t decodeInteger(unsigned char byte0, unsigned char byte1, 
                       unsigned char byte2, unsigned char byte3);
uint32_t countLines(unsigned char* file, int size);
unsigned int* lineStarts(unsigned char* file, int size, uint32_t count);
void getLine(unsigned char* file, unsigned char* line, int start, int end);
uint32_t decodeLineLen(unsigned char* line, unsigned int pos);
uint32_t decodeDirection(unsigned char* line, unsigned int pos);
int32_t decodeUTM(unsigned char* line, unsigned int pos);
uint32_t decodeDepth(unsigned char* line, int pos);
void pointLocation(int distance, double* location, double direction, double
                   northing, double easting, bool port);
double latitude(double northing);
double getLatitude(unsigned char* line, int pos);
double longitude(double easting);
double getLongitude(unsigned char* line, int pos);
// Down imaging functions
double waterAttenuation(double H, double pH, int T, double S);
double returnCoeff(unsigned char* line, int loc, double depth);
double calcnoise(unsigned char* line, int end);
double calcE(unsigned char* line, int start, int end, int echo1Start);

double getDepth(uint32_t echos, uint8_t* flag);
void getEchos(unsigned char* peaks, unsigned int* echos, unsigned char* line, 
              uint32_t ll);
void findPeaks(unsigned char* line, unsigned char* peaks, uint32_t ll, 
               float sigma);
void detectPeaks(unsigned char* line, unsigned char* peaks, uint32_t ll);
bool e1e2File(string filename);
// Side imaging functions
void breakOnNoPosition(unsigned char* sonIn, bool* breaks, int lineCount);
void breakOnDirectionChange(unsigned char* sonIn, bool* breaks, int lineCount,
                            unsigned int directionChange);
void breakOnMaxLines(bool* breaks, int lineCount, int maxLines);
void distanceAcrossTrack(int* distance, unsigned int lineLen, unsigned int depth);
bool generateSideScanCSV(unsigned char* sonData, unsigned int* lineStarts, int lineLen,
                         int startLine, int endLine, bool port);
bool generateTIFF(unsigned char* sonData, unsigned int* lineStarts,
                  int lineLen, int startLine, int endLine, bool port,
                  int minSize);
bool boatPathCSV(unsigned char* sonData, unsigned int* lineStarts, int count);
bool processSideScan(string filename, bool writeCSV, bool writeTIFF,
                     bool pathAndDepth, int tolerateAngle, int maxLength,
                     int minLength, bool port);
#endif

