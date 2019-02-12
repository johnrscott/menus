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
public:
  // Constructor and destructor
  FancyTerm();
  ~FancyTerm();
};


class Menu;

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
  // Call this function to execute the menu function
  virtual void execute() = 0;
};

/** 
 * @brief Templated class for holding menu actions
 *
 * @detail
 *
 */
template<typename T>
class UserPtr_t : public UserPtr { // Templated type (_t)
private:
  T & action; // A callable object 
  Menu & oldmenu;
  
public:
  // Constructor for generic callable function
  UserPtr_t(T & action, Menu & oldmenu)
    : action(action), oldmenu(oldmenu) { }
  
  void execute() {
    action(); // 
  }
};

/** 
 * @brief Specialisation to the case where the first argument is Menu
 *
 * @detail
 *
 */
template<>
class UserPtr_t<Menu> : public UserPtr { // Templated type (_t)
private:
  Menu & submenu; // Either a submenu (Menu type) or callable object
  Menu & oldmenu;
  
public:
  // Constructor for switching from one menu to another
  UserPtr_t(Menu & submenu, Menu & oldmenu);
  
  // Define the menu-switch function
  void execute();
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
  std::vector<UserPtr * > user_pointers; // Holds submenu pointers

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

  void add_back_button(Menu * oldmenu) {

    // This code is run from the submenu object
    
    // It seems like you have to free the menu before messing around
    // with menu_items
    //unpost_menu(menu);
    //free_menu(menu);

    // Store the previous menu (above the current menu)
    previous = oldmenu;

    // Add the new item onto the end of menu list
    //menu_items.insert(menu_items.end()-1, // i.e. before the nullptr
    //		      new_item("Back", "Go back to previous menu"));

    // When is this going to get deleted?
    // Also where is it getting stored
    //UserPtr_t<Menu> * menu_switch = new UserPtr_t<Menu>(* previous_menu, * this);

    // Prepare the go-up-one-level function
    auto go_back = [this, oldmenu]() {
      this -> hide();
      oldmenu -> show();
      // Set new current menu
      Menu::set_current_menu(oldmenu);
    };

    // Add action item
    this -> add("Back", go_back);
    
    // Add a new submenu pointer to the list
    //user_pointers.push_back(new UserPtr_t<Menu>(* previous_menu, * this)); // Deleted on clear_all?
    
    // Associate the foreground menu action with the item
    //set_item_userptr(* (menu_items.end() - 2), (void * )user_pointers.back());

    // Create new menu
    // Since std::vector<ITEM*> stores elements contiguously in
    // memory, the required argument ITEM** is a pointer to the first
    // element of menu_items.
    //
    //menu = new_menu(&menu_items[0]);
    //create_menu();
    
    // Indicate there is a back button
    back_button = 1;

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
   * @brief Add an action menu item
   *
   * @detail Add a menu item which performs an action (calls a function)
   * when it is selected. The action can either be a function or a 
   * submenu. The two options are dealt with using the templated 
   * constructor of UserPtr.
   *
   */
  template<typename T>
  void add(const char * name,
	   T & action) {

    // It seems like you have to free the menu before messing around
    // with menu_items
    unpost_menu(menu);
    free_menu(menu);
    
    // Set back button offset
    int offset;
    if(back_button == 0) offset = 1;
    else offset = 2;
    
    // Add the new item onto the end of menu list
    menu_items.insert(menu_items.end()-offset,
		      new_item(name, "Default description")); // Before nullptr

    // Add a new submenu pointer to the list
    // The first argument is the action or submenu, the second is the higher
    // level menu
    user_pointers.push_back(new UserPtr_t<T>(action, *this)); // Deleted on clear_all?
    
    // Associate the new action to the last menu item
    set_item_userptr(*(menu_items.end()-1-offset), (void*)user_pointers.back());
    
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
