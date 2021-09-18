/*
╔══════════════════════════════════════════════════════════════════════════════╗
║Sonar Gridder GUI Functions: A self-explanatory title.                        ║
╠══════════════════════════════════════════════════════════════════════════════╣
║Copyright ©2021 Jim Birch (https://angularfish.net)                           ║
╟──────────────────────────────────────────────────────────────────────────────╢
║This program is free software; you can redistribute it and/or modify it under ║
║the terms of the GNU General Public License as published by the Free Software ║
║Foundation; either version 2 of the License, or (at your option) any later    ║
║version.                                                                      ║
║                                                                              ║
║This code was written by a fish biologist during his spare time during a glob-║
║al pandemic. It was written for free and is available for free on the internet║
║I have provided this code in the hope that it may prove useful to others, but ║
║WITHOUT ANY WARRANTY; not even the implied warranty of MERCHANTABILITY or FIT-║
║NESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more de-║
║tails.                                                                        ║
║                                                                              ║
║You should have received a copy of the GNU General Public License along with  ║
║this program; if not, write to the Free Software Foundation, Inc., 51 Franklin║
║Street, Fifth Floor, Boston, MA 02110-1301 USA.                               ║
╟──────────────────────────────────────────────────────────────────────────────╢
║Disclaimer: I wrote this code for my own purposes and am sharing it in case it║
║might be useful to others. It is slow, buggy, and written by a fish biologist.║
║I have used it with some success but I am unwilling to make any claims as to  ║
║the suitability of any analysis detailed here for your project, hardware, or  ║
║mental wellbeing. If you use it please consider letting me know and see the   ║
║README file for a suggested citation. Back up your data often, make sure you  ║
║have copies of your original files, and consider using a research-grade sonar ║
║instead of this consumer-level unit in the future.                            ║
╟──────────────────────────────────────────────────────────────────────────────╢
║it is useful to others. This section is a GUI wrapper over the functions from ║
║the main version of the software. I had thought it was largely going to be for║
║convenience for folks who are afraid of the command line, but it turns out to ║
║be more useful than I intended. I haven't set up a tool chain for building it,║
║but it uses Gtkmm-3. The code it implements requires LibTIFF. Currently it do-║
║es not conduct any of the experimental analyses and it has a number of proble-║
║ms (compile it with -Wall if you want a laugh). I will keep working on it as I║
║have time. My goals are:                                                      ║
║1. Fully user-reconfigurable for fresh water, salt water, and sonar specs.    ║
║2. User-reconfigurable for hardware-specific file layouts.                    ║
║3. Possibly build in file handlers for other file types. XTF maybe?           ║
║4. An interface for the experimental functions.                               ║
║5. Fix all of the memory management issues currently plaguing the gtk impleme-║
║   ntation herein.                                                            ║
╟──────────────────────────────────────────────────────────────────────────────╢
║Files:                                                                        ║
║sonwindows.h: Object prototypes for the gui. Requires Gtkmm-3                 ║
║songui.cpp:   Code associated with getting a gui going                        ║
║guimain.cpp:  A very short main function to launch the gtkmm-3 gui            ║
╟──────────────────────────────────────────────────────────────────────────────╢
║Depends on files from Sonar Gridder:                                          ║
║songridder.h:     Header file for code related to georeferencing SON files.   ║
║sidescan.cpp:     Code for georeferencing SON files.                          ║
║processfiles.cpp: Code for processing SON files.                              ║
╟──────────────────────────────────────────────────────────────────────────────╢
║Tested with data produced by a Humminbird 598ci HD manufactured circa 2014.   ║
║May work with other units with or without modification.                       ║
╟──────────────────────────────────────────────────────────────────────────────╢
║   Please let me know if you find this software useful! jim[át]jdbirch.com    ║
║                 (Or if you can't get it to run at all)                       ║
╚══════════════════════════════════════════════════════════════════════════════╝
*/
#include "sonwindows.h"
#include "songridder.h"

