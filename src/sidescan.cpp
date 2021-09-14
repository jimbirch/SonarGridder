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

void breakOnNoPosition (unsigned char* sonIn, bool* breaks, int lineCount) {
// Updates the break point matrix to add scans that don't have a corresponding location.
// This can happen if the GPS has no fix.
	unsigned int eastingPos = EASTINGLOC;
	unsigned int northingPos = NORTHINGLOC;
	unsigned int lenPos = LENLOC;
	uint32_t lineLen = 0;
	for(int i = 0; i < lineCount; i++) {
		lineLen = decodeInteger(sonIn[lenPos], sonIn[lenPos + 1],
														sonIn[lenPos + 2], sonIn[lenPos + 3]);
		int32_t easting = decodeUTM(sonIn, eastingPos);
		int32_t northing = decodeUTM(sonIn, northingPos);
		if(easting == 0 || northing == 0) {
			cout << "No position for line number: " << i << ".\n";
			breaks[i] = true;
		}
		lenPos = lenPos + HEADERLEN + lineLen;
		eastingPos = eastingPos + HEADERLEN + lineLen;
		northingPos = northingPos + HEADERLEN + lineLen;
	}
}

void breakOnDirectionChange (unsigned char* sonIn, bool* breaks, int lineCount, 
														 unsigned int directionChange) {
// Updates the break point matrix to add scans where the direction changes, this prevents
// Overlapping data.
	unsigned int lenPos = LENLOC;
	unsigned int lineLen = 0;
	unsigned int directionPos = HEADINGLOC;
	uint32_t direction = 0;
	uint32_t initialDirection = 0;
	for(int i = 0; i < lineCount; i++) {
		lineLen = decodeInteger(sonIn[lenPos], sonIn[lenPos + 1],
														sonIn[lenPos + 2], sonIn[lenPos + 3]);
		direction = decodeInteger(0, 0,	sonIn[directionPos], sonIn[directionPos + 1]);
		// Correct direction to eliminate zero crossings
		if(direction < directionChange) direction += 3600;
		if(direction + directionChange > 3600) direction -= 3600;
		// Determine if we need a break point
		if(direction > initialDirection + directionChange || 
			direction < initialDirection - directionChange) {
			breaks[i] = true;
			initialDirection = direction;
		}
		directionPos = directionPos + HEADERLEN + lineLen;
		lenPos = lenPos + HEADERLEN + lineLen;
	}
}

void breakOnMaxLines (bool* breaks, int lineCount, int maxLines) {
	int lastBreak = 0;
	for(int i = 0; i < lineCount; i++) {
		if(breaks[i]) lastBreak = i;
		else if(i - lastBreak > maxLines) {
			breaks[i] = true;
			lastBreak = i;
		}
	}
}

void distanceAcrossTrack(int* distance, unsigned int lineLen, unsigned int depth) {
// Gets the across track distance for each reading in milimeters.
	for(unsigned int i = 0; i < lineLen; i++) {
		if(i < HEADERLEN) {
			distance[i] = 0;
		} else {
			unsigned int pingDist = (i - HEADERLEN) / SAMPLESPERMETER;
			if(pingDist < depth + (HEADERLEN/SAMPLESPERMETER)) distance[i] = 0;
			else {
				double dist = sqrt(pow(pingDist,2) - pow(depth,2));
				distance[i] = int(dist);
			}
		}
	}
}

