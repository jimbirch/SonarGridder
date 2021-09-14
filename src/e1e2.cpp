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



#include "songridder.h"

double waterAttenuation(double H, double pH, int T, double S) {
	//double H = depth;
	int f = FREQUENCY / 1000;
	double A1 = (8.86/SOUNDSPEED) * pow(10, 0.78 * pH -5);
	double f1 = 2.8 * sqrt(S/35) * pow(10, 4 - 1245/(T + 273));
	double A2 = 21.44 * S * (1 + 0.025 * T) / SOUNDSPEED;
	double A3 = 0.0003964 - 0.00001146 * T + 0.000000145 * pow(T, 2) 
							- 0.00000000065 * pow(T, 3);
	double f2 = 8.17 * pow(10, 8 - 1990/(T + 273)) / (1 + 0.0018 * (S - 35));
	double P2 = 1 - 0.000137 * H + 0.0000000062 * pow(H, 2);
	double P3 = 1 - 0.0000383 * H + 0.00000000049 * pow(H, 2);
	double alphaw = A1 * f1 * pow(f, 2) / (pow(f, 2) + pow(f1, 2))
									+ A2 * P2 * f2 * pow(f, 2)/(pow(f,2) + pow(f2, 2))
									+ A3 * P3 * pow(f, 2);
	return alphaw;
}

double returnCoeff (unsigned char* line, int loc, double depth) {
	double attenuation = waterAttenuation(depth, 7, 25, 0.0);
	/* double returnStrength = line[loc] * RETURNSTRENCOEFF + 20 * log10(depth) +
								 depth * attenuation / 500 - 10 * log10(MAXW * pow(WAVELENGTH, 2)
								 * SOUNDSPEED * PINGDURATION * EQUIVBEAM / (32 * pow(PI, 2))); */
	double returnStrength = line[loc] + pow(10, depth * attenuation / 50000);
	// double returnCoeff = pow(10,returnStrength/10);
	// return returnCoeff;
	return returnStrength;
}

double calcnoise(unsigned char* line, int end) {
// Calculates the noise (the average return energy in the water column part of
// the the farfield region). Takes the line and the offset of the first location 
// of the first echo. Returns the average noise energy.
	if(end < HEADERLEN + 108) return 1;
	int startSample = HEADERLEN + 108; // int(NEARFIELDRANGE * SAMPLESPERMETER);
	double depth = (end - HEADERLEN) / SAMPLESPERMETER;
	int endSample = end * 0.9;
	double coefficients = 0;
	for(int sample = startSample; sample < endSample; sample++) {
		coefficients += returnCoeff(line, sample, depth)/(end - startSample);
	}
	// double noiseCalc = 27439232 * PI * coefficients;
	double noiseCalc = coefficients;
	return noiseCalc;
}

double calcE(unsigned char* line, int start, int end, int echo1Start) {
// Calculates E1 or E2. Takes a line, a start offset, and an end offset for both
// the peaks, as well as the start offset for the first echo. Returns the energy 
// of the first (E1) or second (E2) echo.
	double depth = (echo1Start - HEADERLEN) / SAMPLESPERMETER;
	double E = 0;
	for(int sample = start; sample < end; sample++) {
		E += returnCoeff(line, sample, depth);
	}
	E *= 2;
	double noise = calcnoise(line, echo1Start);
	int n = end - start + 1;
	// double roughnessHardness = E * 13619616 * PI - n * noise;
	double roughnessHardness = E - n * noise;
	return roughnessHardness;
}

double getDepth(uint32_t* echos, uint8_t* flag) {
	// Calculates the depth from the 4 element echo array. Takes a 4 element uint32_t
	// array structured as: echo 2 start, echo 2 end, echo 1 start, echo 1 end; and
	// a flag variable. Returns a double of the depth in metres. Sets bit 3 of the flag
	// variable if the peaks aren't the main echos.
	uint32_t echo = echos[4];
	double depth = (echo - HEADERLEN) / SAMPLESPERMETER;
	if(depth < 1 || depth > 200) *flag |= (uint8_t)0x04;
	return depth;
}