MainWindow::MainWindow() : port(true),
                           ready(false),
                           writeCSV(false),
                           writeTIFF(true),
                           pathAndDepth(true),
                           maxLines(300),
                           minLines(10),
                           angle(100),
                           soundspeed(1463),
                           frequency_s(455000),
                           frequency_d(200000),
                           main_layout(Gtk::ORIENTATION_VERTICAL),
                           bottom_buttons(Gtk::ORIENTATION_HORIZONTAL),
                           console(Gtk::ORIENTATION_HORIZONTAL),
                           button_start("Start"),
                           button_open("Open"),
                           button_settings("Settings"),
                           button_update("Update"),
                           l_speed("Speed of Sound"),
                           l_freq_s("Sidescan Frequency"),
                           l_freq_d("Down Frequency"),
                           l_unit("Model"),
                           l_tiff_max("Maximum TIFF Scans"),
                           l_tiff_min("Minimum TIFF Scans"),
                           l_heading("Maximum Heading Change") {
  // Set up the structure of the window.
  set_border_width(10);
  set_size_request(800,600); 
  set_title("Sonar Gridder (working title)");

  // Disable user inputs on the dangerous buttons until the operation is set up
  button_start.set_sensitive(false);
  button_open.set_sensitive(false);
  combo_side.set_sensitive(false);
  program_output.set_sensitive(false);

  // Attach signal handlers to the buttons.
  button_start.signal_clicked().connect(
    sigc::mem_fun(*this,&MainWindow::on_button_start));
  button_settings.signal_clicked().connect(
    sigc::mem_fun(*this,&MainWindow::on_button_configure));
  button_open.signal_clicked().connect(
    sigc::mem_fun(*this,&MainWindow::on_button_open));
  combo_side.signal_changed().connect(
    sigc::mem_fun(*this,&MainWindow::on_combo_changed));

  // Add the options for the combo box.
  combo_side.append("Port");
  combo_side.append("Starboard");

  // Set up the scrolled window "console"
  console_window.add(console);
  console.pack_start(program_output); 
  text_buffer = program_output.get_buffer();

  // Add a box container to the main window. Pack it.
  add(main_layout);
  main_layout.pack_start(console_window,Gtk::PACK_EXPAND_WIDGET);
  main_layout.pack_start(operation_progress,Gtk::PACK_SHRINK);
  main_layout.pack_start(bottom_buttons,Gtk::PACK_SHRINK);
  
  // Add the bottom buttons to the main window.
  bottom_buttons.pack_start(combo_side);
  bottom_buttons.pack_start(button_settings);
  bottom_buttons.pack_start(button_open);
  bottom_buttons.pack_start(button_start);

  // The settings dialog is instantiated here because it needs to be accessible
  // regularly and it updates data in this object.

  // Add a signal to the settings button for copying the settings information
  // into the appropriate variables.
  button_update.signal_clicked().connect(
    sigc::mem_fun(*this,&MainWindow::update_settings));

  // The settings dialog has a grid layout with two columns. The left contains
  // labels, and the right contains fields for editing the labeled variables.
  settings.attach(l_speed,0,0,1,1);
  settings.attach(e_speed,1,0,1,1);
  settings.attach(l_freq_s,0,1,1,1);
  settings.attach(e_freq_s,1,1,1,1);
  settings.attach(l_freq_d,0,2,1,1);
  settings.attach(e_freq_d,1,2,1,1);
  settings.attach(l_unit,0,3,1,1);
  settings.attach(c_unit,1,3,1,1);
  settings.attach(l_tiff_max,0,4,1,1);
  settings.attach(e_tiff_max,1,4,1,1);
  settings.attach(l_tiff_min,0,5,1,1);
  settings.attach(e_tiff_min,1,5,1,1);
  settings.attach(l_heading,0,6,1,1);
  settings.attach(e_heading,1,6,1,1);

  // The only button at the bottom is the update button. Panicked users can x
  // out of the dialog I think.
  settings.attach(button_update,0,7,1,1);
  dialog_settings.set_default_size(200,200);

  // Dialogs have a box container build in. Here we're packing the grid into
  // the box.
  dialog_settings.get_content_area()->pack_start(settings);
  dialog_settings.set_title("Settings");


  // Fire up this window
  show_all_children();

  // Give the user some information on the software.
  add_text("Welcome to SonarGridder v0.0.1\n");
  add_text("Copyright ©2021 Jim Birch\nhttps://angularfish.net\n\n");
  add_text("This program is free, open-source software published under");
  add_text(" version 2 of the GNU GPL ");
  add_text("Please see the LICENSE file\nfor more information.\n\n");
  add_text("If you use this software for your own work, please consider ");
  add_text("dropping me a citation!\n\n");
  add_text("Real scientists read the manual ☺.\n");
  add_text("\n\n\n");

  // The workflow order is enforced in this UI. It mitigates crashes caused by
  // running the code without specifying settings first. I think it also helps
  // mitigate user errors like processing port side data as starboard or vice-
  // versa.
  add_text("Workflow:\n");
  add_text("Settings →  Side → Open → Start\n");
  add_text("Configure your unit and study site in settings.\n");
  add_text("Select either the port or starboard side transducer in the combo box.\n");
  add_text("HINT: Port is typically B002.SON, starboard is typically B003.SON.\n");
}

