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
║Tested with data produced by a Humminbird 698ci HD manufactured circa 2014.   ║
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

#include "songridder.h"

// Global variables for all analyses
uint16_t speed_sound = SOUNDSPEED;
double count_samples_meter = SAMPLESPERMETER;
double correction_depth = DEPTHCORR;
uint8_t offset_depth = DEPTHLOC;
uint8_t offset_sentence_length = LENLOC;
uint8_t offset_northing = NORTHINGLOC;
uint8_t offset_easting = EASTINGLOC;
uint8_t offset_heading = HEADINGLOC;
uint8_t length_header = HEADERLEN;

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
  depth = (uint32_t)((double)depth * correction_depth);
  return depth;
}

void pointLocation(int distance, double* location, double direction, double northing, double easting, bool port) {
// Ugh I guess this is where we start using floats.
// Returns an x and y location in meters using distance, boat location,
// and direction.
//  double* location;
//  location = new double[2];
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

