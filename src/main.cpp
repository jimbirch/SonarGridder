/*
		songridder: A simple utility for mapping Humminbird SON files
		By: Angular Fish

		This program is free software; you can redistribute it and/or modify it
		under the terms of the GNU General Public License as published by the Free
		Software Foundation; either version 2 of the License or any later version.

		I have made this software available under the hope that it will be useful,
		without any warranty, implied or otherwise.


		This program is set up to work with files produced by the Humminbird 598ci HD
		side imaging fishfinder (because that's what I have). The basic file
		structure is the same between units, but the header lengths and locations
		of metadata in the binary file may differ. Specs such as frequency and
		wattage may also differ.

*/



#include "songridder.h"

void helpme() {
	cout << "SON Gridder v 0.1 by Angular Fish\n";
	cout << "songridder <filename> <port|starboard|down> ";
	cout << "[-notiff|-nopath|-a 100|-max 100|-min 10]\n\n";
	cout << "Mandatory:\nfilename = a Humminbird .SON file\n";
	cout << "type: port or starboard or down\n";
	cout << "	For sidescan indicate port or starboard transducer. ";
	cout << "For downward looking transducer type down.\n";
	cout << "Optional (Sidescan only): \n";
	cout << "-notiff -nt: Disable georeferenced TIFF output.\n";
	cout << "-nopath -np: Disable csv output of the ship's path.\n";
	cout << "-a [number]: Specify the maximum heading change in tenths of ";
	cout << "a degree.\n";
	cout << "-max [number]: Specify the maximum number of scans in a tiff or ";
	cout << "csv file.\n";
	cout << "-min [number]: Specify the minimum number of scans in a tiff or ";
	cout << "csv file.\n";
}

int main (int argc, char **argv) {
	if(argc < 3) {
		helpme();
		return 0;
	}
	// Start with the mandatory arguments
	string filename = argv[1];
	string type = argv[2];
	bool port = true;
	bool down = true;

	if(type == "port") {
		down = false;
	} else if (type == "starboard") {
		port = false;
		down = false;
	} else if (type == "down") {
		port = false;
	} else {
		helpme();
		return 0;
	}
	
	if(down) {
		bool e1e2 = e1e2File(filename);
		if(!e1e2) {
			cout << "Error outputting e1e2 file for " << filename << "\n";
		}
	} else {
		bool writeCSV = false;
		bool writeTIFF = true;
		bool pathAndDepth = true;
		int tolerateAngle = 100;
		int maxLength = 100;
		int minLength = 10;
		for(int i = 3; i < argc; i++) {
			if(argv[i] == "-csv" || argv[i] == "-c") writeCSV = true;
			else if(argv[i] == "-notiff" || argv[i] == "-nt") writeTIFF = false;
			else if(argv[i] == "-nopath" || argv[i] == "-np") pathAndDepth = false;
			else if(argv[i] == "-a") {
				if(i + 1 >= argc) {
					helpme();
					return 0;
				}
				tolerateAngle = stoi(argv[i+1]);
			}	else if(argv[i] == "-max") {
				if(i + 1 >= argc) {
					helpme();
					return 0;
				}
				maxLength = stoi(argv[i+1]);
			} else if(argv[i] == "-min") {
				if(i + 1 >= argc) {
					helpme();
					return 0;
				}
				minLength = stoi(argv[i+1]);
			}
		}
		bool sideScan = processSideScan(filename, writeCSV, writeTIFF, pathAndDepth,
																		tolerateAngle, maxLength, minLength, port);
		if(!sideScan) {
			cout << "Error outputting sidescan data for " << filename << "\n";
			return 0;
		}
	}
	
	return 0;
}

