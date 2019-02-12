/** 
 * @file menu.hpp
 * @authors J Scott, O Thomas
 * @date Jan 2019 
 *
 * @detail Classes for dealing with curses menus
 *
 */

#include <iostream>
#include <menu.h>
#include <curses.h>
#include <thread>
#include <vector>
#include <cerrno>
#include <cstring>


/**
 * @brief Class for managing ncurses
 *
 * @detail 
 */
class FancyTerm {
private:
  
  
public:
  // Constructor and destructor
  FancyTerm();
  ~FancyTerm();
};

/** 
 * @brief Userptr class
 *
 * @detail Contains data for executing functions when a menu item is
 * selected. It should be cast to void* and past to the set_item_userptr
 * function
 *
 */
class UserPtr {
private:

public:
  virtual void execute() = 0;
};

template<typename T>
class Action : public UserPtr {
private:
  const char * name;
  T func;

public:
  Action(const char * name) : name(name) { }

  Action(T func) : func(func) { }

  void execute() {
    // Call the function
    func();
  }

};

class Menu;

class MenuLink : public UserPtr {
private:
  Menu & submenu;
  Menu & oldmenu;
public:
  MenuLink(Menu & submenu, Menu & oldmenu)
    : submenu(submenu), oldmenu(oldmenu) { }
  void execute(); // Defined later
};

/**
 * @brief Menu handling class
 *
 * @detail The point of this class is to make menu handling with ncurses easier
 * and more c++ compatible. Making an object of the Menu type will take care 
 * of setting up (and subsequently releasing) ncurses resources. It also 
 * creates a thread which polls getch for characters and handles menu item
 * navigation. 
 *
 */
class Menu {
private:
  std::vector<ITEM * > menu_items;
  int back_button = 0; // Set to 1 if there is a back button
  MENU * menu;
  static Menu * current_menu; // The menu that is currently visible
  Menu * previous; // The previous menu for submenu back button
  std::thread background;
  static int background_flag; // Set to one to stop the thread
  static int background_running; // Set to one when background is running
  std::vector<MenuLink * > submenus; // Holds submenu pointers

  WINDOW * menu_win;
    
  // Start background
  void start_background() {
    
    // Start background thread if not already started
    if(background_running == 0) {
      background = std::thread(Menu::navigate);
      background_running = 1;
    }
    
  }
  
  // Stop background
  void stop_background() {
    
    // Wait for background to end
    if(background.joinable()) {
      background_flag = 1;
      background.join();
      background_flag = 0;
      background_running = 0;
    }
  }

  void add_back_button(Menu * previous_menu) {

    // It seems like you have to free the menu before messing around
    // with menu_items
    unpost_menu(menu);
    free_menu(menu);

    // Store the previous menu (above the current menu)
    previous = previous_menu;

    // Add the new item onto the end of menu list
    menu_items.insert(menu_items.end()-1, // i.e. before the nullptr
		      new_item("Back", "Go back to previous menu"));

    // When is this going to get deleted?
    MenuLink * menu_switch = new MenuLink(* previous_menu, * this);
    
    // Associate the foreground menu action with the item
    set_item_userptr(* (menu_items.end() - 2), (void * )menu_switch);

    // Create new menu
    // Since std::vector<ITEM*> stores elements contiguously in
    // memory, the required argument ITEM** is a pointer to the first
    // element of menu_items.
    //
    //menu = new_menu(&menu_items[0]);
    create_menu();
    
    // Indicate there is a back button
    back_button = 1;

  }
  
  /** 
   * @brief Handle menu navigation and selection
   *
   * @detail Every menu shares the same background thread, because only
   * one menu is visible at once, so this method is static.
   *
   */
  static void navigate() {
    while(1) {

      int key = getch();
      if(key == KEY_F(1))
	abort(); // Do something
      if(background_flag == 1) {
	background_flag = 0; // Reset the flag
	std::cout << "Exiting background " << std::endl;
	return; // from thread
      }
      if(current_menu != nullptr && current_menu -> menu != nullptr) {
	switch(key) {
	case KEY_DOWN:
	  menu_driver(current_menu -> menu, REQ_DOWN_ITEM);
	  break;
	case KEY_UP:
	  menu_driver(current_menu -> menu, REQ_UP_ITEM);
	  break;
	case KEY_LEFT:
	  menu_driver(current_menu -> menu, REQ_LEFT_ITEM);
	  break;
	case KEY_RIGHT:
	  menu_driver(current_menu -> menu, REQ_RIGHT_ITEM);
	  break;
	case 10: { // Enter
	  ITEM * cur = current_item(current_menu -> menu);
	  if(cur == nullptr) break;
	  UserPtr * user_ptr = static_cast<UserPtr * >(item_userptr(cur));
	  user_ptr -> execute();
	  pos_menu_cursor(current_menu -> menu);
	  break;
	}
	} // End of switch
      } // End of if
      wrefresh(current_menu -> menu_win);
    } // End of while
  }
  
public:

  // Constructor (called when object is made)
  Menu() : menu_items(1), menu(nullptr), previous(nullptr) {

    // Need to make sure there is a FancyTerm object before doing
    // any of this

    // Initialise colors
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
    
    // Initialise empty menu item list
    menu_items[0] = nullptr;
    
    // If not already started, start the background thread
    start_background();

  }

