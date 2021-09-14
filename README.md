# SonarGridder
A C++ program for simple georeferencing of Humminbird side scan sonar files. Intended for visual substrate classification using side scan sonar records and a GIS program of your choice. Outputs (regular) TIFF files with world files indicating how to stretch and rotate them onto the map. Corrects cross-track distance and poor gps resolution. Tested with the Humminbird 598ci HD, will likely work with other units of the same generation but may require some modification to the header to adjust the sonar header length and the offsets of particular important sonar header values. TIFF files import into QGIS without difficulty, I have not tested them with any other software.

The approach is to divide the boat's track into segments where the heading is relatively constant, correct the across track distances, write a TIFF file containing the data within each segment, and then write a world file (TFW) describing how to stretch and rotate the image to a map. This approach improves legibility over gridding methods dependent on (GPS-derived) boat heading and radiometric correction at the expense of some spatial accuracy.

This software is a hobby project that I've made available under the GNU GPL version 2.0. It is provided WITHOUT WARRANTY and is probably absolutely loaded with potentially dangerous bugs (I am a biologist after all). Feel free to copy, distribute, or modify the code to your own ends under the terms of the version 2.0 of the GNU GPL. Please see the attached LICENSE file for more information. If you use this software in your own work, please consider dropping me a citation!

########################################################################################################################

