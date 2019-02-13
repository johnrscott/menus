/** 
 * @file menu-test.cpp
 * @authors J Scott, O Thomas
 * @date Jan 2019 
 *
 * @detail Testing the menu class
 *
 */

#include <string>
#include "menu.hpp"

void arg_test(){//std::string a, int b, double c) {
  return;  
}

int main() {
  
  // Create ncurses background
  FancyTerm term;

  // Menus
  Menu string_menu;
  Menu integer_menu; 
  Menu fraction_menu;

  // Choice of string
  string_menu.add("ABC", integer_menu, "ABC");
  string_menu.add("DEF", integer_menu, "DEF");
  string_menu.add("GHI", integer_menu, "GHI");
  
  // Choice of integer
  integer_menu.add("1", fraction_menu, 1);
  integer_menu.add("2", fraction_menu, 2);
  integer_menu.add("3", fraction_menu, 2);

  // Choice of fraction
  fraction_menu.add("0.1", arg_test, 0.1);
  fraction_menu.add("0.2", arg_test, 0.2);
  fraction_menu.add("0.3", arg_test, 0.3);  

  // Show the top level menu
  string_menu.show();
  
  // Stop program exiting
  while(1);
  
}