  void create_menu() {

    // Create new menu
    menu = new_menu(&menu_items[0]);
    if(menu == nullptr) abort();
    
    // Set up menu window
    menu_win = newwin(10, 75, 4, 4);
    keypad(menu_win, TRUE);
    menu_opts_off(menu, O_SHOWDESC);

    
    // Window properties
    set_menu_win(menu, menu_win);
    set_menu_sub(menu, derwin(menu_win, 6, 73, 3, 1));
    set_menu_format(menu, 4, 2);
    set_menu_mark(menu, " * ");

    // Box around menu
    box(menu_win, 0, 0);

    // Colors
    attron(COLOR_PAIR(1));
    mvprintw(LINES - 3, 0, "Use PageUp and PageDown to scroll");
    mvprintw(LINES - 2, 0, "Use Arrow Keys to navigate (F1 to Exit)");
    attroff(COLOR_PAIR(1));

    // Refresh
    refresh();
    
  }
  
  static void set_current_menu(Menu * menu) {
    Menu::current_menu = menu;
  }
  
  // This method foregrounds the current menu, overwriting
  // whichever menu is currently in view
  void show() {

    // Post the menu
    mvprintw(LINES - 3, 0, "Press <ENTER> to see the option selected");
    mvprintw(LINES - 2, 0, "Up and Down arrow keys to naviage (F1 to Exit)");

    // Set new current menu
    post_menu(menu);
    Menu::set_current_menu(this);

    // Refresh the menu
    refresh();
    wrefresh(menu_win);
    
  }

  // Hide the menu
  void hide() {
    unpost_menu(menu);
  }
  
  /** 
   * @brief Add a submenu item
   *
   * @detail Add a menu item which opens another menu when it is selected
   */
  void add(const char * name,
	   Menu & submenu) {

    // It seems like you have to free the menu before messing around
    // with menu_items
    unpost_menu(menu);
    free_menu(menu);

    // Set back button offset
    int offset;
    if(back_button == 0) offset = 1;
    else offset = 2;

    // Add the new item onto the end of menu list (before back)
    menu_items.insert(menu_items.end()-offset,
		      new_item(name, "Default description")); // Before nullptr/back
    
    // Add a new submenu pointer to the list
    submenus.push_back(new MenuLink(submenu, *this)); // Deleted on clear_all?

    // Associate the foreground menu action with the item
    set_item_userptr(*(menu_items.end()-1-offset), (void*) submenus.back());
    
    // Add a back button to the submenu
    submenu.add_back_button(this);

    // Create new menu
    // Since std::vector<ITEM*> stores elements contiguously in
    // memory, the required argument ITEM** is a pointer to the first
    // element of menu_items.
    //
    //menu = new_menu(&menu_items[0]);
    create_menu();
    
    // This seems critical
    refresh();
    
  }
  
  /** 
   * @brief Add an action menu item
   *
   * @detail Add a menu item which performs an action (calls a function)
   * when it is selected
   *
   */
  template<typename R, typename T>
  void add(const char * name,
	   R arg, 
	   T func) {

    // It seems like you have to free the menu before messing around
    // with menu_items
    unpost_menu(menu);
    free_menu(menu);

    // Create new action item
    Action<T> * action = new Action<T>(func);
    
    // Set back button offset
    int offset;
    if(back_button == 0) offset = 1;
    else offset = 2;
    
    // Add the new item onto the end of menu list
    menu_items.insert(menu_items.end()-offset,
		      new_item(name, "Default description")); // Before nullptr

    // Associate the new action to the last menu item
    set_item_userptr(*(menu_items.end()-1-offset), (void*)action);
    
    // Create new menu
    // Since std::vector<ITEM*> stores elements contiguously in
    // memory, the required argument ITEM** is a pointer to the first
    // element of menu_items.
    //
    //menu = new_menu(&menu_items[0]);
    //if(menu == nullptr) abort();
    create_menu();
    
    // This seems critical
    refresh();
  }

    
  /** 
   * @brief Add an action menu item
   *
   * @detail Add a menu item which performs an action (calls a function)
   * when it is selected
   *
   */
  template<typename R, typename T>
  void add(const char * name,
	   T func) {

    // It seems like you have to free the menu before messing around
    // with menu_items
    unpost_menu(menu);
    free_menu(menu);

    // Create new action item
    Action<T> * action = new Action<T>(func);
    
    // Set back button offset
    int offset;
    if(back_button == 0) offset = 1;
    else offset = 2;
    
    // Add the new item onto the end of menu list
    menu_items.insert(menu_items.end()-offset,
		      new_item(name, "Default description")); // Before nullptr

    // Associate the new action to the last menu item
    set_item_userptr(*(menu_items.end()-1-offset), (void*)action);
    
    // Create new menu
    // Since std::vector<ITEM*> stores elements contiguously in
    // memory, the required argument ITEM** is a pointer to the first
    // element of menu_items.
    //
    //menu = new_menu(&menu_items[0]);
    //if(menu == nullptr) abort();
    create_menu();
    
    // This seems critical
    refresh();
  }
  
  // Add a menu item
  void clear_all() {

    // It seems like you have to free the menu before messing around
    // with menu_items
    unpost_menu(menu);
    free_menu(menu);    

    // Reset the menu_items list
    menu_items.clear();
    
    // Add nullptr to the end of the list
    menu_items.push_back(nullptr);
    
    // Create new menu
    // Since std::vector<ITEM*> stores elements contiguously in
    // memory, the required argument ITEM** is a pointer to the first
    // element of menu_items.
    //
    //menu = new_menu(&menu_items[0]);
    //if(menu == nullptr) abort();
    create_menu();
    
    // This seems critical
    refresh();
  }


  
  // Destructor (called when object is deleted)
  ~Menu() {

    // Clear the menu
    clear_all();

    unpost_menu(menu);
    free_menu(menu);
    
  }
};
