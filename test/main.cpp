#include <iostream>
#include <dlfcn.h>
#include <string>
#include <type_traits>
#include <cxxabi.h>
#include "test_funcs.hpp"

// Template function to get the unmangled (demangled) symbol name of any function pointer
template <typename FuncType>
std::string getUnmangledName(FuncType funcPtr) {
    Dl_info info;
    if (dladdr(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(funcPtr)), &info) && info.dli_sname) {
        int status = 0;
        char* demangledName = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
        if (status == 0 && demangledName != nullptr) {
            std::string result(demangledName);
            free(demangledName); // Remember to free the memory allocated by __cxa_demangle
            return result;
        } else {
            // Demangling failed, return the mangled name as a fallback
            return std::string(info.dli_sname);
        }
    } else {
        std::cerr << "dladdr failed to find symbol information." << std::endl;
        return "";
    }
}

int main() {
    // Get the unmangled name of the function
    std::string unmangledName = getUnmangledName(&freeFunctionBinaryAddByOne);

    if (!unmangledName.empty()) {
        std::cout << "Unmangled name: " << unmangledName << std::endl;

        // Construct the reverse function name
        std::string reverseFunctionName = "__undo_" + unmangledName;

        std::cout << "Reverse function name: " << reverseFunctionName << std::endl;

        // Use dlsym to find the reverse function
        typedef void (*UndoFuncType)(int&);
        void* symbol = dlsym(RTLD_DEFAULT, reverseFunctionName.c_str());
        if (!symbol) {
            std::cerr << "Could not find reverse handler: " << reverseFunctionName << "\n";
            std::cerr << "dlsym error: " << dlerror() << std::endl;
        } else {
            std::cout << "Reverse handler found: " << reverseFunctionName << "\n";
            UndoFuncType undoFunc = reinterpret_cast<UndoFuncType>(symbol);

            int state = 1;

            // Execute the forward function
            std::cout << "Before forward function call: " << state << std::endl;
            freeFunctionBinaryAddByOne(state);
            std::cout << "After forward function call: " << state << std::endl;

            // Invoke the reverse function
            std::cout << "Before reverse function call: " << state << std::endl;
            undoFunc(state);
            std::cout << "After reverse function call: " << state << std::endl;
        }
    }

    return 0;
}