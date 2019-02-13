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
#include <functional>

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
template<typename T, typename... R>
class UserPtr_t : public UserPtr { // Templated type (_t)
private:
  T & action; // A callable object 
  Menu & oldmenu;
  std::tuple<R...> data;
  
public:
  // Constructor for generic callable function
  UserPtr_t(T & action, Menu & oldmenu, R... data)
    : action(action), oldmenu(oldmenu), data(data...) { }

  void execute() {
    action();
  }
};

/** 
 * @brief Specialisation to the case where the first argument is Menu
 *
 * @detail
 *
 */
template<typename... R>
class UserPtr_t<Menu, R...> : public UserPtr { // Templated type (_t)
private:
  Menu & submenu; // Either a submenu (Menu type) or callable object
  Menu & oldmenu;
  std::tuple<R...> data;  

public:
  // Constructor for switching from one menu to another
  // This is a very tricky function
  UserPtr_t(Menu & submenu, Menu & oldmenu, R... data);
  
  // Define the menu-switch function
  void execute();
};


// Non template base class for pointer declaration

class Arguments {
private:
public:
  virtual ~Arguments() = 0; // To force abstract class
};

template<typename T, typename... R>
class Arguments_t : public Arguments {
private:
  std::tuple<R...> args;
public:
  // Store a list of arguments
  Arguments_t(R... args) : args(args...) { }
  // Get args
  std::tuple<R...> get_args() {
    return args;
  }
  
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
  std::function<void()> go_back; // Function to go back to previous menu
  Arguments * args; // Holds the argument list for the final function
  
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
	case KEY_DC: // Delete character 
	  // Check if the go_back function is defined
	  if(current_menu -> go_back != nullptr)
	    // Execute the go_back function
	    current_menu -> go_back();
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

    // Initialise pointer to arguments
    // I think maybe the type (void here) doesn't matter because
    // the list of voids is zero length
    args = new Arguments_t<void>();
    
  }

  void create_menu() {

    // Create new menu
    // Since std::vector<ITEM*> stores elements contiguously in
    // memory, the required argument ITEM** is a pointer to the first
    // element of menu_items.
    //
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
    
    // Refresh
    refresh();
    
  }
  
  static void set_current_menu(Menu * menu) {
    Menu::current_menu = menu;
  }
  
  void add_back_button(Menu * oldmenu) {

    // This code is run from the submenu object

    // Check if there is already a back button
    if(back_button == 1)
      return; // Don't add another button
    
    // Store the previous menu (above the current menu)
    previous = oldmenu;

    // Set the go back function
    //
    // Can't directly use a lambda function because it goes
    // out of scope at the end of this function. The go_back
    // variable is a std::function object.
    //
    go_back = [this, oldmenu]() {
      this -> hide();
      oldmenu -> show();
      // Set new current menu
      Menu::set_current_menu(previous);
    };

    // Add action item
    this -> add("Back", this->go_back);
    
    // Indicate there is a back button
    back_button = 1;

  }

  // Add arguments to the front of the args std::tuple
  template<typename... R>
  void add_args(R... new_args) {
    auto old_args = args -> get_args();


    auto result = std::tuple_cat(std::tuple<R...>(new_args...), args);
    args = result;
  }

  // Get arguments from the menu
  template<typename... R>
  std::tuple<R...> get_args() {
    return args;
  }

  
  // Reset the args std::tuple
  //void reset_args() {
  //  args = {}; // Is this what you would think it is...?
  //}
  
  // This method foregrounds the current menu, overwriting
  // whichever menu is currently in view
  void show() {

    // Colors
    attron(COLOR_PAIR(2));
    mvprintw(LINES - 3, 0, "Press <ENTER> to see the option selected");
    mvprintw(LINES - 2, 0, "Use Arrow Keys to navigate (F1 to Exit)");
    attroff(COLOR_PAIR(2));

    
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
  template<typename T, typename... R>
  void add(const char * name,
	   T & action,
	   R... data);

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

// The placement of some of these template definitions is critical.
// Firstly, they can't go in the source file (normally). The general
// rule is that the template has to come before the place where it's
// used, otherwise the compiler doesn't know which version to instantiate.
// Putting them in a .cpp file would make them innaccessible in files that
// include this .hpp file, and so you'd get linking errors when the compiler
// hasn't generated the correct versions of the function.
//
// Secondly, there are circular dependencies between the Menu and UserPtr
// classes, meaning it is not possible to place all the Menu stuff above
// UserPtr or vice versa. In addition, the above template considerations also
// apply inside this file (because both Menu and UserPtr use templates). The
// current arrangement works because all template declarations appear before
// they are used, and all functions are declared before they are used.


// Constructor for switching from one menu to another
template<typename... R>
UserPtr_t<Menu, R...>::UserPtr_t( Menu & submenu, Menu & oldmenu, R... data)
  : submenu(submenu), oldmenu(oldmenu), data(data...) {
  
  // Add a back button to the submenu
  // Done here so that every submenu definitely gets a back button
  submenu.add_back_button(&oldmenu);
}

template<typename... R>
void UserPtr_t<Menu, R...>::execute() {
  oldmenu.hide();
  auto old_args = oldmenu.get_args();
  submenu.show();
  submenu.add_args(old_args);
  // Set new current menu
  Menu::set_current_menu(&submenu);
}

template<typename T, typename... R>
void Menu::add(const char * name,
	 T & action,
	 R... data) {

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
  user_pointers.push_back(new UserPtr_t<T, R...>(action, *this, data...)); // Deleted on clear_all?
    
  // Associate the new action to the last menu item
  set_item_userptr(*(menu_items.end()-1-offset), (void*)user_pointers.back());
    
  // Create the menu
  create_menu();
    
  // This seems critical
  refresh();
}

