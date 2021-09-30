# SonarGridder: 

## A utility for georeferencing consumer-grade, recreational side-scan sonar files.

#### © 2021 Jim Birch (AngularFish.net)

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License or (at your option) any later version.

This is free software written as a hobby project by a fish biologist (read: not a sonar expert or computer scientist) during his free time during a global pandemic. I have distributed it in the hopes that it will be useful to others, but WITHOUT ANY WARRANTY; not even an the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE See the GNU GPL (the LICENSE text file included) for more details.

Need help/have help/want to talk: jim[ä]jdbirch.com

## Table of Contents

**1. PREAMBLE:**  What this is, what it's for, why I made it, standard warnings about running this code. Emphasis on the amble part.

**2. BUILDING:** Information on how to compile and install the program. Information on how to modify the program to run under different conditions or with different sonar units.                                  

**3. RUNNING:** Information on command line options for running the program. Basics for GIS import of files and tips on how to run a survey.     

**4. TIPS:** How to open these files in QGIS, how to plan a study with this instrumentation and software in mind, suggested instrument setup and boat operation, and common pitfalls

**5. SUGGESTED READING:** Peer-reviewed publications, technical documentation, and helpful tips and tricks for the discerning user who wants to actually know how to use this information

**6. POST-AMBLE?:** Good luck!

**7. CHANGELOG:** Updates to this software.

# 1. Preamble

## 1.1 How did I get here?

I am a fish biologist, with a motivating interest in fish habitat use. Circa 2014 I was a broke graduate student in a fishing supply store in a rural community in Canada. They had a Humminbird display with several units showing a running demonstration of side-scan imagery. I immediately went home, checked my bank account, and then drove back to the store and picked one up. Initially I used the unit for visual substrate classification in the boat while doing gillnet surveys, but I also have acoustic telemetry data and it sure would be cool to be able to reference that to side-scanning imagery.

I went with the 698ci HD combo because it was the smallest. It was also the cheapest, but as near as I could tell at the time the primary difference between the units is their display size. I still use it. It has been intermittently screwed into the transom of small aluminum boats, duct-taped to canoes and kayaks, and even once screwed onto a pole and held firmly to the side of a zodiac by someone who definitely wasn't paid enough for this.

At the time there were few utilities for georeferencing side-scan files. Dan Buscombe's PyHum was in its infancy (and although it seemed cool, Python dependencies vexed me at the time). Barb Faggetter (Oceanecology.ca) had a great guide on using a utility called Son2Xtf to convert files into the extended triton format, and then georeference them with MB-System (MBARI: The Monterey Bay Aquarium Research Insitute, California, USA). I captured a fair amount of data, georeferenced it in this way, and a colleague validated the method using an epibenthic sled with a mounted camera at our study site.

Eventually I was revisiting some questions about substrate and habitat use and had cause to open the rasters generated by MBSystem. I was disappointed by their illegibility—about 75% of the imagery is interpretable and, because I never turned the instrument off between transects, there are overlapping sections that are gridded together. The imagery that was so crisp and clear on the unit's display was rendered into what appeared to be static in QGIS.

The uninterpretable sections of the raster grid still bothered me. I decided to try PyHum again, but was vexed this time by trying to satisfy Python 2.7 dependencies in 2021. Why Python had to obsolete decades of other peoples' work is beyond the scope of this document, but I'll humour my bitterness in this one sentence.

## 1.2 Design compromises, research software, and mixing the two

The problem with MB-System as I see it is that it is intended to process data from side-scan and multibeam units that are substantially more sophisticated than the yee-haw grade fish-finder I was using. The general technique used in that software is to process each *scan* (set of returns associated with a single ping) semi-independently, correcting the cross-track distance of each return and then positioning it relative to the other returns using the boat's and position where the ping was recorded. I believe there is some filtering used to correct GPS locations and headings, but this is inadequate for the hardware. The transducers used in this unit seem to be of decent quality, but the positioning **sucks**. There are some major positioning-related compromises that can be summed up by saying that the recording format is intended to be replayed on the instrument, not fed into serious research software. Those compromises are:
1. GPS locations are recorded as a signed integer in World Mercator metres. You get roughly 1 metre of precision on an instrument that pings multiple times per metre (especially if you take the advice of pretty much everyone and go real slow).
2. There is no magnetometer (compass). Headings and speeds are GPS derived and the GPS is not reliable.
3. Again, the GPS is unreliable. I don't know if the antenna choice was related to assumptions about the unit being used on the ocean or otherwise open water, but it takes minutes to get a fix and god help you if you are ever under a tree.