void getEchos(unsigned char* peaks, unsigned int* echos, unsigned char* line, uint32_t ll) {
	// Finds the start and end points for the two echos. Takes the line, the array
	// of peaks, and the length of the line. Returns an array of 4 uint32_t containing
	// the offsets in the order: second echo start, second echo end, first echo start,
	// first echo end. Major issues are ranges that are too short or too long (ie: There
	// is only one echo or there are more than two).

	// Step 1: find the depth ping. We will consider this the highest return in a peak.
	uint8_t maxReturn = 0;
	unsigned int depthPing = 0;
	for(unsigned int i = HEADERLEN + 27; i < ll; i++) {
		if(peaks[i] == 0xFF && line[i] > maxReturn) {
			maxReturn = line[i];
			depthPing = i;
		}
	}
	if(depthPing < 336) depthPing = 336;
	//unsigned int depthEnd = depthPing + 3 * NEARFIELDRANGE * SAMPLESPERMETER;

	// Step 2: find the start of the second echo. We will consider this the highest return
	// more than double the distance from the first peak.
	maxReturn = 0;
	unsigned int secondPing = 0;
	unsigned int startSearch = 1.8 * (depthPing - HEADERLEN) + HEADERLEN;
	if(startSearch >= ll) startSearch = ll - 11;
	unsigned int endSearch = 2.2 * (depthPing - HEADERLEN) + HEADERLEN;
	if(endSearch - 10 < startSearch) endSearch += 10;
	if(endSearch >= ll) endSearch = ll - 2;
	for(unsigned int i = startSearch; i < endSearch; i++) {
		if(line[i] > maxReturn) {
			maxReturn = line[i];
			secondPing = i;
		}
	}
	if(secondPing < 337) secondPing += 337;
	//unsigned int secondPingEnd = secondPing + 3 * NEARFIELDRANGE * SAMPLESPERMETER;
	//if(secondPingEnd > ll) secondPingEnd = ll - 1;
	
	// Step 3: find the local minimum ahead of the second echo.
	int8_t incr = 1;
	if(secondPing < depthPing + 336) incr = -1;

	maxReturn = 255;
	unsigned int localMin = 0;
	for(unsigned int i = secondPing; i != secondPing + 54 * incr; i += incr) {
		if(line[i] < maxReturn) {
			localMin = i;
			maxReturn = line[i];
		}
	}

	// Step 4: find the noise floor for the metre preceding the local minimum
	unsigned int noiseFloor = 0;
	for(unsigned int i = localMin; i != localMin + incr * 54; i += incr) {
		noiseFloor += line[i] / 54;
	}
	// cout << "Noise floor = " << noiseFloor << "\n";
	noiseFloor += noiseFloor * 0.1;

	// Step 5: identify the start points for both pings.
	unsigned int depthPingStart = 0;
	for(unsigned int i = depthPing; i >= depthPing - 336; i--) {
		if(i <= HEADERLEN) { 
			depthPingStart = HEADERLEN;
			break;
		}
		else if(line[i] < noiseFloor) {
			depthPingStart = i;
			break;
		}
	}

	unsigned int secondPingStart = 0;
	for(unsigned int i = secondPing; i >= depthPing; i--) {
		if(line[i] < noiseFloor) {
			secondPingStart = i;
			break;
		}
	}

	// Step 6: identify the end points for both pings.
	unsigned int depthPingEnd = 0;
	for(unsigned int i = depthPing; i < depthPing + 336; i++) {
		if(line[i] < noiseFloor) {
			depthPingEnd = i;
			break;
		}
	}

	unsigned int secondPingEnd = 0;
	for(unsigned int i = secondPing; i < secondPing + 336; i++) {
		if(i >= ll) {
			secondPingEnd = ll - 1;
			break;
		} else if(line[i] < noiseFloor) {
			secondPingEnd = i;
			break;
		}
	}

	// Step 7: Save the start and end points;
	echos[0] = secondPingStart;
	echos[1] = secondPingEnd;
	echos[2] = depthPingStart;
	echos[3] = depthPingEnd;
	echos[4] = depthPing;
}

