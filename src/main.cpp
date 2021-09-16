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
║Jim Birch (2021). SonarGridder: A utility for georeferencing consumer grade   ║
║side-scan sonar files. URL https://angularfish.net                            ║
╟──────────────────────────────────────────────────────────────────────────────╢
║Please let me know if you find this software useful! jim[át]jdbirch.com       ║
║                 (Or if you can't get it to run at all)                       ║
╚══════════════════════════════════════════════════════════════════════════════╝
*/

#include "songridder.h"
#ifdef EXPERIMENTAL
#include "acousticanalysis.h"
#endif

void helpme() {
  cout << "SON Gridder v 0.1 by Angular Fish\n";
#ifdef EXPERIMENTAL
  cout << "songridder <filename> <port|starboard|down> ";
  cout << "[-notiff|-nopath|-a 100|-max 100|-min 10]\n\n";
  cout << "Mandatory:\nfilename = a Humminbird .SON file\n";
  cout << "type: port or starboard or down\n";
  cout << "  For sidescan indicate port or starboard transducer. ";
  cout << "For downward looking transducer type down.\n";
  cout << "Optional (Sidescan only): \n";
#endif
#ifndef EXPERIMENTAL
  cout << "sonargridder <filename> <port|starboard> ";
  cout << "[-notiff|-nopath|-a 100|-max 100|-min 10]\n\n";
  cout << "Mandator:\nfilename = a Humminbird .SON file\n";
  cout << "type: port or starboard\n";
  cout << "Optional: \n";
#endif
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
#ifdef EXPERIMENTAL
    bool e1e2 = e1e2File(filename);
    if(!e1e2) {
      cout << "Error outputting e1e2 file for " << filename << "\n";
    }
#endif
#ifndef EXPERIMENTAL
    helpme();
    return 0;
#endif
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
      }  else if(argv[i] == "-max") {
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