## 1.3 Limitations, extant software, and a disclaimer about my motivation

Although my inability to use PyHum motivated my creation of this utility, recreating that project is not my goal. Dan Buscombe did excellent work and if he or someone else decides to port that software to Python 3 or a saner programming language, that is a project I would very much support. I have little interest in automatically classifying substrates. My study areas are small, and my eyes are still good. I only wanted a method to georefernce raster files from this instrument in a legible way. 

I am also a fish biologist and neither the binary files produced by this instrument nor C++ are fish. I have done my best here and put together a software package that does what I set out to do in a reasonably repeatable way, but your mileage may vary. If it meets your needs, please consider dropping me a citation. If it mangles your files, sets your computer on fire, or produces unreadable garbage. C'est la vie. Send me a bug report and I'll look into it, or update the code as you see fit. As expressed in the license there's no warranty, suitability guarantees, or commitment to support included here.

## 1.4 How this software works, or making at least as many compromises as the manufacturer

The operating principle of this software is to keep the analysis simple and make the outputs legible. As such, the only corrections are to cross-track distance. There are no radiometric corrections and there are miriad sacrifices made in spatial accuracy extending even beyond the flat-bottom assumption. The program converts groups of scans into a TIFF file and then georefences the (rectancular) files using the GPS location of (only) the first and last scans in the TIFF and the calculated cross-track distance. 

This means that, in addition to the flat-bottom assumption, this software has a straight-track assumption. The steps for generating TIFF files is as follows:
1. Open the file and locate groups of scans where the boat's path was (relatively) straight. The allowable heading deviation is user specifiable.
2. Process the cross-track distances for each return in each (relatively) straight portion of the track, output these in a (vanilla) TIFF file.
3. Calculate the transformation necessary to locate, stretch, and rotate the TIFF file to fit a map (WGS84), and save it in a world file.

This method has notable limitations:
1. The boat's track is rarely actually straight, so actual locations of objects seen are not super precise.
2. Again, the boat's track is rarely actually straight, so laying scans out perpendicularly to the straight line between two points may distort and mislocate objects on the bottom, especially when they are far from the boat.
3. Since the GPS can only position the boat on a 1 metre grid, short files cannot be reasonably interpreted. These short files are discarded. The cutoff is user specifiable but must be greater than two.
4. Scans collected during turns are almost always discarded.
5. Each individual scan is assumed to lie within an even interval between the start and end point of the file, variations in vessel speed will distort the location and presentation of data.
6. You will get a metric ton of files output. Like, so many files. Adding them to your GIS program may be time consuming and also I've found that QGIS's performance with a lot of separate raster files opened is degraded. It may be impossible to look at all transects simultaneously.

This method two notable advantages:
1. Georeferenced rectangular files are as close as you can get to a distortion-corrected view of what you saw on the unit's display.
2. It is super fast and doesn't have a ton of dependencies.

## 1.5 The nuts and bolts of how the georeferencing works

Anyone who worked on a research vessel in the last few decades—and now an entire generation of recreational anglers—is familiar with the raw presentation of side-scan data. The water column is laid out horizontally on either side of the boat, the view of the bottom is distorted, and the horizontal and vertical resolutions are not the same. The naïve approach to georeferencing is to manually remove the water column and then manually stretch and rotate the image. The distances represented are, however, represented as distance from the instrument and not along the bottom, and this method distorts the distances.