bool generateSideScanCSV(unsigned char* sonData, unsigned int* lineStarts, int lineLen, 
								int startLine, int endLine, bool port) {
	// Generates a CSV file containing the coordinates and return strength of each sample.
	if(startLine + 1 == endLine) return false;
	string side = "Starboard";
	if(port) side = "Port";
	ofstream csv(side + "_lines" + to_string(startLine) + "to" + to_string(endLine) + ".csv");
	if(!csv.is_open()) return false;
	csv.precision(11);
	double startLocs[2][2];
	unsigned char* line = new unsigned char[lineLen];
	int* distance = new int[lineLen];
	double* xy = new double[2];
	startLocs[0][0] = decodeUTM(sonData,lineStarts[startLine] + NORTHINGLOC);
	startLocs[0][1] = decodeUTM(sonData,lineStarts[startLine] + EASTINGLOC);
	startLocs[1][0] = decodeUTM(sonData,lineStarts[endLine] + NORTHINGLOC);
	startLocs[1][1] = decodeUTM(sonData,lineStarts[endLine] + EASTINGLOC);
	double slopeNorth = (startLocs[1][0] - startLocs[0][0])/(endLine - startLine);
	double slopeEast = (startLocs[1][1] - startLocs[0][1])/(endLine - startLine);
	csv << "LineNo,Depth(mm),SigStren,latitude,longitude,distance\n";
	for(int i = startLine; i < endLine; i++) {
		getLine(sonData, line, lineStarts[i], lineStarts[i+1]);
		double northing = startLocs[0][0] + slopeNorth * (i - startLine);
		double easting = startLocs[0][1] + slopeEast * (i - startLine);
		int32_t direction = decodeDirection(line, HEADINGLOC);
		int32_t depth = decodeDepth(line, DEPTHLOC) * 100;
		distanceAcrossTrack(distance, lineLen, depth);
		for(int j = HEADERLEN + depth/SAMPLESPERMETER; j < lineLen; j++) {
			pointLocation(distance[j], xy, (double)direction/10.0, northing, easting, port);
			double longi = longitude(xy[1]);
			double lat = latitude(xy[0]);
			uint16_t sigStren = (uint16_t)line[j] & 255;
			csv << i << ",";
			csv << depth << ",";
			csv << sigStren << ",";
			csv << longi << ",";
			csv << lat << "," ;
			csv << distance[j] << "\n";
		}
	}
	delete[] line;
	delete[] distance;
	delete[] xy;
	csv.close();
	return true;
}