void findPeaks(unsigned char* line, unsigned char* peaks, uint32_t ll, float sigma) {
// Takes a down imaging line, the line's length, and a coefficient for determining the
// noise floor. Returns an array of equal length indicating where peaks are found.
// Mangles the original line to reduce noise over sigma * σ.
	for(unsigned int i = 0; i < ll; i++) {
		peaks[i] = 0;
	}

	// Step one: calculate a moving average over a ~25 cm distance for each sample in
	// the line. We start at 2 m to avoid noise caused by the boat. TODO: make this
	// user changeable.
	for(unsigned int i = HEADERLEN + 108; i < ll; i++) {  
		uint8_t mean = 0;
		for(unsigned int j = i - 13; j < i; j++) { // 13 ~ 25 cm in fresh water.
			mean += line[j]/13;
		}
		uint32_t ustdvar = 0;
		for(unsigned int j = i - 13; j < i ; j++) { 	// Calculate σ
			ustdvar += (mean - line[j]) * (mean - line[j]);
		}
		// Check if we are over sigma σ from the mean
		int peakOffLevel = int(mean - sigma * sqrt(ustdvar/13));
		int peakOnLevel = int(mean + sigma * sqrt(ustdvar/13) * 0.65);
		if((line[i] > peakOnLevel || line[i] < peakOffLevel || line[i] > 230) && line[i] > 100) {
			// Log the peak
			peaks[i] = (unsigned char)0xFF;
			// This unweights the earlier parts of the peak and removes some noise from
			// the sample.
			line[i] = mean + (line[i] - mean) * 0.67;
		}
	} 

	/*for(int i = ll - 14; i >= HEADERLEN + 108; i--) {
		uint8_t mean = 0;
		for(int j = i + 13; j > i; j--) {
			mean += line[j]/13;
		}
		uint32_t ustdvar = 0;
		for(int j = i + 13; j > i; j--) {
			ustdvar += (mean - line[j]) * (mean - line[j]);
		}
		int peakOnLevel = int(mean - sigma * sqrt(ustdvar/13));
		if(line[i] > peakOnLevel) {
			peaks[i] = (unsigned char)0xFF;
			line[i] = mean + (line[i] - mean) * 0.67;
		}
	}*/

	// Step two: Count the peaks, remove peaks that are just from noise.
	unsigned int finder = 0;
	unsigned int tracker = 0;
	unsigned int offcounter = 0;
	peaks[0] = 0x00;
	for(unsigned int i = 77; i < ll; i++) {
		if(peaks[i] == 0xFF) {
			if(offcounter > 13) offcounter = 0;
			tracker++;
			if(tracker < 13 || tracker > 108) { // Zero out all peaks shorter than 25 cm.
				peaks[i] = 0x00;
				finder = 0;
				if(tracker > 108) {
					peaks[0]--;
					for(unsigned int j = i - tracker - 13; j < i; j++) {
						peaks[j] = 0x00;
					}
				}
			} else { // Count all peaks longer than 25 cm.
				finder++;
				for(unsigned int j = i - offcounter - 13; j < i; j++) {
					peaks[j] = (uint8_t)0xFF;
				}
				if(finder == 1 && peaks[0] < 255)	peaks[0]++; // peaks[0] is zero until we
				if(tracker > peaks[3] && tracker < 256) peaks[3] = tracker;
			} 																							// add the count information.
			offcounter = 0;
		}	else {
			offcounter++;
			if(offcounter > 13) {
				tracker = 0;
				finder = 0;
			}
		}
	}
}

void detectPeaks(unsigned char* line, unsigned char* peaks, uint32_t ll) {
// Automated peak detection and noise removal. Takes a down imaging line
// and the length of the line. Returns an array of 1 byte integers containing
// peak detections. Mangles the originial line to remove noise (run getLine
// again prior to processing E1 and E2 data).
	float sigma = 3;
	int i;

	// Step 1: automatic noise removal and peak detection
	// repeats the findPeaks function (which detects peaks
	// and removes noise from the original line) at decreasing
	// sigma σ thresholds until between 2 and 4 peaks are detected
	// or sigma σ equals zero (the window average is the noise
	// threshold.)
	for(i = 0; i < 20; i++) {
		findPeaks(line, peaks, ll, sigma);
		sigma -= 0.1;
		if(sigma < 0) break;
		if((uint8_t)peaks[0] < 4 && (uint8_t)peaks[0] >= 2 && peaks[3] > 27
		&& peaks[3] < 170) break;
	}

	// Run the noise removal two more times at the final sigma σ level.
	// peaks = findPeaks(line, ll, sigma);
	// peaks = findPeaks(line, ll, sigma);

	// Add sigma and the number of times the algorithm was repeated to
	// two unused parts of peaks[].
	int sigmasave = int(sigma * 10);
	peaks[1] = (unsigned char) sigmasave;
	peaks[2] = (unsigned char) i + 2;
}