The simplest description of the operation of the instrument is that it sends out a *ping* and then records the *returns* or the sound intensity at that frequency at regular intervals for a specified period of time. The result in this instrument is a *line* of *bins*, where each successive *bin* represents the intensity of the sonic environment at 455 kHz during a very small time interval. Since the speed of sound can be assumed to be constant for the ping, these times directly translate into distance from the boat. 

As the *ping*'s wave front propagates along the bottom, the returns represent the *backscatter* strength from the bottom at their time-derived distance. The distance from the boat is the hypotenuse of a right triangle where the depth is one leg and the *cross-track distance* (the distance along the bottom) is the other. We can use the Pythagorean theorem to derive the *cross-track distance*. *If you've caught the glaring, obvious, and definitely wrong assumption with this statement, this is the **flat bottom assumption**.*

What this software is doing is taking each *line* and calculating the *cross-track distance* for each *bin*. It then creates a *TIFF scan line* where each pixel represents the minimum *cross-track* size of a bin (in the ballpark of 19 mm in fresh water). It then iterates through the line, stretching each bin to its theoretical real-world distance. Since all *scan lines* in a TIFF must be the same length (to my knowledge), each line is the maximum length that the project might need, and the end of each line is populated with zeros. 

The steps are as follows:
1. Open a line and use the depth and Pythagorean theorem to compute the *cross-track* distance associated with each *bin* in the line.
2. Produce a *TIFF scan line* at the maximum possible theoretical spatial resolution of the line
3. Increment through the *Tiff scan line*, one (19 mm) *corrected bin* at a time, check the *bin* distance for the current *return*.
  - If the *bin* distance for the current *return* is greater than the *corrected bin* distance for the TIFF, move to the next *return* and record its value for the *corrected bin* in the *TIFF scan line*.
  - If the *bin* distance for the current *return* is less than or equal to the *corrected bin* distance for the TIFF, record its value in for the *corrected bin* in the *TIFF scan line*
  - If the *corrected bin* is beyond the *corrected distance* of the last return, record a zero in the corresponding location of the *TIFF scan line*.
4. Record the GPS location of the boat at the beginning of the group of *scans* in the TIFF, this will be the top left corner of the TIFF.
5. Find the bottom right corner of the TIFF by adding the maximum *cross-track* distance to the boat's last GPS location in the group of *scans*
6. Rotate the track to the movement direction of the boat.
7. Save this georeferencing information in a *World File* (text file that tells GIS programs where to put certain kinds of raster data).
> Note that all TIFFs are laid out with the boat's first location at the top left and the end of the last scan at the bottom right, regardless of the direction of travel or side of the transducer. *Cross-track* distances are negative for the *Port Side* (left) transducer and positive for the *Starboard Side* (right) transducer, and the rotation in the world file is sufficient to flip the image over if the boat is travelling generally north instead of generally south.

### 1.5.1 A note on the flat bottom assumption:

As you probably noticed from my earlier description of these corrections, we are assuming that the distance of a *return bin* is the hypotenuse of a right triangle where one leg is the depth and the other the distance along the bottom. This is only true if the bottom is flat and level (if you recall, right triangles are made of three straight lines, two of which intersect at a right angle). Fancier units (read multi-beam) can correct for this, but you are using a *consumer-grade* fish finder to do research. Upward slopes and things projecting up from the bottom will appear closer than they actually are, whereas downward slopes will appear farther away. Although this assumption is two-dimensional, so if the bottom is locally flat and level within a single scan, it will generally be correct. 

You can minimize this distortion by conducting transects *perpendicular* to the *depth contours* of the bottom.

### 1.5.2 A note on other assumptions:

I am making two other huge assumptions that are not addressed here: first is that the scan from each transducer covers a region from the boat to the maximum *cross-track* distance (the first *return* comes from directly beneath the boat). In reality these transducers are offset at a shallow angle and the bottom immediately below the boat is not well-captured. Second is that the depth recorded by the instrument is correct. 

