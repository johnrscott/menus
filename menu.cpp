/** 
 * @file menu.cpp
 * @authors J Scott, O Thomas
 * @date Jan 2019 
 *
 * @detail Implementation of menu classes
 *
 */

#include "menu.hpp"

// Constructor
FancyTerm::FancyTerm() {
  
  // Initialize curses 
  initscr();
  start_color();
  // read inputs without carrige return
  cbreak();
  // don't echo inputs to term
  noecho();
  // enable F-keys and Arrow keys
  keypad(stdscr, TRUE);
  
}

// Destructor
FancyTerm::~FancyTerm() {
  endwin();
}  

// The menu that is currently visible
Menu * Menu::current_menu = nullptr;

// Relating to background thread
int Menu::background_flag = 0;
int Menu::background_running = 0;

void UserPtr_t<Menu>::execute() {
  oldmenu.hide();
  submenu.show();
  // Set new current menu
  Menu::set_current_menu(&submenu);
}

// Constructor for switching from one menu to another
UserPtr_t<Menu>::UserPtr_t( Menu & submenu, Menu & oldmenu)
  : submenu(submenu), oldmenu(oldmenu) {
  
  // Add a back button to the submenu
  // Done here so that every submenu definitely gets a back button
  submenu.add_back_button(&oldmenu);
}
