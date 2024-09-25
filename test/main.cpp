#include <iostream>
#include <dlfcn.h>
#include <string>
#include <type_traits>
#include <cxxabi.h>
#include "test_funcs.hpp"
#include "event.hpp"


int main() {
    int a = 1;

    // Create an event
    Event e = CreateEvent(20.0, freeFunctionBinaryAddByOne, a);

    // Call the function
    std::cout << "Before function call: " << a << std::endl;
    e.call();
    std::cout << "After function call: " << a << std::endl;

    // Call the undo function
    std::cout << "Before undo function call: " << a << std::endl;
    e.callUndo();
    std::cout << "After undo function call: " << a << std::endl;

    return 0;
}