There are two depth-related things going on here: Humminbird calculates the depth internally and their algorithm works better than mine (see the EXPERIMENTAL section for more details), and second, there are radiometric corrections that work on depth recording from sonar that deal with depth-related density changes that I'm not applying. Humminbird's internal calculation is based on the speed of sound and the unit only knows two. From a cursory read-through of comments I wrote in python code that I put together in 2015, this is 1463 m/s in fresh water and 1500 m/s if the unit is configured for salt water. I have added functions for both specifying the unit's configuration and a non-firmware speed of sound to address these issues.

# 2. Building

Enough with the preamble, you've downloaded this **open source** code from the **internet** and you would like to run it. This section will cover how to do that. Welcome to the amble. There are a couple of different ways you can run this software and the builds are detailed below. The options right now are: 1. the vanilla, command line invoked program; 2. the command line invoked program with experimental signal analysis functions built in; and 3. the GUI

Start by cloning this repository (or download and extract the zip file):

`git clone https://github.com/SonarGridder`

Then move into the src directory:

`cd ./SonarGridder/src`

## 2.1 Dependencies

This software requires the gnu C++ compiler (g++) and standard libraries. The computer it was designed on is a 64 bit x86 system running Ubuntu 20.04, if this is your setup you might be in luck. I promise to one day learn autotools, so this might become easier.

Additional library requirements are libTIFF for the TIFF file production (no way to get around this one really), and Gtkmm-3.0 if you want to build the GUI.

## 2.2 The vanilla command line version

I've included a make file and this is the default target. The makefile is configured for my system, but the only thing that might need to change is the library location for libTIFF. If it yells at you, try using the find command and change `LD_LIBRARY_PATH` in the Makefile to only the path of the file:

`find / -name tiffio.h`

`sed -i "s+/usr/include/x86_64-linux-gnu+[wherever you found tiffio.h]+g" Makefile`

You can then compile the binary file

`make`

If you want to install it in /usr/local/bin:

`sudo make install`

Or if you don't want to do this (I don't blame you) you can copy the binary file sonargridder (from the bin subdirectory) wherever you'd like (or just leave it where it is) and invoke it there, add a directory in your home directory to PATH, or add this repository's bin subdirectory to your path.

Other standard targets exist in the makefile including clean, which you should run between builds.

## 2.4 Experimental functions

I've included some experimental signal analysis functions that are in their early infancy. None of them are used for georeferencing TIFFs so if that's what you want to do, I recommend the vanilla version. If you want to include them, have a read through of the EXPERIMENTAL text file in the repository and decide if their functionality suits your needs. To build the experimental version, follow the steps in **2.2** until you get to `make`. Instead of making the default target we will be making the experimental target. This can be done by typing `make experimental` instead.

If you previously built the vanilla version, type `make clean` before starting.

`make clean`

`make experimental`

`(optional) sudo make install`

## 2.5 GUI

I've included a Gtkmm-3.0 GUI for the vanilla functions. It was intended to be a simple wrapper on the command line version, but it adds some functionality that I actually find helpful. It is early days and very buggy. There is a make target for it, and one day I will learn how to use autotools. You will need Gtkmm-3.0 (this is not the latest version but it is what Ubuntu 20.04 had).

`make clean`

`make gui`

`(optional) sudo make install`

If your distro put libTIFF somewhere else, change LD_LIBRARY_PATH in the Makefile to wherever you found it (see **2.2**).

# 3. Running

There are different methods of running this code, depending on whether you are using the CLI or GUI version. The experimental functions add some options to the CLI version for the down-imaging transudcer.

## 3.1 Vanilla and experimental CLI versions

The command line utility takes all options as command line arguments (there is no interactive mode). All files are output into the directory it is invoked from, so I strongly recommend making a new folder for your imagery. The invocation for side imagery is:

`sonargridder [FILENAME.SON] [port|starboard] [options]`

The filename is the name of a Humminbird .SON file. The unit outputs several other metadata files with other extensions but we don't use them here.

It is also necessary to specify whether the file corresponds to the *port* or *starboard* transducer **in lowercase letters**. Based on some code comments in a python script I wrote circa 2015, the port side file from my device (a Humminbird 698ci HD combo) is usually titled B002.SON, and the starboard file is B003.SON. I don't know how universally true that is or where I got that information, but it seems to hold true for me.

