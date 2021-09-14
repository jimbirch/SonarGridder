# SonarGridder
A C++ program for simple georeferencing of Humminbird side scan sonar files. Intended for visual substrate classification using side scan sonar records and a GIS program of your choice. Outputs (regular) TIFF files with world files indicating how to stretch and rotate them onto the map. Corrects cross-track distance and poor gps resolution. Tested with the Humminbird 598ci HD, will likely work with other units of the same generation but may require some modification to the header to adjust the sonar header length and the offsets of particular important sonar header values. TIFF files import into QGIS without difficulty, I have not tested them with any other software.

The approach is to divide the boat's track into segments where the heading is relatively constant, correct the across track distances, write a TIFF file containing the data within each segment, and then write a world file (TFW) describing how to stretch and rotate the image to a map. This approach improves legibility over gridding methods dependent on (GPS-derived) boat heading and radiometric correction at the expense of some spatial accuracy.

This software is a hobby project that I've made available under the GNU GPL version 2.0. It is provided WITHOUT WARRANTY and is probably absolutely loaded with potentially dangerous bugs (I am a biologist after all). Feel free to copy, distribute, or modify the code to your own ends under the terms of the version 2.0 of the GNU GPL. Please see the attached LICENSE file for more information. If you use this software in your own work, please consider dropping me a citation!

Some notes on how this software works:
My study involved surveying an inland waterway with a Humminbird 598ci HD fishfinder. This unit was chosen largely because it is side scanning (side imaging to borrow the manufacturer's parlance) and was the smallest available unit at the time (it was often portaged or run through rapids on a canoe or small aluminum boat, weight mattered). Depth and range are calculated in this unit using data hard-coded into the firmware allowing the user to select seawater or fresh water. The speed of sound for fresh water used by the manufacturer is (I believe) 1436 m/s. Per the manufacturer's specifications, the true RMS power output is 500 watts and the transducer package contains 2 455 khz "side-imaging" transducers (one per side), 1 200 khz high resolution "down-imaging" transducer, and 1 83 khz traditional fish finder transducer. I have configured this software to work with this specific unit in fresh water, but will include documentation on how to change these values at the end of this file.

Using: 
This software runs on the command line and outputs (a ton of) files directly to the folder it is run from. I recommend making a new folder to hold these files and running sonargridder from inside that folder. From the command line:

Synopsis: 

  sonargridder [filename] [transducer] [options]
  
Filename:

  The .SON file from a recording. On my unit (Humminbird 598ci HD), port side files are typically B002.SON while starboard side files are typically B003.SON.
  
  
Transducer:

  The transducer whose output corresponds to the file, should be starboard or port (all lowercase). See filename for typical filenames for each side.


Options:

  -notiff or -nt: Disable georeferenced TIFF output
  
  -nopath or -np: Disable CSV output of the ship's path
  
  -a [number]: Specify the maximum heading change in tenths of a degree (default 100).
  
  -max [number]: Specify the maximum number of scans to include in a TIFF file (default 100).
  
  -min [number]: Specify the minimum number of scans to include in a TIFF file (default 10). Must be greater than 1. Files with fewer scans than specified (due to frequent direction changes or broad turns for example) will not be output.
  

Example:

  sonargridder B002.SON port -a 2000 -max 300 -min 10
  
    Will output TIFF files with a maximum vertical resolution of 300 and minimum vertical resolution of 10 for the port side recording. Heading changes of less than 30° will be ignored.
    
  sonargridder B003.SON starboard -nopath -a 100 -max 200 -min 2
  
    Will output TIFF files with a maximum vertical resolution of 200 and minimum vertical resolution of 2 for the starboard side recording. Heading changes of less than 20° will be ignored.
    

Outputs:

  All coordinates are given in (roughly) WGS84 (EPSG:4326)
  
  [Port/Starboard]_lines[start]to[end].tif : 
  
    vanilla (ungeoreferenced) tiff files containing the scans from the start to the end of a block.
    
  [Port/Starboard]_lines[start]to[end].tfw :
  
    World files (text) georeferencing the tif files of the corresponding name, allowing a suitable GIS program to stretch and rotate the tiff files into place.
    
  shipPath.csv :
  
    A csv file containing the latitude and longitude of each scan, as well as the depth.
    

Suggested workflow:

  Managing files will be easier if you create a new folder for the program's output. For example:
  
    mkdir Lake\ Superior\ Transect\ 1\ Starboard
    cd Lake\ Superior\ Transect\ 1\ Starboard
    cp ../B003.SON ./
    sonargridder B003.SON starboard -nopath -a 100 -max 200


Building:

  Pre-requisites: This code requires only libtiff. If it is not installed, please install it first. In ubuntu:
    sudo apt-get install libtiff-dev
    
  1. Download the repository.

  2. Move into the src directory
 
     cd src
     
  3. If necessary change LD_LIBRARY_PATH in the makefile to the path for libtiff (the path included is valid in the Ubuntu 20.04 x86-64 distribution I'm working from)
  
     find /usr -name tiffio.h
     
     sed -i "s+/usr/include/x86-64-linux-gnu+[location of tiffio.h]+g"
     
  4. Build the program
 
     make
  
  The program will be output to ../bin


Installing:

  I've included an install directive in the Makefile that will dump the binary into /usr/local/bin (and an uninstall directive that will remove it). I'm not sure I'd recommend this course of action, but if you want to do it:
  
  sudo make install
  
  
  Alternatively (suggested) if you want to keep it local or don't have root access, make a folder to add to $PATH and copy the binary file there:
  
  1. mkdir ~/bin (if it doesn't exist)
  
  2. cp ../bin/sonargridder ~/bin/
  
  3. export PATH=$PATH:~/bin

  
  Alternatively alternatively add the bin folder from the repository to your PATH variable.
