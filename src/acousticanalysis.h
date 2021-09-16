/*
╔══════════════════════════════════════════════════════════════════════════════╗
║AcousticAnalysis: Extremely experimental and probably doesn't work.           ║
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
║Disclaimer: I wrote this code for my own purposes and am sharing it in case   ║
║it is useful to others. This section is experimental, might break my computer,║
║might break your computer, might call your graduate advisor and tell them you ║
║finished your thesis, or might not give you any useful information whatsoever.║
║Please read the attached EXPERIMENTAL text file for suitability information.  ║
║It is unnecessary to compile this code if all you want is georeferenced TIFFs.║
╟──────────────────────────────────────────────────────────────────────────────╢
║Files:                                                                        ║
║acousticanalysis.h:    Physical metadata for the down-imaging transducer,     ║
║                       prototypes for acoustic analysis functions.            ║
║acousticfunctions.cpp: Acoustic analysis functions and data processing.       ║
║main.cpp:              Command line invocation, help                          ║
╟──────────────────────────────────────────────────────────────────────────────╢
║Extremely experimental sonar analysis conducted on the down-imaging           ║
║Transducer. Some code based on descriptions of E1 and E2 analysis from Barb   ║
║Faggetter (Oceanecology.ca) and Dan Buscombe (dbuscombe-usgs.github.io).      ║
╟──────────────────────────────────────────────────────────────────────────────╢
║Tested with data produced by a Humminbird 598ci HD manufactured circa 2014.   ║
║May work with other units with or without modification.                       ║
╟──────────────────────────────────────────────────────────────────────────────╢
║   Please let me know if you find this software useful! jim[át]jdbirch.com    ║
║                 (Or if you can't get it to run at all)                       ║
╚══════════════════════════════════════════════════════════════════════════════╝
*/


#ifndef ACOUSTICANALYSIS_H
#define ACOUSTICANALYSIS_H

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

// Physical properties of the down imaging transducer
#define FREQUENCY 200000 // Frequency in Hertz
#define BEAM 20 // Humminbird -10 dB beam width from the manual
#define WAVELENGTH 0.007315 // SOUNDSPEED/FREQUENCY
#define RETURNSTRENCOEFF 0.00761384 // Can you figure out what 255 means in dB?
#define MAXW 500 // Output power (RMS)
#define PINGDURATION 0.000085 // Length of a ping in seconds (From Buscombe)

// Flags
#define FLAGECHOSFAILED 0b10000000
#define FLAGSECONDECHOFAILED 0b01000000
#define FLAGDEPTHNONSENSE 0b00100000
#define FLAGDEPTHSHALLOW 0b00010000
#define FLAGDEPTHDEEP 0b00001000


using namespace std;
// Down imaging functions
double waterAttenuation(double H, double pH, int T, double S);
double returnCoeff(unsigned char* line, int loc, double depth);
double calcnoise(unsigned char* line, int end);
double calcE(unsigned char* line, int start, int end, int echo1Start);
void addDepthToPeaks(unsigned char* peaks, double depth, uint32_t ll, uint8_t* flag);
double getDepth(uint32_t echos, uint8_t* flag);
void getEchos(unsigned char* peaks, unsigned int* echos, unsigned char* line, 
              uint32_t ll, uint8_t* flag);
void findPeaks(unsigned char* line, unsigned char* peaks, uint32_t ll, 
               float sigma);
void detectPeaks(unsigned char* line, unsigned char* peaks, uint32_t ll);
bool e1e2File(string filename);
#endif