MainWindow::~MainWindow() {
}


void MainWindow::on_combo_changed() {
// We have the user select a side (of the boat) that the transducer is
// broadcasting from. Humminbird makes a separate file for each transducer.
// If this gets extended to process XTF I will need to rethink this.

  // Enforce the order in which the user can press buttons. It doesn't need to
  // be as strict as it is here, but being rigid in the workflow makes sure
  // important information doesn't get missed.
  button_open.set_sensitive(true);

  // Determine which option the user selected.
  Glib::ustring text = combo_side.get_active_text();
  Glib::ustring starboard = "Starboard";

  // More string literal comparison. The text combo box didn't require me to
  // learn how to use trees.
  if(text == starboard) port = false;
  else port = true;
}

void MainWindow::on_button_start() {
// I think this button should be red and have a cover over it, but I don't know
// how to do that in gtk. This starts the analysis.

  // Make the user specify a folder to put our newly-generated files in.
  add_text("Please specify an output folder. \n");
  savefolder = open_dialog("Choose a folder to save output", 
                           Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
  if(savefolder == "try again") { // The function only returns std::strings.
                                  // I'm doing my best.
    add_text("No folder specified. Please try again. \n");
    return;
  }

  // Handle the last couple of variables necessary for the analysis to begin.
  savefolder = savefolder + "/"; // It is annoying that this is necessary and
                                 // it will need to change for Windows if that
                                 // happens.
  
  // Tell the user where the files will go. This will be helpful if they are
  // absent minded like I am.
  add_text("Files will be output in: ");
  add_text(savefolder);
  add_text("\n");

  // Make super sure this button doesn't get clicked twice. Double extra super
  // sure.
  button_start.set_sensitive(false);

  // Time for some analysis
  run_sidescan();
}

void MainWindow::on_button_open() {
// Invoke the file open dialog to get the path of a (hopefully) side-scan
// binary file.
  
  // This is intended to help the user. It is still possible to change the
  // side after a file is selected, right up until the start button is clicked.
  add_text("Please select a SON file to open for the ");
  add_text(combo_side.get_active_text());
  add_text(" side transducer.\n");
  // The following appears to be true for my sonar device. I'm unsure how
  // I got this information but I think it's from a since-deleted forum post 
  // and made its way uncited into a comment in python code I wrote in 2015.
  if(port) add_text("hint: Port is typically B002.SON\n");
  else add_text("hint: Starboard is typically B003.SON\n");
  // I've offloaded the dialog itself to a separate function so we can reuse it
  // to open a folder to dump a metric ton of TIFF files into.
  filename = open_dialog("Choose a SON file", Gtk::FILE_CHOOSER_ACTION_OPEN);
}

void MainWindow::on_button_configure() {
// Configures and launches the settings dialog. Most of these settings are
// currently unimplemented as of 2021-09-17
  
  // Copy values for the settings from their appropriate variables
  e_speed.set_text(std::to_string(soundspeed));
  e_freq_s.set_text(std::to_string(frequency_s));
  e_freq_d.set_text(std::to_string(frequency_d));
  e_tiff_max.set_text(std::to_string(maxLines));
  e_tiff_min.set_text(std::to_string(minLines));
  e_heading.set_text(std::to_string(angle));
  // Set the input purpose for the settings to digits. This seems to be more of
  // an invocation or prayer than obvious change to how the data are handled.
  // certainly it still lets me type whatever I want and I don't know why.
  e_speed.set_input_purpose(Gtk::INPUT_PURPOSE_DIGITS);
  e_freq_s.set_input_purpose(Gtk::INPUT_PURPOSE_DIGITS);
  e_freq_d.set_input_purpose(Gtk::INPUT_PURPOSE_DIGITS);
  e_tiff_max.set_input_purpose(Gtk::INPUT_PURPOSE_DIGITS);
  e_tiff_min.set_input_purpose(Gtk::INPUT_PURPOSE_DIGITS);
  e_heading.set_input_purpose(Gtk::INPUT_PURPOSE_DIGITS);
  // Show the dialog
  dialog_settings.show_all();
}

void MainWindow::update_settings() {
// Copy the text from the entry fields into the relevant variables for data
// processing.

  // More workflow order enforcement. Don't let the user do anything before
  // configuring the speed of sound, etc. Let them do one thing after that.
  combo_side.set_sensitive(true);

  // Here we extract a bunch of Glib::ustrings, convert them into UTF8 char 
  // arrays, convert those into signed integers and then cast the signed integers 
  // into unsigned integers.
  maxLines = (uint32_t)std::atoi(e_tiff_max.get_text().c_str());
  minLines = (uint32_t)std::atoi(e_tiff_min.get_text().c_str());
  angle = (uint32_t)std::atoi(e_heading.get_text().c_str());
  soundspeed = (uint32_t)std::atoi(e_speed.get_text().c_str());
  frequency_s = (uint32_t)std::atoi(e_freq_s.get_text().c_str());
  frequency_d = (uint32_t)std::atoi(e_freq_s.get_text().c_str());

  // The readback here is more for my own piece of mind than for the user, but
  // having a log of these settings is probably helpful. Maybe I should
  // implement a save log option to convert this buffer into a text file.
  add_text("\n\n\nSettings updated.\n");
  add_text("Current settings:\n");
  add_text("The maximum scans in a TIFF will be: ");
  add_text(std::to_string(maxLines));
  add_text("\nThe minimum scans in a TIFF will be: ");
  add_text(std::to_string(minLines));
  add_text("\nTolerating heading changes up to: ");
  add_text(std::to_string(angle));
  add_text(" tenths of a degree");
  add_text("\nThe speed of sound is: ");
  add_text(std::to_string(soundspeed));
  add_text(" metres per second.\n    Note: 1463 should be used for fresh water");
  add_text(" 1500 should be used for seawater.\n");
  add_text("\n");

  // Disappear the settings window. Until next time, settings window.
  dialog_settings.hide();

  // The ready flag should be one of the two keys necessary to launch the
  // nuclear missiles. Currently it is not.
  ready = true;
}

std::string MainWindow::open_dialog(std::string title_text, 
                                    Gtk::FileChooserAction opensave) {
// It didn't make sense to implement the file open dialog twice so this
// function handles both opening the data file and selecting a folder to output
// the TIFFs, CSVs, and other processed data to. We pass a
// Gtk::FileChooserAction to it directly.

  // Set up the dialog window
  Gtk::FileChooserDialog dialog(title_text, opensave);
  dialog.set_transient_for(*this);
  
  // The cancel button is used in both implementations
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  // The OK button is labeled "Select" with the folder chooser
  if(opensave == Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER) {
    dialog.add_button("Select", Gtk::RESPONSE_OK);
  }
  
  // If the dialog was launched from the file open button:
  // Set up a pattern filter to make sure only SON files get opened. Humminbird
  // outputs some metadata files that we don't use in this software with other
  // extensions, and the people using this are presumably biologists. (:
  if(opensave == Gtk::FILE_CHOOSER_ACTION_OPEN) {
    auto filter_son = Gtk::FileFilter::create();
    filter_son->set_name("Humminbird .SON files");
    filter_son->add_pattern("*.SON");
    dialog.add_filter(filter_son);
    // Make the button say "Open". See? It's the little details.
    dialog.add_button("_Open", Gtk::RESPONSE_OK);
  }

  int result = dialog.run(); // Stop everything and wait for the user to find
  switch(result) {           // their files.
    case(Gtk::RESPONSE_OK): {
    // If the user selected a file or folder, return its path.
      std::string selection = dialog.get_filename();
      if(ready) button_start.set_sensitive(true);
      return selection;
      break;
    }
    case(Gtk::RESPONSE_CANCEL): {
    // If the user cancelled. 
      add_text("Cancelling \n");
      return "try again"; // When I was learning how to do this I had a cout
      break;              // here, but I changed it to a return string. It
    }                     // should get screened out, but if it doesn't they'll
                          // get an error about not being able to open "try 
                          // again" and that's worth leaving it in for tbh.
    default: {            
      return "try again"; // What the heck did they even click I only gave two
      break;              // options.
    }
  }
}

void MainWindow::add_text(Glib::ustring newtext) {
// The text console is not the prettiest interface I've ever seen but it took
// way more effort to get right than I am willing to admit here. This function
// does two things: 1. Update the console buffer with any new information we're
// passing it, and 2. scroll the text view to the bottom whenever it updates.
  
  // Get an iterator for the end of the text buffer.
  Gtk::TextIter iterator = text_buffer->end(); 
  // Add the text from the call at the position of the iterator.                                         
  text_buffer->insert(iterator, newtext); 

  // Get the scrolled window's vadjustment
  Glib::RefPtr<Gtk::Adjustment> adj = console_window.get_vadjustment();
  // Wait for the iterator to stop moving
  while(Gtk::Main::events_pending()) Gtk::Main::iteration();
  // Set the vadjustment to the length of the scrolled window
  adj->set_value(adj->get_upper());
}

void MainWindow::run_sidescan() {
// This is effectively the same function as processSideScan in sidescan.cpp.
// It has been modified for the GUI by adding hooks for the progress bar and
// and text console, as well as using this class's member variables instead of
// taking arguments.

  // File stream objects
  streampos size;
  char* sFileData;

  // Update the user
  add_text("\n\n\nBeginning to process a sidescan file ");
  if(port) add_text("for the port side.\n");
  else add_text("for the starboard side.\n");

  // Step 1: Read file into memory. If files end up being really f'in huge this
  // might be a bad idea.
  ifstream sonFile(filename, ios::in | ios:: binary | ios::ate);
  if(!sonFile.is_open()) {
    // Could be that it's "Try again" lol.
    add_text("Unable to open file: ");
    add_text(filename);
    add_text("\n");
    return;
  }

  // Since we opened the file stream and moved to the end (ios::ate), we can
  // use the stream position to get its size.
  size = sonFile.tellg();

  // We have to read the data as a signed char as far as I know. This is dumb
  // and makes signal processing hard but I don't make the rules.
  sFileData = new char[size];

  // Now we jump to the beginning of the stream
  sonFile.seekg(0, ios::beg);
  // Take the whole dang file and put it in the computer's ram. My largest
  // transects are ~50 megabytes so this should be okay.
  sonFile.read(sFileData, size);
  // Close the file we don't need it anymore
  sonFile.close();

  // This is more logging to be helpful to the user. We've read the file
  // successfully into ram, but don't even know if it's a SON file yet.
  add_text("File: ");
  add_text(filename);
  add_text(" read successfully.\n");

  // Reinterpret the file as an unsigned char. This is important for using the
  // same functions for line header data in files and lines.
  unsigned char* fileData = reinterpret_cast<unsigned char*>(sFileData);

  // Count the number of lines in the file, an important limit to the functions
  // in fileprocessing and sidescan.cpp
  int count = countLines(fileData, (int)size);

  // Step 2: Break the recording up into sections that can be reasonably
  // approximated with a rectangle
  bool* breaks = new bool[count]; // An array for breakpoint locations.
  for(int i = 0; i < count; i++) breaks[i] = false;
  // If the boat's heading changes too much, our rectangularness assumptions are
  // violated. Put a breakpoint in where the heading changes.
  breakOnDirectionChange(fileData, breaks, count, angle);
  // Sometimes the GPS doesn't work. I'm not sure this function does either, but
  // we try to put in a breakpoint whenever there is no location.
  breakOnNoPosition(fileData, breaks, count);
  // Finally, break up long, straight sections into smaller sections to keep the
  // TIFF rows down.
  breakOnMaxLines(breaks, count, maxLines);

  // Step 3: Transform the breakpoint array into an array listing the locations
  // of breakpoints. Start by counting the number of breakpoints in the file.
  int bpCount = 0;
  for(int i = 0; i < count; i++) if(breaks[i]) bpCount++;

  // This text helps the user track how their settings are being represented in
  // the data. The processing doesn't stop, but we let them know how many break
  // points there are and how much data is being thrown out.
  add_text("\nUnder current settings there are:\n");
  add_text(std::to_string(bpCount));
  add_text(" break points\n");

  // Finish transforming the breakpoints array into the array that tracks the 
  // start and end lines of each file.
  uint32_t* breakPoints = new uint32_t[bpCount];
  int bpCurr = 0;
  int skippedOutputs = 0;
  for(int i = 0; i < count; i++) if(breaks[i]) {
    breakPoints[bpCurr] = i;
    // Also count the number of lines that are below the user-specified minimum
    // file length
    if(minLines > (i - breakPoints[bpCurr - 1])) skippedOutputs++;
    bpCurr++;
  }
  delete[] breaks;

  // Finish telling the user how their settings are impacting the analysis. The
  // minimum line count parameter is used to: 1. Prevent single-line TIFFs, and
  // 2. avoid the weird nonsense associated with the boat turning.
  add_text(std::to_string(skippedOutputs));
  add_text(" TIFF files are too short to be generated\n");
  // The number of files being output is controlled by the number of breakpoints
  // which the user does not control, and the maximum file length, which they do
  int numberFiles = bpCount - skippedOutputs;
  add_text(std::to_string(numberFiles));
  add_text(" total TIFF files will be output\n");
  
  // Step 4: determine offsets of the beginning of each line, as well as the
  // maximum length of a line in the file. We'll reuse data structures, so we
  // need to know the largest they could need to be. Also, it's helpful to know
  // where to look for the specific data we need.
  unsigned int* LineStarts = lineStarts(fileData, (int)size, count);
  unsigned int lineLen = 0;
  for(int i = 0; i < count - 1; i++) if((LineStarts[i+1] - LineStarts[i]) > lineLen) {
    lineLen = LineStarts[i+1] - LineStarts[i];
  }

  // Tell the user how long the lines are idk do they care? I guess the TIFF
  // size is proportional(ish) to this. It is only user-controlled by setting
  // the instrument's range before recording.
  add_text(std::to_string(lineLen));
  add_text(" is the maximum line length\n");

  operation_progress.set_fraction(0.0); // Progress bars are awesome.

  // Step 5: If the user requested it, calculate a CSV containing the location
  // and strength of each signal return. Currently this is not implemented.
  // QGIS can't handle these files well even with 24 gb of ram.
/*  if(writeCSV) add_text("\nGenerating signal strength CSV.\n");
  for(int i = 0; i < bpCurr - 1; i++) {
    double progress = double(i) / double(bpCurr);
    if(!writeCSV) break;
    bool outputCSV = generateSideScanCSV(savefolder, fileData, lineStarts, lineLen,
                                         breakPoints[i], breakPoints[i+1], port);
    operation_progress.set_fraction(progress);
  } */

  operation_progress.set_fraction(0.0); // If we just ran the CSV output, reset
                                        // the progress bar.
  

  // Step 6: Output georeferenced TIFF files. This is what we've all been
  // waiting for I'm sure.
  if(writeTIFF) add_text("\nGenerating side-scan TIFFs.\n");
  for(int i = 0; i < bpCurr - 1; i++) {
    // This has been a loop from main since I first conceived of this program.
    // It's nice that it's so amenable to a progress bar.
    double progress = double(i) / double(bpCurr);
    if(!writeTIFF) break;

    // Generate a TIFF between the two breakpoints
    bool outputTIFF = generateTIFF(savefolder, fileData, LineStarts, lineLen, 
                                   breakPoints[i], breakPoints[i+1], port, minLines);
    if(!outputTIFF) {
      // A file wasn't generated (or was but not georeferenced)
      if(minLines > (breakPoints[i+1] - breakPoints[i])) {
        // Tell the user if we skipped the file because it was too short
        add_text("TIFF for lines: ");
        add_text(std::to_string(breakPoints[i]));
        add_text(" to ");
        add_text(std::to_string(breakPoints[i + 1]));
        add_text(" is too short to be output.\n");
      } else {
        // Tell the user if something else happened
        add_text("Unspecified TIFF error for lines: ");
        add_text(std::to_string(breakPoints[i]));
        add_text(" to ");
        add_text(std::to_string(breakPoints[i + 1]));
        add_text(" may be a GPS error. Check world file.\n");
      }
    }
    // We've done the work now we get to see the progress bar advance.
    operation_progress.set_fraction(progress);
  }

  // Step 7: Output a CSV file detailing the path of the boat through the
  // transect. This is helpful for identifying the locations of particular
  // pings, interpolating depth, and data validation.
  if(pathAndDepth) {
    add_text("\nGenerating GPS track for the boat.\n");
    bool boatPath = boatPathCSV(savefolder, fileData, LineStarts, count);
    if(!boatPath) add_text("Boat path could not be generated\n");
  }

  delete[] fileData;
  delete[] LineStarts;
  delete[] breakPoints;
  
  // Tell the user we have finished.
  add_text("\nDone with ");
  if(port) add_text("port");
  else add_text("starboard");
  add_text(" file:\n");
  add_text(filename);
  add_text("\n");
}
