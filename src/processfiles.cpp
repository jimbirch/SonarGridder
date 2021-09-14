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

uint32_t decodeInteger(unsigned char byte0, unsigned char byte1,
											 unsigned char byte2, unsigned char byte3) {
// Takes four bytes and combines them into a 32 bit unsigned integer.
// Humminbird is big endian. Most header values are unsigned except northing
// and easting. Some values are 16 bit. These can be decomposed using
// decodeInteger(0,0,byte0,byte1). Sonar returns are 8 bit and this 
// function is unnecessary for them.
	uint32_t newInt = 0;
	newInt |= ((uint32_t)byte0) << 24;
	newInt |= ((uint32_t)byte1) << 16;
	newInt |= ((uint32_t)byte2) << 8;
	newInt |= (uint32_t)byte3;
	return newInt;
}

uint32_t countLines(unsigned char* file, int size) {
// Count the lines in a Humminbird SON file. Take the memory-mapped file and
// file size. Returns a uint32_t containing the count of how many times the
// start bit sequency (0xC0DEAB21) occurs in the file.
	uint32_t count = 0;
	for(int i = 0; i < size - 4; i++) {
		if(file[i] == 0xC0 && file[i+1] == 0xDE && 
			 file[i+2] == 0xAB && file[i+3] == 0x21) count++;
	}
	return count;
}

unsigned int* lineStarts(unsigned char* file, int size, uint32_t count) {
// Determines the starting offsets of each line in a Humminbird SON file. Uses
// the line start sequence (0xC0DEAB21) to find the starting bit. Necessary
// when autoranging is turned on or if the user changed the range while
// recording. Takes the memory-mapped file, the file size, and the line count
// (run countLines first). Returns an array of unsigned integers with the
// offset of the start bit for each line.
	unsigned int* starts = new unsigned int[count+1];
	unsigned int i = 0;
	unsigned int pos = 0;
	starts[0] = 0;
	starts[count] = size;
	while (i < count - 1) {
		pos++;
		if(file[pos] == 0xC0 && file[pos+1] == 0xDE && file[pos+2] && file[pos+3]) {
			i++;
			starts[i] = pos;
		}
	}
	return starts;
}

void getLine(unsigned char* file, unsigned char* line, int start, int end) {
// Retrieves a line (an array of bytes between two offsets in the file).
	for(int i = start; i < end; i++) {
		line[i - start] = file[i];
	}
}

uint32_t decodeLineLen (unsigned char* line, unsigned int pos) {
// Figure out the length of a line. Lines encoded as big endian
// unsigned 4 byte integers.
	uint32_t lineLen = decodeInteger(line[pos], line[pos+1], line[pos+2], line[pos+3]);
	return lineLen;
}

uint32_t decodeDirection (unsigned char* line, unsigned int pos) {
// Directions are unsigned big endian 2 byte integers.	
	uint32_t direction = decodeInteger(0,0,line[pos],line[pos+1]);
	return direction;
}

int32_t decodeUTM (unsigned char* line, unsigned int pos) {
// Locations are world mercator (epsg 3395). We're keeping it in meters for corrections.
// Solving for latitude and longitude will probably ultimately happen.
// 4 bytes, representing a signed 32 bit integer.
	uint32_t UTMPos = decodeInteger(line[pos], line[pos+1], line[pos+2], line[pos+3]);
	int32_t position = *(int32_t*)&UTMPos; // Jam the position into a 32 bit signed integer.
	return position;										
}

uint32_t decodeDepth(unsigned char* line, int pos) {
// Finds the depth in a line or file.
	uint32_t depth = decodeInteger(line[pos], line[pos + 1], 
																 line[pos + 2], line[pos + 3]);
	return depth;
}

void pointLocation(int distance, double* location, double direction, double northing, double easting, bool port) {
// Ugh I guess this is where we start using floats.
// Returns an x and y location in meters using distance, boat location,
// and direction.
//	double* location;
//	location = new double[2];
	int scanDirection = direction + 90 * port - 90 * !port;
	if(scanDirection > 360) scanDirection -= 360;
	if(scanDirection < 0) scanDirection += 360;
	location[0] = cos(scanDirection * PI / 180) * distance/1000 + northing;
	location[1] = sin(scanDirection * PI / 180) * distance/1000 + easting;
}

double latitude (double northing) {
// Turns a northing value in World Mercator metres from the GPS to latitude.
	double lat = atan(tan(atan(exp(northing / 6378388.0)) *
										2.0 - 1.570796326794897) * 1.0067642927) * 
										57.295779513082302;
	return lat; 
}

double getLatitude (unsigned char* line, int pos) {
// Single function for getting the latitude of a line.
	double northing = decodeUTM(line, pos);
	double lat = latitude(northing);
	return lat;
}

double longitude (double easting) {
// Turns an easting value in World Mercator metres from the GPS to longitude.
	double lon = easting * 180 / 20038297;
	return lon;
}

double getLongitude (unsigned char* line, int pos) {
// Single function for getting the longitude of a line.
	double easting = decodeUTM(line, pos);
	double lon = longitude(easting);
	return lon;
}