bool e1e2File (string filename) {
	uint8_t flag = 0;
	char* sFileData;
	streampos size;

	ifstream sonFile (filename, ios::in | ios::binary | ios::ate);
	if(!sonFile.is_open()) {
		cout << "Unable to open file. Aborting.\n";
		return false;
	}

	size = sonFile.tellg();
	sFileData = new char[size];
	sonFile.seekg(0, ios::beg);
	sonFile.read(sFileData, size);
	sonFile.close();

	unsigned char* fileData = reinterpret_cast<unsigned char*>(sFileData);

	uint32_t count = countLines(fileData, (int)size);
	unsigned int* beginnings = lineStarts(fileData, (int)size, count); 
	unsigned int maxLL = 0;

	// Find the maximum line length
	for(unsigned int i = 1; i < count; i++) {
		unsigned int curLL = beginnings[i] - beginnings[i-1];
		if(curLL > maxLL) maxLL = curLL;
	}

	// Output a line as its own csv file
	srand (time(NULL));
	uint32_t midpoint = rand() % count;

	unsigned char* line = new unsigned char[maxLL];
	getLine(fileData, line, beginnings[midpoint], beginnings[midpoint+1]);
	unsigned char* peaks = new unsigned char[maxLL];
	detectPeaks(line, peaks, beginnings[midpoint+1] - beginnings[midpoint]);
	unsigned char* line2 = new unsigned char[maxLL];
	getLine(fileData, line2, beginnings[midpoint], beginnings[midpoint+1]);
	uint32_t* echo = new uint32_t[5];
	getEchos(peaks, echo, line2, beginnings[midpoint+1] - beginnings[midpoint]);
	double depth = getDepth(echo, &flag);
	double noise = calcnoise(line2, echo[2]);
	double E1 = calcE(line2, echo[2], echo[3], echo[2]);
	double E2 = calcE(line2, echo[0], echo[1], echo[2]);
	double attenuation = waterAttenuation(depth, 7, 25, 0);
	double lat = getLatitude(line, NORTHINGLOC);
	double lon = getLongitude(line, EASTINGLOC);
	string message = "No flags\n";
	if(flag & (0x04 > 0)) message = "Second ping is out of range\n";
	ofstream lineCSV("line.csv");
	lineCSV << ",,,,,Scan number:," << midpoint << "\n";
	lineCSV << ",,,,,Latitude:," << lat << ",Longitude:," << lon << "\n";
	lineCSV << ",,,,,Echo one at pings:, " << echo[2] << ", to, " << echo[3] << "\n";
	lineCSV << ",,,,,Echo two at pings:, " << echo[0] << ", to, " << echo[1] << "\n";
	lineCSV << ",,,,,Depth:," << depth << "\n";
	lineCSV << ",,,,,E1:," << E1 << "\n";
	lineCSV << ",,,,,E2:," << E2 << "\n";
	lineCSV << ",,,,,Noise:," << noise << "\n";
	lineCSV << ",,,,,,,,Message:," << message << "\n";
	lineCSV << ",,,,,Attenuation:," << attenuation << "\n";
	lineCSV << "n,ret,retc,peak,guess\n";

	delete[] line2;

	int j = 0;

	unsigned int guess = 0;
	for(unsigned int i = HEADERLEN; i < beginnings[midpoint+1] - beginnings[midpoint]-14; i++) {
		if(i > HEADERLEN + 3) j = i + peaks[2]/7 + 13;
		else j = i - HEADERLEN;
		if(((i >= echo[0]) && (i <= echo[1])) || 
			 ((i >= echo[2]) && (i <= echo[3]))) guess = 0xFF;
		else guess = 0x00;
		lineCSV << i << "," << (int)line2[i] << "," << (int)line[j] << "," << (int)peaks[j];
		lineCSV << "," << guess << "\n";
	}

	ofstream csv("e1e2.csv");
	if(!csv.is_open()) {
		cout << "Error writing csv file. Aborting.\n";
		return false;
	} 
	csv << "Number,Depth,Latitude,Longitude,E1,E2\n";
	//csv.setf(ios_base::fixed);
	//csv.precision(10);
	//unsigned char* line;
	for(unsigned int i = 0; i < count; i++) {
		flag = 0;
		uint32_t ll = beginnings[i + 1] - beginnings[i];
		csv << i << ",";
		getLine(fileData, line, beginnings[i], beginnings[i+1]);
		detectPeaks(line, peaks, ll);
		getLine(fileData, line, beginnings[i], beginnings[i+1]);
		getEchos(peaks, echo, line, ll);
		double depth = getDepth(echo,&flag);
		if(flag > 0) {
			csv << ",0,0,0,0,0,0\n";
			continue;
		}
		csv << depth << ",";
		double latitude = getLatitude(line, NORTHINGLOC);
		csv << fixed << setprecision(10) << latitude << ",";
		double longitude = getLongitude(line, EASTINGLOC);
		csv << fixed << setprecision(10) << longitude << ",";
		double E1 = calcE(line, echo[2], echo[3], echo[4]);
		csv << E1 << ",";
		double E2 = calcE(line, echo[0], echo[1], echo[4]);
		csv << E2 << ",";
		double Noise = calcnoise(line, echo[2]); 
		csv << Noise << "\n";
	}
	csv.close(); 
	//delete[] line;
	delete[] fileData;
	delete[] line;
	delete[] peaks;
	delete[] echo;

	cout << "E1E2 Calculated Successfully.\n";

	return true;
}

