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

MainWindow::MainWindow() : main_layout(Gtk::ORIENTATION_VERTICAL),
                           bottom_buttons(Gtk::ORIENTATION_HORIZONTAL),
                           console(Gtk::ORIENTATION_HORIZONTAL),
                           button_start("Start"),
                           button_open("Open"),
                           button_settings("Settings"),
                           port(true),
                           maxLines(300),
                           minLines(10),
                           angle(100),
                           soundspeed(1463),
                           frequency_s(455000),
                           frequency_d(200000),
                           l_speed("Speed of Sound"),
                           l_freq_s("Sidescan Frequency"),
                           l_freq_d("Down Frequency"),
                           l_unit("Model"),
                           l_tiff_max("Maximum TIFF Scans"),
                           l_tiff_min("Minimum TIFF Scans"),
                           l_heading("Maximum Heading Change"),
                           button_update("Update"),
                           button_cancel("Cancel"),
                           ready(false),
                           pathAndDepth(true),
                           writeTIFF(true),
                           writeCSV(false) {
  set_border_width(10);
  set_size_request(800,600);
  button_start.set_sensitive(false);
  button_open.set_sensitive(false);
  combo_side.set_sensitive(false);
  program_output.set_sensitive(false);
  button_start.signal_clicked().connect(sigc::mem_fun(*this,&MainWindow::on_button_start));
  button_settings.signal_clicked().connect(sigc::mem_fun(*this,&MainWindow::on_button_configure));
  button_open.signal_clicked().connect(sigc::mem_fun(*this,&MainWindow::on_button_open));
  combo_side.signal_changed().connect(sigc::mem_fun(*this,&MainWindow::on_combo_changed));
  combo_side.append("Port");
  combo_side.append("Starboard");
  // Gtk::VScrollbar console_scrollbar(program_output.get_vadjustment());
  // program_output.set_size_request(600,600);
  console_window.add(console);
  console.pack_start(program_output); //,Gtk::PACK_EXPAND_WIDGET);
  // console.pack_start(console_scrollbar,Gtk::PACK_SHRINK);
  text_buffer = program_output.get_buffer();
  add(main_layout);
  main_layout.pack_start(console_window,Gtk::PACK_EXPAND_WIDGET);
  main_layout.pack_start(operation_progress,Gtk::PACK_SHRINK);
  main_layout.pack_start(bottom_buttons,Gtk::PACK_SHRINK);
  bottom_buttons.pack_start(combo_side);
  bottom_buttons.pack_start(button_settings);
  bottom_buttons.pack_start(button_open);
  bottom_buttons.pack_start(button_start);

  // Configure the settings dialog
  button_update.signal_clicked().connect(sigc::mem_fun(*this,&MainWindow::update_settings));
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
  settings.attach(button_update,0,7,1,1);
  settings.attach(button_cancel,1,7,1,1);
  dialog_settings.set_default_size(200,200);
  dialog_settings.get_content_area()->pack_start(settings);

  show_all_children();
  add_text("Welcome to SonarGridder v0.0.1\n");
  add_text("Copyright ©2021 Jim Birch\nhttps://angularfish.net\n");
  add_text("This program is free software published under the GNU GPL v.2. ");
  add_text("Please see the LICENSE file for more information.");
  add_text("\n\n\n");
  add_text("Workflow:\n");
  add_text("Settings →  Side → Open → Start\n");
  add_text("Configure your unit and study site in settings.\n");
  add_text("Select either the port or starboard side transducer in the combo box.\n");
  add_text("HINT: Port is typically B002.SON; starboard is typically B003.SON.\n");
}

MainWindow::~MainWindow() {
}

void MainWindow::add_text(Glib::ustring newtext) {
  Gtk::TextIter iterator = text_buffer->end();
  text_buffer->insert(iterator, newtext);
  Glib::RefPtr<Gtk::Adjustment> adj = console_window.get_vadjustment();
  while(Gtk::Main::events_pending()) Gtk::Main::iteration();
  adj->set_value(adj->get_upper());
}

void MainWindow::on_combo_changed() {
  button_open.set_sensitive(true);
  Glib::ustring text = combo_side.get_active_text();
  Glib::ustring starboard = "Starboard";
  if(!text.empty()) std::cout << text << std::endl;
  if(text == starboard) port = false;
  else port = true;
}

