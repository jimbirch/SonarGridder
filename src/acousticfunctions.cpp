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
║Disclaimer: I wrote this for my own use and am making it publicly available in║
║case anyone else finds it helpful. I make no guarantees about it's suitability║
║for your work. These analyses are immature, buggy, and generally not ready for║
║prime time. They were also written by a biologist who isn't an expert in acou-║
║stics or software design. If you run them they may break your computer, break ║
║my computer, delete all of your data, call your mother and tell her you are m-║
║oving home, or just not produce any usable information whatsoever. These func-║
║tions are not used to generate side-scan TIFFs, so if you only plan on visual-║
║ly classifying substrate, I strongly suggest you stop reading here and build  ║
║the program in its normal configuration. Building this version will at-best   ║
║not impact your analysis and at-worst cause you a massive headache.           ║
║                                                                              ║
║Continue at your own risk. Back up your data often, read the EXPERIMENTAL tex-║
║t file and understand the suitability of these functions for what you are pla-║
║nning to do. I will update that file if and when changes are made to these fu-║
║nctions.                                                                      ║
╟──────────────────────────────────────────────────────────────────────────────╢
║Files:                                                                        ║
║acousticanalysis.h:    Physical metadata for the down-imaging transducer,     ║
║                       prototypes for acoustic analysis functions.            ║
║acousticfunctions.cpp: Acoustic analysis functions and data processing.       ║
║main.cpp:              Command line invocation, help.                         ║
╟──────────────────────────────────────────────────────────────────────────────╢
║Extremely experimental sonar analysis conducted on the down-imaging           ║
║Transducer. Some code based on descriptions of E1 and E2 analysis from Barb   ║
║Faggetter (Oceanecology.ca) and Dan Buscombe (dbuscombe-usgs.github.io).      ║
║Please see the text file: EXPERIMENTAL for more information and suggested rea-║
║ding.                                                                         ║
╟──────────────────────────────────────────────────────────────────────────────╢
║Tested with data produced by a Humminbird 598ci HD manufactured circa 2014.   ║
║May work with other units with or without modification.                       ║
╟──────────────────────────────────────────────────────────────────────────────╢
║   Please let me know if you find this software useful! jim[át]jdbirch.com    ║
║                 (Or if you can't get it to run at all)                       ║
╚══════════════════════════════════════════════════════════════════════════════╝
*/

#include "songridder.h"
#include "acousticanalysis.h"

// Global variables
uint32_t dt_freq = FREQUENCY;
double beam = BEAM;
double lambda = WAVELENGTH;
double coeff_return = RETURNSTRENCOEFF;
uint16_t watts_max = MAXW;
double duration_ping = PINGDURATION;

double waterAttenuation(double H, double pH, int T, double S) {
  int f = dt_freq / 1000;
  double A1 = (8.86/speed_sound) * pow(10, 0.78 * pH - 5);
  double f1 = 2.8 * sqrt(S/35) * pow(10, 4 - 1245/(T + 273));
  double A2 = 21.44 * S * (1 + 0.025 * T) / speed_sound;
  double A3 = 0.0003964 - 0.00001146 * T + 0.000000145 * pow(T, 2) 
              - 0.00000000065 * pow(T, 3);
  double f2 = 8.17 * pow(10, 8 - 1990/(T + 273)) / (1 + 0.0018 * (S - 35));
  double P2 = 1 - 0.000137 * H + 0.0000000062 * pow(H, 2);
  double P3 = 1 - 0.0000383 * H + 0.00000000049 * pow(H, 2);
  double alphaw = A1 * f1 * pow(f, 2) / (pow(f, 2) + pow(f1, 2))
                  + A2 * P2 * f2 * pow(f, 2)/(pow(f,2) + pow(f2, 2))
                  + A3 * P3 * pow(f, 2);
  return alphaw / 500;
}

double returnCoeff (unsigned char* line, int loc, double depth) {
  double returnStrength = line[loc] + waterAttenuation(depth, 7.0, 25, 0.0) / 
                          coeff_return; // Instead of correcting the return to 
                                        // dB-W, we correct the attenuation to
                                        // return units so that this can be 
                                        // removed if necessary.
  double returnStren = pow(10,returnStrength/255); // Linearizes the (presumed) 
                                                   // logarithmic return strength
                                                   // to a number between 1 and 10
  return returnStrength;
}

double calcnoise(unsigned char* line, int end) {
// Calculates the noise (the average return energy in the water column part of
// the the farfield region). Takes the line and the offset of the first location 
// of the first echo. Returns the average noise energy.
  if(end < length_header + 108) return 0;
  int startSample = length_header + 108; 
  double depth = (end - length_header) / count_samples_meter;
  int endSample = length_header + (end - length_header) * 0.9;
  double coefficients = 0;
  for(int sample = startSample; sample < endSample; sample++) {
    coefficients += returnCoeff(line, sample, depth)/(endSample - startSample);
  }
  return coefficients;
}

double calcE(unsigned char* line, int start, int end, int echo1Start) {
// Calculates E1 or E2. Takes a line, a start offset, and an end offset for both
// the peaks, as well as the start offset for the first echo. Returns the energy 
// of the first (E1) or second (E2) echo.
  double depth = (echo1Start - length_header) / count_samples_meter;
  double E = 0;
  for(int sample = start; sample < end; sample++) {
    E += returnCoeff(line, sample, depth);
  }
  E *= 2;
  double noise = calcnoise(line, echo1Start);
  int n = end - start;
  double roughnessHardness = E - n * noise;
  return roughnessHardness;
}

double getDepth(uint32_t* echos, uint8_t* flag) {
  // Calculates the depth from the 4 element echo array. Takes a 4 element uint32_t
  // array structured as: echo 2 start, echo 2 end, echo 1 start, echo 1 end; and
  // a flag variable. Returns a double of the depth in metres. Sets bit 3 of the flag
  // variable if the peaks aren't the main echos.
  uint32_t echo = echos[4];
  double depth = (echo - length_header) / count_samples_meter;
  if(depth < 1 || depth > 200) *flag |= 0b00100000;
  return depth;
}

void addDepthToPeaks(unsigned char* peaks, double depth, uint32_t ll,
                     uint8_t* flag) {
  // Adds the region surrounding the unit's detected depth to the peaks.
  // Specify depth as a double in meters. Will mangle the peaks array to
  // include the depth peak.
  unsigned int depthBin = length_header + depth * count_samples_meter;
  unsigned int startSearch = length_header;
  unsigned int endSearch = ll - 1;
  if((depthBin - count_samples_meter) >= length_header) startSearch = 
                                              depthBin - count_samples_meter;
  else *flag |= 0b00010000;
  if((depthBin + count_samples_meter) < ll) endSearch = startSearch + count_samples_meter;
  else *flag |= 0b00001000;
  for(unsigned int i = startSearch; i < endSearch; i++) {
    peaks[i] = 0xFF;
  }
}

void getEchos(unsigned char* peaks, unsigned int* echos, unsigned char* line,
              uint32_t ll, uint8_t* flag) {
  // Finds the start and end points for the two echos. Takes the line, the array
  // of peaks, and the length of the line. Returns an array of 4 uint32_t containing
  // the offsets in the order: second echo start, second echo end, first echo start,
  // first echo end. Major issues are ranges that are too short or too long (ie: There
  // is only one echo or there are more than two).

  // Step 1: find the depth ping. We will consider this the highest return in a peak.
  uint8_t maxReturn = 0;
  unsigned int depthPing = 0;
  for(unsigned int i = length_header + 27; i < ll; i++) {
    if(peaks[i] == 0xFF && line[i] > maxReturn) {
      maxReturn = line[i];
      depthPing = i;
    }
  }

  // Step 2: find the start of the second echo. We will consider this the
  // highest return around double the distance from the first peak.
  maxReturn = 0;
  unsigned int secondPing = 0;
  unsigned int startSearch = 1.8 * (depthPing - length_header) + length_header;
  if(startSearch >= ll) startSearch = ll - 11;
  unsigned int endSearch = 2.2 * (depthPing - length_header) + length_header;
  if(endSearch - 10 < startSearch) endSearch += 10;
  if(endSearch >= ll) endSearch = ll - 2;
  for(unsigned int i = startSearch; i < endSearch; i++) {
    if(line[i] > maxReturn) {
      maxReturn = line[i];
      secondPing = i;
    }
  }

  // Step 3: find the local minimum ahead of the second echo (or behind if
  // the water is super shallow).
  int8_t incr = 1;
  if(secondPing < depthPing + 336) incr = -1;
  endSearch = secondPing + 54 * incr;
  if(endSearch > ll-2) endSearch = ll-2;
  else if(endSearch <= length_header) endSearch = length_header;
  maxReturn = 255;
  unsigned int localMin = 0;
  for(unsigned int i = secondPing; i != endSearch; i += incr) {
    if(line[i] < maxReturn) {
      localMin = i;
      maxReturn = line[i];
    }
  }

  // Step 4: find the noise floor for the metre preceding the local minimum
  // or following if the water is super shallow.
  unsigned int noiseFloor = 0;
  endSearch = localMin + incr * 54;
  if(endSearch > ll - 2) endSearch = ll - 2;
  else if(endSearch < length_header) endSearch = length_header;
  for(unsigned int i = localMin; i != endSearch; i += incr) {
    noiseFloor += line[i] / 54;
  }
  noiseFloor += noiseFloor * 0.1;

  // Step 5: identify the start points for both pings using the local noise
  // floor.
  unsigned int depthPingStart = 0;
  endSearch = depthPing - 336;
  if(depthPing < length_header + 336) endSearch = length_header;
  
  for(unsigned int i = depthPing; i >= endSearch; i--) {
    if(i <= length_header) { 
      depthPingStart = length_header;
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
  endSearch = depthPing + 336;
  if(endSearch >= secondPingStart) endSearch = secondPingStart - 10;
  if((endSearch < depthPing) || (endSearch > ll - 2)) endSearch = ll - 2;
  for(unsigned int i = depthPing; i < endSearch; i++) {
    if(line[i] < noiseFloor) {
      depthPingEnd = i;
      break;
    }
  }

  unsigned int secondPingEnd = 0;
  endSearch = secondPing + 336;
  if(endSearch > ll - 2 || endSearch < depthPingEnd) endSearch = ll - 2;
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

  // Flag if something weird happened.
  for(int i = 0; i < 5; i++) {
    if((echos[i] < length_header) || (echos[i] > ll + 2)) *flag |= 0b10000000;
  }
  if(echos[0] <= echos[3]) *flag |= 0b01000000;
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
  for(unsigned int i = length_header + 108; i < ll; i++) {  
    uint8_t mean = 0;
    for(unsigned int j = i - 13; j < i; j++) { // 13 ~ 25 cm in fresh water.
      mean += line[j]/13;
    }
    uint32_t ustdvar = 0;
    for(unsigned int j = i - 13; j < i ; j++) {   // Calculate σ
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

  /*for(int i = ll - 14; i >= length_header + 108; i--) {
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
        if(finder == 1 && peaks[0] < 255)  peaks[0]++; // peaks[0] is zero until we
        if(tracker > peaks[3] && tracker < 256) peaks[3] = tracker;
      }                                               // add the count information.
      offcounter = 0;
    }  else {
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

double lineWattdBWCorr(unsigned char* line, uint32_t ll) {
// Attempts to fit an individual line to a model of water attenuation to
// make a stab at finding the conversion to decibel watts.
  double correction = 1;
  double depth = decodeDepth(line, offset_depth) / 10;
  unsigned int binDepth = int(depth * count_samples_meter + length_header + 0.5);
  if(ll < binDepth + 65 || length_header > binDepth - 65) return 0.0;
  // Find the maximum return
  uint8_t maxReturn = 0;
  for(unsigned int i = binDepth - 65; i < binDepth + 65; i++) {
    if(line[i] > maxReturn) maxReturn = line[i];
  }
  if(maxReturn == 255) return 0.0;

  // Find the predicted attenuation
  double atten = waterAttenuation(depth, 7.0, 25, 0.0);
  double Rcorr = atten / (255 - maxReturn);
  return Rcorr;
}

double guessdBWCorr(unsigned char* file, uint32_t maxLL, 
                    unsigned int* lineStarts, uint32_t count) {
// Makes a wild guess as to the correction to decibel watts. Not sure if it works
  double sumdBWattCorr = 0;
  unsigned int actualSamples = 0;
  unsigned char* line = new unsigned char[maxLL];
  for(uint32_t i = 0; i < maxLL; i++) line[i] = 0x00;
  for(unsigned int i = 0; i < count - 1; i++) {
    getLine(file, line, lineStarts[i], lineStarts[i+1]);
    double received = lineWattdBWCorr(line, lineStarts[i+1] - lineStarts[i]);
    if(received > 0.001) {
      cout << "Received conversion: " << received << "\n";
      sumdBWattCorr += received;
      actualSamples++;
    }
  }
  if(actualSamples < 1) {
    cout << "decibel watts conversion failed.\n";
    return 10000; // Return a number that removes water attenuation IFF failure
  }
  delete[] line;
  double averageConversion = sumdBWattCorr / actualSamples;
  double maxReturn = 255 * averageConversion;
  cout << "Best guess for transducer saturation is: ";
  cout << maxReturn << " dB-Watts\n";
  cout << "Average conversion to dB-Watts is: " << averageConversion << "\n";
  return averageConversion;
}

bool e1e2File (string filename, bool guessCoeff) {
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

  // Guess the decibel watts conversion
  if(guessCoeff) coeff_return = guessdBWCorr(fileData, maxLL, beginnings, count);
  else coeff_return = 10000;

  // Output a line as its own csv file
  srand (time(NULL));
  uint32_t midpoint = rand() % count;
  string message = "No flags :)\n";

  unsigned char* line = new unsigned char[maxLL];
  for (unsigned int i = 0; i < maxLL; i++) line[i] = 0;
  getLine(fileData, line, beginnings[midpoint], beginnings[midpoint+1]);
  unsigned char* peaks = new unsigned char[maxLL];
  detectPeaks(line, peaks, beginnings[midpoint+1] - beginnings[midpoint]);
  unsigned char* line2 = new unsigned char[maxLL];
  for (unsigned int i = 0; i < maxLL; i++) line2[i] = 0;
  double depth = 0;
  depth = (double)decodeDepth(line2, DEPTHLOC)/10;
  addDepthToPeaks(peaks, depth, beginnings[midpoint+1] - beginnings[midpoint], &flag);
  getLine(fileData, line2, beginnings[midpoint], beginnings[midpoint+1]);
  uint32_t* echo = new uint32_t[5];
  getEchos(peaks, echo, line2, beginnings[midpoint+1] - beginnings[midpoint], &flag);
  depth = getDepth(echo, &flag);
  if((flag & FLAGDEPTHNONSENSE) > 0) {
    depth = (double)decodeDepth(line2, DEPTHLOC)/10;
    message = "Depth unable to be detected from echos flag code: ";
    message += to_string(flag) + "\n";
  }
  double noise = calcnoise(line2, echo[2]);
  double E1 = calcE(line2, echo[2], echo[3], echo[2]);
  double E2 = calcE(line2, echo[0], echo[1], echo[2]);
  double attenuation = waterAttenuation(depth, 7, 25, 0);
  double lat = getLatitude(line, offset_northing);
  double lon = getLongitude(line, offset_easting);
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

  int j = 0;

  unsigned int guess = 0;
  for(unsigned int i = length_header; i < beginnings[midpoint+1] - beginnings[midpoint] - 28; i++) {
    if(i > length_header + 3) j = i + peaks[2]/7 + 13;
    else j = i - length_header;
    if(((i >= echo[0]) && (i <= echo[1])) || 
       ((i >= echo[2]) && (i <= echo[3]))) guess = 0xFF;
    else guess = 0x00;
    lineCSV << i << "," << (int)line2[i] << "," << (int)line[j] << "," << (int)peaks[j];
    lineCSV << "," << guess << "\n";
  }
  delete[] line2;

  ofstream csv("e1e2.csv");
  if(!csv.is_open()) {
    cout << "Error writing csv file. Aborting.\n";
    return false;
  } 
  csv << "Number,Flag,Depth,Latitude,Longitude,E1,E2\n";
  //csv.setf(ios_base::fixed);
  //csv.precision(10);
  //unsigned char* line;
  for(unsigned int i = 0; i < count - 1; i++) {
    flag = 0;
    uint32_t ll = beginnings[i + 1] - beginnings[i];
    getLine(fileData, line, beginnings[i], beginnings[i+1]);
    detectPeaks(line, peaks, ll);
    getLine(fileData, line, beginnings[i], beginnings[i+1]);
    double depth = (double)decodeDepth(line, DEPTHLOC) / 10;
    addDepthToPeaks(peaks, depth, ll, &flag);
    getEchos(peaks, echo, line, ll, &flag);
    bool calculateE2 = true;
    bool calculateE1 = true;
    bool calculateD = true;
    if((flag & FLAGECHOSFAILED) > 0) {
      continue;
    }
    csv << i << ",";
    if(calculateD) depth = getDepth(echo,&flag); // This may or may not be important. Leaning
                                  // toward not. Might help correct for
                                  // variations in speed_sound though.
    if((flag & FLAGDEPTHNONSENSE) > 0) {
      depth = (double)decodeDepth(line, DEPTHLOC)/10;
      csv << "Depth update failed. ";
    } 
    if((flag & FLAGDEPTHDEEP) > 0) csv << "Depth within 1 m of range. ";
    if((flag & FLAGDEPTHSHALLOW) > 0) csv << "Depth less than 1 m. ";
    if( (flag & FLAGSECONDECHOFAILED) > 0) {
      csv << "No second echo detected ";
      calculateE2 = false;
    }
    if (flag == 0) csv << "No flags :)";
    csv << ",";

    
    csv << depth << ",";
    double latitude = getLatitude(line, offset_northing);
    csv << fixed << setprecision(10) << latitude << ",";
    double longitude = getLongitude(line, offset_easting);
    csv << fixed << setprecision(10) << longitude << ",";
    double E1 = 0;
    if(calculateE1) E1 = calcE(line, echo[2], echo[3], echo[4]);
    csv << E1 << ",";
    double E2 = 0;
    if(calculateE2) E2 = calcE(line, echo[0], echo[1], echo[4]);
    csv << E2 << ",";
    double Noise = calcnoise(line, echo[2]); 
    csv << Noise << "\n";
  }
  delete[] beginnings;
  delete[] fileData;
  delete[] line;
  delete[] peaks;
  delete[] echo;

  cout << "E1E2 Calculated Successfully.\n";

  return true;
}