bool generateTIFF(unsigned char* sonData, unsigned int* lineStarts, int lineLen, 
								int startLine, int endLine, bool port, int minSize) {
	// Generates a tiff file and georeferences it using a world file.
	if(startLine >= endLine - minSize) return false;
	string side = "Starboard";
	if(port) side = "Port";
	stringstream fname;
	fname << side << "_lines" << setw(8) << setfill('0') << startLine << "to";
	fname << setw(8) << setfill('0') << endLine;
	string filename = fname.str() + ".tif";
	int n = filename.length();
	char* filenameArray = new char[n+1];
	strcpy(filenameArray, filename.c_str());
	TIFF *out = TIFFOpen(filenameArray,"w");
	delete[] filenameArray;
	uint32_t minDepth = 0xFFFF;
	for (int i = startLine; i < endLine; i++) {
		uint32_t lineDepth = decodeDepth(sonData, lineStarts[i] + DEPTHLOC);
		if(lineDepth < minDepth) minDepth=lineDepth;
	}
	minDepth *= 100;
	int maxDist = (int)sqrt(pow((lineLen - HEADERLEN) * 19,2)-pow(minDepth,2));
	TIFFSetField(out, TIFFTAG_IMAGEWIDTH, maxDist/19);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, endLine - startLine);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

	int tiffLineLen = TIFFScanlineSize(out);
	unsigned char* line = new unsigned char[lineLen];
	int* distance = new int[lineLen];
	char* tiffLine = new char[tiffLineLen];
	
	for(int i = startLine; i < endLine; i++) {
		getLine(sonData, line, lineStarts[i], lineStarts[i+1]);
		uint32_t depth = decodeDepth(line, DEPTHLOC) * 100;
		uint32_t currLineLen = lineStarts[i+1] - lineStarts[i];
		unsigned int currLinePos = int(depth / 19) + HEADERLEN;
		if(currLinePos >= currLineLen) continue;
		distanceAcrossTrack(distance, lineLen, depth);
		for(int j = 0; j < tiffLineLen; j++) {
			if(j > HEADERLEN + maxDist/19) { 
				tiffLine[j] = 0;
				continue;
			} 
			else if(distance[currLinePos] > HEADERLEN + j * 19) {
				tiffLine[j] = line[currLinePos];
			}
			else {
				currLinePos++;
				if(currLinePos >= currLineLen) {
					currLinePos = currLineLen - 1;
					tiffLine[j] = 0;
					continue;
				}
				tiffLine[j] = line[currLinePos];
			}
		}
		bool writeSuccess = TIFFWriteScanline(out,tiffLine,i-startLine,0);
		if (!writeSuccess) {
			cout << "Error writing TIFF.\n";
			return 0;
		}
	}
	delete[] line;
	delete[] tiffLine;
	delete[] distance;
	(void)TIFFClose(out);

	// Build world file
	ofstream tfw(fname.str() + ".tfw");
	if(!tfw.is_open()) return false;

	// Bump up the precision and disable scientific notation
	tfw.setf(ios_base::fixed);
	tfw.precision(20);

	// Make a place to store the locations of our corners
	double* topLeft { new double[2] };
	double* bottomRight { new double[2] };

	// Find the location of the boat at the top left of our raster
	topLeft[0] = decodeUTM(sonData, lineStarts[startLine] + NORTHINGLOC);
	topLeft[1] = decodeUTM(sonData, lineStarts[startLine] + EASTINGLOC);

	// Find the location of the boat at the bottom (left) of our raster
	bottomRight[0] = decodeUTM(sonData, lineStarts[endLine-1] + NORTHINGLOC);
	bottomRight[1] = decodeUTM(sonData, lineStarts[endLine-1] + EASTINGLOC);

	// Determine the direction the boat was heading (so we can rotate)
	double direction = atan2((latitude(topLeft[0])-latitude(bottomRight[0])),
													(longitude(topLeft[1])-longitude(bottomRight[1])));
	direction += PI/2; // We used the ship's heading. This was a mistake, corrected by adjusting
										 // the rotation by 90° (π/2) because I'm lazy.
	
	// Our raster's layout is vertical, with the distance the boat traveled on
	// the y-axis and the length of the scan on the x-axis. The origin is the top
	// left and scans are always to the right. We convert everything into latitude
	// and longitude at the earliest convenience to avoid conversion errors with
	// the instrument's world mercator projection (EPSG 3395?) because the conversion
	// using GDAL sucks for some reason.
	bottomRight[0] = latitude(topLeft[0]) + sqrt(pow(latitude(topLeft[0]) 
									 - latitude(bottomRight[0]),2) + pow(longitude(topLeft[1]) 
									 - longitude(bottomRight[1]),2)); // The y-location of the bottom right is
									 																	// the latitude of the top left plus the
																										// distance travelled, calculated as the
																										// hypotenuse of the right triangle where
																										// Δlatitude and Δlongitude are the sides.

	bottomRight[1] = topLeft[1] + maxDist * !port/1000 // The x-location of the bottom right
									 - maxDist * port/1000;						 // is the x-location of the top left plus
									 																	 // the maximum across-track distance.
																										 // The across track distance is negative
																										 // for the port transducer (it's on the
																										 // left).
	bottomRight[1] = longitude(bottomRight[1]);	// We convert the longitude after setting the distance
																							// because this is less critical than the latitude
																							// for matching the ship's heading. Locations aren't
																							// exact using this method (if that wasn't obvious).
	
	// Convert the top left corner to latitude and longitude.
	topLeft[0] = latitude(topLeft[0]);
	topLeft[1] = longitude(topLeft[1]);

	// Our strategy for generating the world file numbers is to initially produce a grid of
	// where the pixels represent latitude and longitude as though the transect were in a 
	// perfectly north-south direction, and then rotate it with the ship's direction.
	double slopeY = (topLeft[0] - bottomRight[0])/(startLine - endLine);
	double slopeX = (topLeft[1] - bottomRight[1])/(0 - maxDist/19); // These slopes relate latitude
																																	// and longitude to pixel locations.

	double F = topLeft[0]; // These are the intercepts. In the world file F is the Y location of 
	double E = topLeft[1]; // the top left and E is the X location of the top left. This is also
												 // the origin of the rotation.

	// Apply the rotation.
	double A = slopeX * cos(direction);
	double B = slopeX * sin(direction);
	double C = slopeY * sin(direction) * -1;
	double D = slopeY * cos(direction);

	// Output the world file data;
	tfw << A << "\n" << B << "\n" << C << "\n" << D << "\n" << E << "\n" << F;

	// Housekeeping
	delete[] topLeft;
	delete[] bottomRight;
	tfw.close();

	// Send an error if the GPS screwed up. It happens a lot. The world file is still output
	// Maybe this shouldn't be the case.
	if(abs(bottomRight[1] - topLeft[1]) > 1000 || abs(bottomRight[0] - topLeft[0]) > 1000) {
		cout << "Error producing world file. ";
		return false;
	}

	return true;
}

