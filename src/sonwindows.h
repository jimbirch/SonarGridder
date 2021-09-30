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
║Tested with data produced by a Humminbird 698ci HD manufactured circa 2014.   ║
║May work with other units with or without modification.                       ║
╟──────────────────────────────────────────────────────────────────────────────╢
║   Please let me know if you find this software useful! jim[át]jdbirch.com    ║
║                 (Or if you can't get it to run at all)                       ║
╚══════════════════════════════════════════════════════════════════════════════╝
*/
#ifndef SONGRIDDER_GUI_H
#define SONGRIDDER_GUI_H

#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/textview.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/window.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/label.h>
#include <gtkmm/grid.h>
#include <gtkmm/entry.h>
#include <gtkmm/orientable.h>
#include <gtkmm/dialog.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/textiter.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/widget.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/scrollable.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/main.h>

class MainWindow : public Gtk::Window {
  public:
    MainWindow();
    virtual ~MainWindow();

  protected:
    // Variables
    bool port;
    bool ready;
    bool writeCSV;
    bool writeTIFF;
    bool pathAndDepth;
    double adjustDepths;
    uint32_t maxLines;
    uint32_t minLines;
    uint32_t angle;
    uint32_t soundspeed;
    double correction_non_hardware;
    uint32_t frequency_d;
    std::string filename;
    std::string savefolder;
    Glib::RefPtr<Gtk::TextBuffer> text_buffer;
    // Signal handlers
    void on_salinity_changed();
    void on_combo_changed();
    void on_button_start();
    void on_button_open();
    void on_button_configure();
    void update_settings();
    // Other functions
    std::string open_dialog(std::string title_text, Gtk::FileChooserAction opensave);
    void add_text(Glib::ustring newtext);
    void log_text();
    void run_sidescan();
    // Widgets
    Gtk::ComboBoxText combo_side, combo_salinity;
    Gtk::Box main_layout, bottom_buttons, console;
    Gtk::Grid settings;
    Gtk::ProgressBar operation_progress;
    Gtk::TextView program_output;
    Gtk::Button button_start, button_open, button_settings, button_update;
    Gtk::Dialog dialog_settings;
    Gtk::ScrolledWindow console_window;
    // Settings dialog
    Gtk::Label l_speed, l_freq_d, l_salinity, l_unit, l_tiff_max, l_tiff_min, l_heading;
    Gtk::Entry e_speed, e_freq_d, e_tiff_max, e_tiff_min, e_heading;
    Gtk::ComboBoxText c_unit;
};
#endif