Some notes on how this software works:
My study involved surveying an inland waterway with a Humminbird 598ci HD fishfinder. This unit was chosen largely because it is side scanning (side imaging to borrow the manufacturer's parlance) and was the smallest available unit at the time (it was often portaged or run through rapids on a canoe or small aluminum boat, weight mattered). Depth and range are calculated in this unit using data hard-coded into the firmware allowing the user to select seawater or fresh water. The speed of sound for fresh water used by the manufacturer is (I believe) 1436 m/s. Per the manufacturer's specifications, the true RMS power output is 500 watts and the transducer package contains 2 455 khz "side-imaging" transducers (one per side), 1 200 khz high resolution "down-imaging" transducer, and 1 83 khz traditional fish finder transducer. I have configured this software to work with this specific unit in fresh water, but will include documentation on how to change these values at the end of this file.

########################################################################################################################

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
    
########################################################################################################################

Suggested workflow:

  Managing files will be easier if you create a new folder for the program's output. For example:
  
    mkdir Lake\ Superior\ Transect\ 1\ Starboard
    cd Lake\ Superior\ Transect\ 1\ Starboard
    cp ../B003.SON ./
    sonargridder B003.SON starboard -nopath -a 100 -max 200

########################################################################################################################

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

########################################################################################################################

Installing:

  I've included an install directive in the Makefile that will dump the binary into /usr/local/bin (and an uninstall directive that will remove it). I'm not sure I'd recommend this course of action, but if you want to do it:
  
  sudo make install
  
  
  Alternatively (suggested) if you want to keep it local or don't have root access, make a folder to add to $PATH and copy the binary file there:
  
  1. mkdir ~/bin (if it doesn't exist)
  
  2. cp ../bin/sonargridder ~/bin/
  
  3. export PATH=$PATH:~/bin

  
  Alternatively alternatively add the bin folder from the repository to your PATH variable.
  
########################################################################################################################
########################################################################################################################
########################################################################################################################

Changing SONAR parameters:

All parameters relevant to the analysis being done are provided as macros in the header file (songridder.h). Change these before compiling the software to change how the analysis is done. I have divided the macros into three sections consisting of physical constraints, properties of the down imaging transducer (this is only important for the extremely experimental E1 and E2 analysis included that I have not yet documented), and properties of the file.

########################################################################################################################

To change to seawater:

The unit's firmware uses the speed of sound to calculate the depth and range, and has one of two values built in depending on whether the user specified that the unit is being used in fresh water or sea water. I believe these values are 1463 m/s in fresh water and 1500 m/s in seawater (but don't quote me on that). This results in a 2.5% difference between distances calculated in fresh water and those in seawater. Since we rely on the flat bottom assumption and make a lot of spatial compromises here, you can probably get close enough in seawater to the distances for fresh water the default configuration provides. You can also change it.

To change the software to seawater, SOUNDSPEED and SAMPLESPERMETER must be changed. The speed of sound used in fresh water is 1463 (m/s). I believe Humminbird uses 1500 (m/s) as the speed of sound in seawater (again, don't quote me on that). SAMPLESPERMETER corrects the sampling frequency to the distance and is defined as one over the length of each sample in seconds times the speed of sound (or the sampling rate in Hz divided by the speed of sound). Since the sampling frequency (likely constrained by some DAC on the device's mainboard) is undocumented, I determined this by taking the number of samples in a given line and dividing it by the specified range in my transect (50 m) to get 54.4245. This corresponds to a sampling rate of ~79.6 kHz (79623.0435 Hz) or a sample length of 12.56 μs (0.000012559 s). In seawater (1500 m/s) SAMPLESPERMETER would be 53.08278.

Changing to a non-firmware-specified speed of sound is beyond the scope of this how-to, but can likely be accomplished by changing the SAMPLESPERMETER macro definition to what would be correct for the waterbody you are working in and adjusting the depth prior to using it in any of the functions. Depth would be adjusted as follows: 

  Dc = (D1 * Fs / C1) / (Fs / C2)
  
  Where D1 is the depth reported by the unit, C1 is the speed of sound used by the unit, C2 is the actual speed of sound, and Dc is the corrected depth.

########################################################################################################################

To change to a different unit:

BRAVE SOUL: This is possible. Very doable even. You will need a hex editor (I use ghex, others are probably fine) and recordings from your particular device. I would first check to see if the offsets built in for this unit work (I have only tested it with a Humminbird 598ci HD combo, but it may work with more ~2014 x98ci HD Combo units). If that fails, your procedure follows.

Humminbird's SON files, to my knowledge, are all laid out in the same basic structure. They are binary files that are appended to every time the unit scans. Each record (here referred to as a line) begins with a start sequence (in this unit 0xC0DE21), followed by a short header containing information about the scan (position in world mercator meters, depth, heading, number of records in the line, etc) and then a long string of bytes containing the return strength. The header information and unit specs may vary from unit to unit (I expect this structure is common to units within generations, mine is a ~2014 Humminbird 598ci HD combo). The relevant information in the header macros is the location of the sentence length (unsigned, 32 bit; LENPOS), the location of the world mercator northing value (signed 32 bit; NORTHINGLOC), the location of the world mercator easting value (signed 32 bit; EASTINGLOC), the location of the depth (unsigned 32 bit, decimeters; DEPTHLOC), sentence length (unsigned 32 bit; LENLOC), heading (unsigned 16 bit, tenths of a degree; HEADINGLOC), and the length of the ping header in bytes (including start sequence; HEADINGLOC). 

To determine the structure of your particular unit's files, load one up in a hex editor (side imaging may differ in structure from down imaging, so use B002.SON or B003.SON). If your hex editor can indicate multibyte integers, make sure it's configured for big endian. Unsigned values are 2's compliment. The start sequence will be the first four bytes (if this is not 0xC0DEAB21, you will need to change parts of the code in fileprocessing.cpp). 

LENPOS: 
1. Search for the second occurance of the start sequence and record the offset of the first byte. 
2. Sentence length for the first line will be S = Oe - H where Oe is the end of the first line (the offset of beginning of the second line minus 1) and H is the header length. 
3. Since we don't know the header length yet, search for a number similar to the second offset minus 1, record its offset. This is LENPOS.

HEADERLEN:
1. The header length should be (but sometimes is not) the length of the line (the offset of the second occurance of the start sequence minus one) minus the sentence length. In this generation of unit it is 67.
2. Check the byte prior to the offset of the calculated header length for the scan start byte. On this unit the scan start byte is 0x21. Also check the offset of the line start sequence + the offset of the scan start byte elsewhere in the file to make sure this remains the same.
3. If these conditions are met, you have determined HEADERLEN.

EASTINGLOC:
1. This value is harder to identify, and it's extremely helpful to know the world mercator location of a scan (although for some reason this is not identical to EPSG:3395 (may be EPSG:3857). Look for a signed 32 bit integer corresponding to something that seems reasonable. Also try scans other than the first scan as this is dependent on the GPS being warmed up.
2. On my unit the byte prior to the easting value is always 0x82.
3. Record the offset if you found it as EASTINGLOC.

NORTHINGLOC:
1. On my unit this is separated from the bytes corresponding to EASTINGLOC by a spacer byte (0x83). Check the offset at EASTINGLOC + 5 to see if the offset contains a signed 32 bit integer that seems reasonable for a northing value in world mercator units. Otherwise, check other locations in the header. Also check scans other than the first given that the GPS needs to warm up.
2. Record the offset if you found it as NORTHINGLOC.

DEPTHLOC:
1. If you wrote down the depth when you started recording you are already halfway there. Multiply the recorded depth (in metres) by 10 to get the depth in decimetres.
2. Look for an unsigned 32 bit integer representing the depth (or something similar; or if you don't know the depth, something that seems reasonable) in decimetres (tenths of a metre).
3. Record this offset as DEPTHLOC.

HEADINGLOC:
1. Look for an unsigned 16 bit integer that seems reasonable for a heading in tenths of a degree (should be a value between 0 and 3599 at every corresponding offset in the file). It might be tempting to use the northing and easting values to calculate this, but keep in mind that these are rounded to the nearest metre and the pings are frequent, you may need to skip a few scans to get an accurate idea of heading).
2. Record this offset as HEADINGLOC.

If you've gone through the binary file successfully, congratulations! Save and recompile the program (make clean && make). If LENPOS is incorrect, the program will segfault while running. If EASTINGLOC or NORTHINGLOC are wrong, positions will be nonsensical. If DEPTHLOC is wrong, recorded depths will be nonsensical as will cross-track distances (If range is ~50 m, the TIFF files should be at most 50 m wide). If HEADINGLOC is wrong, the program may divide the track in weird ways (eg skipping turns or fragmenting the course in weird places). You may also need to change SAMPLESPERMETER (see "To change to seawater" above) and the physical properties of the down imaging transducer if you are using the extremely experimental parts of the program.