bool boatPathCSV(unsigned char* sonData, unsigned int* lineStarts, int count) {
	// Writes a CSV containing the boat's location and the depth from each ping.
	// Useful for checking the output and doing bathymetry work.
	ofstream csv("shipPath.csv");
	if(!csv.is_open()) return false;
	csv.setf(ios_base::fixed);
	csv.precision(20);
	csv << "Number,Latitude,Longitude,Depth\n";
	for(int i = 0; i < count; i++) {
		int easting = decodeUTM(sonData, lineStarts[i] + EASTINGLOC);
		int northing = decodeUTM(sonData, lineStarts[i] + NORTHINGLOC);
		float depth = decodeUTM(sonData, lineStarts[i] + DEPTHLOC)/10;
		double lat = latitude(northing);
		double lon = longitude(easting);
		csv << i << "," << lat << "," << lon << "," << depth << "\n";
	}
	csv.close();
	return true;
}

bool processSideScan (string filename, bool writeCSV, bool writeTIFF, 
											bool pathAndDepth, int tolerateAngle, int maxLength, 
											int minLength, bool port) {
	streampos size;
	char* sFileData;

	ifstream sonFile(filename,ios::in | ios::binary | ios::ate);
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

	int count = countLines(fileData, (int)size);
	bool* breaks = new bool[count];
	breakOnDirectionChange(fileData, breaks, count, (uint32_t)tolerateAngle);
	breakOnNoPosition(fileData, breaks, count);
	breakOnMaxLines(breaks, count, maxLength);
	int bpCount = 0;
	for(int i = 0; i < count; i++) {
		if(breaks[i]) bpCount++;
	}

	unsigned int* LineStarts = lineStarts(fileData, (int)size, count);
	int* breakPoints = new int[bpCount];
	int bpCurr = 0;
	for(int i = 0; i < count; i++) {
		if(breaks[i]) {
			breakPoints[bpCurr] = i;
			bpCurr++;
		}
	}
	delete[] breaks;

	unsigned int lineLen = 0;
	for(int i = 0; i < count - 1; i++) {
		if(LineStarts[i+1] - LineStarts[i] > lineLen) 
			lineLen = LineStarts[i+1] - LineStarts[i];
	}

	for(int i = 0; i < bpCurr - 1; i++) {
		if(!writeCSV) break;
		bool outputCSV = generateSideScanCSV(fileData, LineStarts, lineLen, 
																				 breakPoints[i], breakPoints[i+1], 
																				 port);
		if(!outputCSV) {
			cout << "Error outputting CSV for line: " << breakPoints[i];
			cout << " to " << breakPoints[i+1] << "\n";
		}
	}

	for(int i = 0; i < bpCurr - 1; i++) {
		if(!writeTIFF) break;
		bool outputTIFF = generateTIFF(fileData, LineStarts, lineLen, breakPoints[i],
																	 breakPoints[i+1], port, minLength);
		if(!outputTIFF) {
			cout << "Error outputting TIFF for line: " << breakPoints[i];
			cout << " to " << breakPoints[i+1] << "\n";
		}
	}

	if(pathAndDepth) {
		bool boatPath = boatPathCSV(fileData, LineStarts, count);
		if(!boatPath) cout << "Error outputting boat path.\n";
	}
	
	delete[] fileData;
	delete[] LineStarts;
	delete[] breakPoints;
	return true;
}
