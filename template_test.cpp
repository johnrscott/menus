#include <tuple>
#include <iostream>

// The conclusion from this test is that you can use an
// empty parameter pack in a function argument, and also
// pass it to other functions. The problem is when you try
// to use it to do something (such as initialise a variable)
// where an empty list wouldn't be syntactically correct

template<typename... C>
void other(C... c) {
  //std::tuple<C...> data(c...);
  //std::cout << std::get<0>(data) << std::endl;
}

template<typename A, typename B, typename... C>
void thing(A a, B b, C... c) {
  other(c...);
  //std::tuple<C...> data(c...);
  //std::cout << std::get<0>(data) << std::endl;
}

int main() {
  thing(1,2,3);
  thing(1,2);
}
