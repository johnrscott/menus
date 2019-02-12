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

// Constructor
UserPtr_t<Menu & >::UserPtr_t(Menu & oldmenu, Menu & submenu)
  : UserPtr(oldmenu), action(action) {
  
  call = [oldmenu, submenu](){
    oldmenu.hide();
    submenu.show();
    // Set new current menu
    Menu::set_current_menu(&submenu);
  }
  
}


// The menu that is currently visible
Menu * Menu::current_menu = nullptr;

// Relating to background thread
int Menu::background_flag = 0;
int Menu::background_running = 0;
