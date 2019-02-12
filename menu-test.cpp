/** 
 * @file menu-test.cpp
 * @authors J Scott, O Thomas
 * @date Jan 2019 
 *
 * @detail Testing the menu class
 *
 */

#include "menu.hpp"

// Probably needs to be static for compatibility with ncurses
void test() {
  move(20, 0);
  clrtoeol();
  mvprintw(20, 0, "Test");
}
void test1() {
  move(20, 0);
  clrtoeol();
  mvprintw(20, 0, "Test1");
}

int main() {
  
  // Create ncurses background
  FancyTerm term;
  
  // Menu object
  Menu menu;

  // Submenu
  Menu submenu;
  Menu another;
  
  // Add menu items
  menu.add("Item 1", test);
  menu.add("Item 2", test1);
  menu.add("Submenu", submenu);

  submenu.add("Subitem 1", test);
  submenu.add("Subitem 2", test);
  submenu.add("Subitem 3", test);
  submenu.add("Subitem 4", test);

  submenu.add("Submenu", another);
  
  menu.show();

  
  //
  
  // Clear menu
  //menu.clear_all();

  // Add new items
  //menu.add_action_item("New item 1", "Test item added dynamically", action);
  //menu.add_action_item("New item 2", "Test item added dynamically", action);

  
  // Stop program exiting
  while(1);
  
}
