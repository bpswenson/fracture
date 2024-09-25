#include <iostream>
#include <dlfcn.h>
#include "test_funcs.hpp"

typedef void (*UndoFuncType)(int&);

#include <iostream>
#include <dlfcn.h>
#include <string>
#include <cxxabi.h>

typedef void (*UndoFuncType)(int&);

UndoFuncType findGeneratedReverseHandler(const std::string& mangledName) {
    // Open the main executable to search for symbols
    void* handle = dlopen(nullptr, RTLD_NOW);
    if (!handle) {
        std::cerr << "dlopen failed: " << dlerror() << std::endl;
        return nullptr;
    }

    // Use the mangled name in dlsym
    void* symbol = dlsym(handle, mangledName.c_str());
    if (!symbol) {
        std::cerr << "Could not find reverse handler: " << mangledName << "\n";
        std::cerr << "dlsym error: " << dlerror() << std::endl;
        return nullptr;
    } else {
        std::cout << "Reverse handler found: " << mangledName << "\n";
        // Cast the symbol to the correct function pointer type
        return reinterpret_cast<UndoFuncType>(symbol);
    }
}

int main() {
    int state = 1;

    // Use the exact mangled name
    std::string mangledName = "__undo_freeFunctionBinaryAddByOne(int&)"; // Replace with actual mangled name

    // Find the reverse function
    UndoFuncType undoFunc = findGeneratedReverseHandler(mangledName);

    if (!undoFunc) {
        std::cerr << "Could not find reverse function: " << mangledName << std::endl;
        return 1;
    }

    // Execute the forward function
    std::cout << "Before forward function call: " << state << std::endl;
    freeFunctionBinaryAddByOne(state);
    std::cout << "After forward function call: " << state << std::endl;

    // Invoke the reverse function
    std::cout << "Before reverse function call: " << state << std::endl;
    undoFunc(state);
    std::cout << "After reverse function call: " << state << std::endl;

    return 0;
}