void MainWindow::on_button_start() {
  add_text("Please specify an output folder. \n");
  savefolder = open_dialog("Choose a folder to save output", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
  if(savefolder == "try again") {
    add_text("No folder specified. Please try again. \n");
    return;
  }
  savefolder = savefolder + ("/");
  add_text("Files will be output in: ");
  add_text(savefolder);
  add_text("\n");
  button_start.set_sensitive(false);
  run_sidescan();
}

void MainWindow::on_button_open() {
  add_text("Please select a SON file to open for the ");
  add_text(combo_side.get_active_text());
  add_text(" side transducer.\n");
  if(port) add_text("hint: Port is typically B002.SON\n");
  else add_text("hint: Starboard is typically B003.SON\n");
  filename = open_dialog("Choose a SON file", Gtk::FILE_CHOOSER_ACTION_OPEN);
}

std::string MainWindow::open_dialog(std::string title_text, Gtk::FileChooserAction opensave) {
  Gtk::FileChooserDialog dialog(title_text, opensave);
  dialog.set_transient_for(*this);

  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  if(opensave == Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER) {
    dialog.add_button("Select", Gtk::RESPONSE_OK);
  }

  if(opensave == Gtk::FILE_CHOOSER_ACTION_OPEN) {
    auto filter_son = Gtk::FileFilter::create();
    filter_son->set_name("Humminbird .SON files");
    filter_son->add_pattern("*.SON");
    dialog.add_filter(filter_son);
    dialog.add_button("_Open", Gtk::RESPONSE_OK);
  }

  int result = dialog.run();
  switch(result) {
    case(Gtk::RESPONSE_OK): {
      std::string selection = dialog.get_filename();
      if(ready) button_start.set_sensitive(true);
      return selection;
      break;
    }
    case(Gtk::RESPONSE_CANCEL): {
      add_text("Cancelling \n");
      return "try again";
      break;
    }
    default: {
      return "try again";
      break;
    }
  }
}

void MainWindow::on_button_configure() {
  e_speed.set_text(std::to_string(soundspeed));
  e_freq_s.set_text(std::to_string(frequency_s));
  e_freq_d.set_text(std::to_string(frequency_d));
  e_tiff_max.set_text(std::to_string(maxLines));
  e_tiff_min.set_text(std::to_string(minLines));
  e_heading.set_text(std::to_string(angle));
  e_speed.set_input_purpose(Gtk::INPUT_PURPOSE_DIGITS);
  e_freq_s.set_input_purpose(Gtk::INPUT_PURPOSE_DIGITS);
  e_freq_d.set_input_purpose(Gtk::INPUT_PURPOSE_DIGITS);
  e_tiff_max.set_input_purpose(Gtk::INPUT_PURPOSE_DIGITS);
  e_tiff_min.set_input_purpose(Gtk::INPUT_PURPOSE_DIGITS);
  e_heading.set_input_purpose(Gtk::INPUT_PURPOSE_DIGITS);


  dialog_settings.show_all();
}

void MainWindow::update_settings() {
  combo_side.set_sensitive(true);
  maxLines = (uint32_t)std::atoi(e_tiff_max.get_text().c_str());
  minLines = (uint32_t)std::atoi(e_tiff_min.get_text().c_str());
  angle = (uint32_t)std::atoi(e_heading.get_text().c_str());
  soundspeed = (uint32_t)std::atoi(e_speed.get_text().c_str());
  frequency_s = (uint32_t)std::atoi(e_freq_s.get_text().c_str());
  frequency_d = (uint32_t)std::atoi(e_freq_s.get_text().c_str());
  add_text("Settings updated.\n");
  add_text("Current settings:\n");
  add_text("MaxLines: ");
  add_text(std::to_string(maxLines));
  add_text("\nminLines: ");
  add_text(std::to_string(minLines));
  add_text("\nangle: ");
  add_text(std::to_string(angle));
  add_text("\nspeed of sound: ");
  add_text(std::to_string(soundspeed));
  add_text("\n");
  dialog_settings.hide();
  ready = true;
}

void MainWindow::run_sidescan() {
  streampos size;
  char* sFileData;

  // Read file into memory. If files end up being really f'in huge this might
  // be a bad idea.
  ifstream sonFile(filename, ios::in | ios:: binary | ios::ate);
  if(!sonFile.is_open()) {
    add_text("Unable to open file: ");
    add_text(filename);
    add_text("\n");
    return;
  }
  size = sonFile.tellg();
  sFileData = new char[size];
  sonFile.seekg(0, ios::beg);
  sonFile.read(sFileData, size);
  sonFile.close();
  add_text("File: ");
  add_text(filename);
  add_text(" read successfully.\n");

  // Reinterpret the file as an unsigned char. This is important for using the
  // same functions for line header data in files and lines.
  unsigned char* fileData = reinterpret_cast<unsigned char*>(sFileData);

  // Determine file properties essential to functions.
  int count = countLines(fileData, (int)size);
  bool* breaks = new bool[count];
  for(int i = 0; i < count; i++) breaks[i] = false;
  breakOnDirectionChange(fileData, breaks, count, angle);
  breakOnNoPosition(fileData, breaks, count);
  breakOnMaxLines(breaks, count, maxLines);

  // Tell the user how their current settings will be output.
  int bpCount = 0;
  for(int i = 0; i < count; i++) if(breaks[i]) bpCount++;
  add_text("\nUnder current settings there are:\n");
  add_text(std::to_string(bpCount));
  add_text(" break points\n");

  unsigned int* LineStarts = lineStarts(fileData, (int)size, count);
  int* breakPoints = new int[bpCount];
  int bpCurr = 0;
  int skippedOutputs = 0;
  for(int i = 0; i < count; i++) if(breaks[i]) {
    breakPoints[bpCurr] = i;
    if(minLines > (i - breakPoints[bpCurr - 1])) skippedOutputs++;
    bpCurr++;
  }
  add_text(std::to_string(skippedOutputs));
  add_text(" TIFF files are to short to be generated\n");

  int numberFiles = bpCount - skippedOutputs;
  add_text(std::to_string(numberFiles));
  add_text(" total TIFF files will be output\n");
  
  unsigned int lineLen = 0;
  for(int i = 0; i < count - 1; i++) if((LineStarts[i+1] - LineStarts[i]) > lineLen) {
    lineLen = LineStarts[i+1] - LineStarts[i];
  }
  add_text(std::to_string(lineLen));
  add_text(" is the maximum line length\n");

  operation_progress.set_fraction(0.0);

/*  if(writeCSV) add_text("\nGenerating signal strength CSV.\n");
  for(int i = 0; i < bpCurr - 1; i++) {
    double progress = double(i) / double(bpCurr);
    if(!writeCSV) break;
    bool outputCSV = generateSideScanCSV(savefolder, fileData, lineStarts, lineLen,
                                         breakPoints[i], breakPoints[i+1], port);
    operation_progress.set_fraction(progress);
  } */

  operation_progress.set_fraction(0.0);

  if(writeTIFF) add_text("\nGenerating side-scan TIFFs.\n");
  for(int i = 0; i < bpCurr - 1; i++) {
    double progress = double(i) / double(bpCurr);
    if(!writeTIFF) break;
    bool outputTIFF = generateTIFF(savefolder, fileData, LineStarts, lineLen, 
                                   breakPoints[i], breakPoints[i+1], port, minLines);
    if(!outputTIFF) {
      if(minLines > (breakPoints[i+1] - breakPoints[i])) {
        add_text("TIFF for lines: ");
        add_text(std::to_string(breakPoints[i]));
        add_text(" to ");
        add_text(std::to_string(breakPoints[i + 1]));
        add_text(" is too short to be output.\n");
      } else {
        add_text("Unspecified TIFF error for lines: ");
        add_text(std::to_string(breakPoints[i]));
        add_text(" to ");
        add_text(std::to_string(breakPoints[i + 1]));
        add_text(" may be a GPS error. Check world file.\n");
      }
    }
    operation_progress.set_fraction(progress);
  }
  
  if(pathAndDepth) {
    add_text("\nGenerating GPS track for the boat.\n");
    bool boatPath = boatPathCSV(savefolder, fileData, LineStarts, count);
    if(!boatPath) add_text("Boat path could not be generated\n");
  }

  delete[] fileData;
  delete[] LineStarts;
  delete[] breakPoints;
  
  add_text("Done with file: ");
  add_text(filename);
  add_text("\n");

}