**Options**

-notiff: Don't output TIFF files (we just want the boat's path and depth maybe)

-nopath: Don't output a CSV detailing the boat's path (if you do both transducers in the same directory, this won't overwrite shipPath.csv on the second run).

-a [number]: Consider heading deviations of up to [number] tenths of a degree to be a "straight" path. The default is 100 (10°).

-max [number]: The maximum number of scans in a TIFF file will be [number]. (ie, if a straight path is more than [number] scans, it will be split into multiple TIFFs). The default is 100

-min [number]: The minimum number of scans in a TIFF file will be [number]. (ie, we won't get a bunch of super small TIFFs that eat RAM and are hard to place). The default is 10, it must be more than 2.

-fresh: The unit was configured for fresh water. Don't adjust depths from the firmware specified speed of sound in fresh water (1463 m/s).

-salt: The unit was configured for seawater. Don't adjust and depths from the firmware specified speed of sound in salt water (1500 m/s).

-mod-fresh [number]: The unit was configured for fresh water but adjust depths and distances for a speed of sound of [number] m/s.

-mod-salt [number]: The unit was configured for seawater but adjust depths and distances for a speed of sound of [number] m/s.
> Note: if more than one of -fresh -salt -mod-fresh [number] or -mod-salt [number] is specified, the last option specified will be used. If none of these options are specified, the program will use the default option (fresh water, using the firmware-specified speed of sound of 1464 m/s).

**If you built the EXPERIMENTAL version you will also have an option for down-imaging transucers**

`sonargrider [FILENAME.SON] [port|starboard|down] [options]`

Down-imaging files are B001.SON on my unit. B000.SON is the 83 kHz fish finder/depth finder transducer.

One additional option exists:

-guess-maxreturn: Calculate the maximum return strength of the transducer by comparing the maximum return of each scan with the calculated water attenuation at that depth. Will include water attenuation in analyses. *In practice, the current algorithm is only slightly better than a wild guess*

## 3.2 The GUI

I haven't fully documented the GUI yet, but if you run it you will notice that there is a combo box and three buttons along the bottom of the panel, and a text console for log outputs occupying most of the screen. The workflow is enforced so that you don't end up processing anything without required information. The workflow follows:
1. Click "Settings". You will get a menu with some options. Set the sound speed you used, the maximum and minimum number of scans in a TIFF file, and the maximum heading change (in tenths of a degree) that the program will consider straight.
2. Click update. The options you selected will add themselves to the on-screen log.
3. Select the *combo box* at the lower left corner of the screen and choose the port or starboard transducer (It is possible to change this until you hit the start button, so don't panic if you choose wrong. The option you choose will be added to the log when you click start).
4. Click the "Open" button. Select a Humminbird .SON file corresponding with the transducer you selected. The log will give you a hint based on which transducer you selected.
5. Click the "Open" button on the dialog.
6. Click the "Start" button (if you messed up any of the options, you can change them before doing this).
7. The system dialog will appear to ask you where to save the files. Select a folder and click the "Select" button on the dialog.
8. The log file output information on the scans including:
   - Which transducer you selected before the scans
   - The number of breakpoints generated by your settings
   - How many TIFFs are too short to be output
   - How many TIFF files will be output
   - How long the longest line in your file was
   - Which specific lines from the file were not output and why
9. After the console indicates that it is done with the file, your TIFF files, world files, and CSV files will be in the folder you selected in step 6 for opening in GIS

# 4. Tips

If you are outputting georeferenced TIFF files, congratulations! You are well on your way to classifying substrates using a $500 fish finder. There are some operational tips contained here that you might find helpful:

## 4.1 Using the output files with QGIS

The TIFF files are output in a format that QGIS opens natively, but they are vanilla TIFFs (read: not GEOTIFF) with world files. World files have the same name as the TIFF (.tif) file but the extension is changed to .tfw. These files must stay in the same folder as the .tif files and their names must match the corresponding .tif file. QGIS will open the .tif file and then stretch and rotate it based on the contents of the .tfw file. As far as I know there isn't a straightforward way to build CRS information into the world file, but it is WGS84 (EPSG:4326). I recommend starting a new map in EPSG:4326 and loading the files there. You can warp them into a different projection, merge them, and/or save to a different format as necessary after loading them.

QGIS represents TIFF raster files as rectangles in a perfectly north-south orientation. This means that the rotated files will be surrounded by parts of a rectangle that contains no data from the original file. This region will be populated with zeros (in addition to the region of each line beyond the actual length of the line). You will need to set zero as a transparent value to get rid of these margins. There is a bug in the current version where the additional transparent values field does not copy with the styles, so my preferred method is to right click on the TIFF file layer and select properties. Then click on transparency. Under "Custom Transparency Options", keep the "Transparency band" combo box set to none and click the + button. A new custom transparency option will appear in the box. Set the "From" field to 0 and "To" field to 9, set the "Percent Transparent" field to 100. Click okay. You can then right click on the layer, navigate to the "Styles" submenu and then click "Copy Style". The style can be pasted on other layers or even groups by right clicking on them and selecting "Paste Style".

My QGIS workflow:
1. Navigate the browser to the folder where the files were output, select all of the files corresponding with the current transect, and then right click them and select "Add Selected Layers to Project".
2. Select all of the new layers and combine them into a group
3. Right click on the new group and select "Set group CRS", select WGS84 (EPSG:4326)
4. Select the first layer in the group, update it's range (10 to 255) and transparency settings (0-9 100% transparent)
5. Right click on the first layer in the group and select "Copy Style"
6. Right click on the group and select "Paste Style"
7. Right click on the group and click export as layer definition file, save the group

## 4.2 Planning a study using this method

In theory this software should be able to handle any recording from the Humminbird 698ci HD combo, or similar units, possibly with modification. In practice it has only been used in a few situations. This section outlines some tips and pitfalls with the way studies are run that can influence the performance of this analysis.

### 4.2.1 Sonar Setup

1. For best results, set the sonar up for fresh water or salt water corresponding with the type of water body you are in. The speed of sound will configure itself to 1463 or 1500 m/s respectively. Use the corresponding value in this software.
2. After turning the unit on, remain stationary for several minutes until the GPS warms up. Don't begin to move the boat until GPS coordinates are properly displayed on the GPS screen. You may not get a fix while moving, so grab a coffee or use this time to clean up your boat.
3. Disable auto-ranging before beginning a recording. This shouldn't be strictly necessary but the firt iteration of this software, the line processing functions would choke on range variations. I set my range to 50 m for all surveys and can't test with auto-ranged data, so YMMV. You definitely will end up with a lot of TIFF files containing lines ending with a bunch of zeros if the range changes. The TIFF resolution is set by the maximum possible *cross-track* distance of the longest scan.

### 4.3.2 Planning an outing

1. Conduct a depth survey first and arrange transects perpendicular to depth contours. This minimizes distortion associated with the flat bottom assumption. Downstream surveys in rivers are "traditional", but if you do this, keep in mind that the *along-track* information will be more valid than the *cross-track* information.
2. You can use recordings and this software or tracks (use GPS Babel to translate these to CSV) to interpolate the bottom contours. Recordings have more data, which can be a blessing or a curse. shipTrack.csv is my program's output of this information.

### 4.3.3 In the field

1. The cross-track resolution is fixed by the device but the along track is both poorer and determined by the boat's speed. Move as slow as you reasonably can without losing control authority.
2. Re-read the second sentence of the previous entry. If you are going so slow that the boat is not maintaining a (relatively) straight path, this program will either output a jumbled mess or require you to modify the heading angle tolerence. Wind, currents, and wave action should all be considered.
3. You may need to redo parts of surveys and you won't know until you've processed them. Keep this in mind and budget extra time.
4. Keep an eye on the GPS. If the coordinates stop changing, wait for a new fix and repeat the transect as necessary.
5. (optional but will make your life easier) Stop recording between transects, possibly use a separate SD card for each transect. This will make your life easier processing the data. I didn't do this and look how much code I had to write to solve the problem I made for myself.

### 4.3.4 Analyzing data

1. If parts of the survey area overlap, look at these separately. There are spatial errors that aren't completely controlled and features may move (hopefully only slightly) between transects.
2. Turn off layers that you aren't looking at. I'm not sure about ESRI, but QGIS's memory use gets a little bogged down if a whole bunch of files are open. I've had a few crashes.
3. Back up your original files, make lots of copies, and don't run the software in the folder you stored the data in (if using the command line), especially if you are reading from the SD card.
4. Don't process data directly from the SD card. I'm assuming you know how unreliable those things are. Copy to your computer first.

### 4.3.5 General disclaimer

1. You are processing data from a commercial-grade fish finder intended for recreational anglers using free software that you downloaded from the internet that was written by a fish biologist (read: not a computer scientist). Please have patience for me, your hardware, and recognize that you will not get data that resembles what you would get with a research-grade multibeam system worth 30 kilobucks.
2. Some functions—especially in the experimental build—are incomplete, poorly written, and possibly dangerous.
3. Please understand that this software is provided WITHOUT WARRANTY (see the LICENSE file for more information) and running it may be a risk to your data, your hardware, and your mental wellbeing.
4. I am doing the best that I can, but you may be able to do better. Please feel free to modify this code, use it in your own projects, or lend me your expertise. Please see the LICENSE file for more information.
5. If you do use this software, please let me know. I am interested in knowing about your project. I can be contacted through this github repository or at the email provided.
6. If you use this software and write a paper based on your analysis, please drop me a citation:

Birch, J. 2021. SonarGridder: A utility for georeferencing consumer-grade side-scan sonar files. URL: https://angularfish.net

# 5. Suggested reading

Buscombe, D. 2017. Shallow water benthic imaging and substrate characterization using recreational-grade sidescan-sonal. Environmental Modelling and Software. 89. pp 1-18

Faggetter, B. 46. Humminbird Raw Sonar Data. Accessed 2019-09-15. URL: https://oceanecology.ca/publications/Humminbird%20%Raw%20Sonar%20Data.pdf

Dan Buscombe's other papers from 2012 to 2017.

Barb Faggetter's other writings on the subject at https://oceanecology.ca

Sonar in general is a huge literature rabbit hole. If you are an expert or know papers I should suggest here, please let me know.

## 5.1 Recommendations for other software

- PyHum (D. Buscombe) https://dbuscombe-usgs.github.io/PyHum

- MB-System Seafloor Mapping Software (MBARI, California, USA) https://www.mbari.org/products/research-software/mb-system

- QGIS https://qgis.org

# 6. Post-amble?

If you read this far and decided this analysis is for you, happy surveying! If you decided to buy or rent a better instrument, thanks for stopping by!

If you need help, don't hesitate to ask for help. I have a day job but will answer emails as quickly as I can.

If you find a bug, want me to add a feature, feel that there is something that I need to know, or just want to get in touch, my contact information is here.

# 7. Changelog

## [0.0.1] - 2021-09-20
### Added
- Makefile target for GUI
- Functions to guess maximum return strength (clipping level) of the down imaging transducer in the EXPERIMENTAL build
### Changed
- Additional command line option `-guess-maxreturn` for experimental down-imaging function

## [0.0.1] - 2021-09-19
### Added
- User-configurable speed of sound using the options -fresh, -salt, -mod-fresh, and -mod-salt.
- User-configurable speed of sound in the GUI version.

### Changed
- File structure and physical parameters are now variables instead of macros to assist in future versions where users may want to change these settings without recompiling.
- Cross-track distance is now calculated in equivalent bin-distances to simplify calculations.
- Side-scan lines where depth apparently exceeds range are now output as TIFF scanlines of zeros to prevent mismatches between TIFFTAG\_IMAGE\_LENGTH and the file size and/or stretching adjacent scans into the empty space left by failed scans.
- Bug fixes.

