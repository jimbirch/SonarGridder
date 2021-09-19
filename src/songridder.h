/*
╔══════════════════════════════════════════════════════════════════════════════╗
║SonarGridder: A utility for georeferencing SON files                          ║
╠══════════════════════════════════════════════════════════════════════════════╣
║Copyright ©2021 Jim Birch (https://angularfish.net)                           ║
╟──────────────────────────────────────────────────────────────────────────────╢
║This program is free software; you can redistribute it and/or modify it under ║
║the terms of the GNU General Public License as published by the Free Software ║
║Foundation; either version 2 of the License, or (at your option) any later    ║
║version.                                                                      ║
║                                                                              ║
║This program is a hobby project distributed in the hope that it will be useful║
║but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY║
║or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for   ║
║more details.                                                                 ║
║                                                                              ║
║You should have received a copy of the GNU General Public License along with  ║
║this program; if not, write to the Free Software Foundation, Inc., 51 Franklin║
║Street, Fifth Floor, Boston, MA 02110-1301 USA.                               ║
╟──────────────────────────────────────────────────────────────────────────────╢
║Files:                                                                        ║
║songridder.h: Physical metadata and file format macros, function prototypes   ║
║main.cpp: Command line invocation, help                                       ║
║processfiles.cpp: Common functions for all files used in the analysis         ║
║sidescan.cpp: Functions for georeferencing sidescan files                     ║
╟──────────────────────────────────────────────────────────────────────────────╢
║Tested with data produced by a Humminbird 598ci HD manufactured circa 2014.   ║
║May work with other units with or without modification. File format specific  ║
║macros are defined in songridder.h. See README.md for more information. As    ║
║originally packaged, data are processed as though they came from fresh water. ║
║If used in salt water, SOUNDSPEED and SAMPLESPERMETER should be changed in    ║
║songridder.h.                                                                 ║
╟──────────────────────────────────────────────────────────────────────────────╢
║Suggested citation:                                                           ║
║Jim Birch (2021). SonarGridder: A utility for georeferencing consumer-grade   ║
║side-scan sonar files. URL https://angularfish.net                            ║
╟──────────────────────────────────────────────────────────────────────────────╢
║Please let me know if you find this software useful! jim[át]jdbirch.com       ║
║                 (Or if you can't get it to run at all)                       ║
╚══════════════════════════════════════════════════════════════════════════════╝
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
#define SOUNDSPEED 1463 // The speed of sound. Hardware encoded is 1463 in fresh
#define PI 3.141592654  // water and 1500 in salt water (I think).
#define SAMPLESPERMETER 54.4245 // Sample rate in Hertz divided by SOUNDSPEED
#define DEPTHCORR 1 // If using a non-firmware specified SOUNDSPEED, this should
                    // be changed (see README.md).
// File properties
#define LENLOC 62 // Ping (metadata) header offset of the sentence length
#define NORTHINGLOC 20 // Ping header offset of the GPS northing variable
#define EASTINGLOC 15 // Ping header offset of the GPS easting variable
#define DEPTHLOC 35 // Ping header offset of the depth variable
#define HEADERLEN 67 // Ping (metadata) header length, including start sequence
#define HEADINGLOC 27 // Ping header offset of the GPS heading

using namespace std; // I know. Bite me (:

// Global variables are the easiest way for me to include user-changeable
// constraints without rewriting a bunch of code.
extern uint16_t speed_sound;
extern double count_samples_meter;
extern double correction_depth;

extern uint8_t offset_sentence_length;
extern uint8_t offset_depth;
extern uint8_t offset_northing;
extern uint8_t offset_easting;
extern uint8_t offset_heading;
extern uint8_t length_header;

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
// Side imaging functions
void breakOnNoPosition(unsigned char* sonIn, bool* breaks, int lineCount);
void breakOnDirectionChange(unsigned char* sonIn, bool* breaks, int lineCount,
                            unsigned int directionChange);
void breakOnMaxLines(bool* breaks, int lineCount, int maxLines);
void distanceAcrossTrack(int* distance, unsigned int lineLen, unsigned int depth);
bool generateSideScanCSV(std::string path, unsigned char* sonData, unsigned int* lineStarts, int lineLen,
                         int startLine, int endLine, bool port);
bool generateTIFF(std::string path, unsigned char* sonData, unsigned int* lineStarts,
                  int lineLen, int startLine, int endLine, bool port,
                  int minSize);
bool boatPathCSV(std::string path, unsigned char* sonData, unsigned int* lineStarts, int count);
bool processSideScan(std::string filename, bool writeCSV, bool writeTIFF,
                     bool pathAndDepth, int tolerateAngle, int maxLength,
                     int minLength, bool port);
#endif

