#pragma once

#include <iostream>
#include <functional>
#include <dlfcn.h>
#include <string>
#include <type_traits>

// Event class that stores the timestamp and callable functions
class Event {
public:
    double timestamp;
    std::function<void()> funcCall;
    std::function<void()> undoFuncCall;

    Event(double ts, std::function<void()> funcCall, std::function<void()> undoFuncCall)
        : timestamp(ts), funcCall(funcCall), undoFuncCall(undoFuncCall) {}

    void call() {
        if (funcCall)
            funcCall();
    }

    void callUndo() {
        if (undoFuncCall)
            undoFuncCall();
    }
};

// Helper function to wrap arguments in std::ref if they are references
template <typename T>
auto wrapArgument(T&& arg) -> decltype(std::forward<T>(arg)) {
    return std::forward<T>(arg);
}

template <typename T>
std::reference_wrapper<T> wrapArgument(T& arg) {
    return std::ref(arg);
}

// Helper function to call a function with arguments from a tuple
template <typename FuncType, typename TupleType, std::size_t... I>
void callWithTupleImpl(FuncType func, TupleType& t, std::index_sequence<I...>) {
    func(std::get<I>(t)...);
}

template <typename FuncType, typename TupleType>
void callWithTuple(FuncType func, TupleType& t) {
    constexpr auto Size = std::tuple_size<TupleType>::value;
    callWithTupleImpl(func, t, std::make_index_sequence<Size>{});
}

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

// Modified CreateEvent function template
template <typename FuncType, typename... Args>
Event CreateEvent(double timestamp, FuncType func, Args&&... args) {
    // Wrap arguments appropriately
    auto boundArgs = std::make_tuple(wrapArgument(std::forward<Args>(args))...);

    // Create a bound function call using a lambda
    auto funcCall = [func, boundArgs]() mutable {
        callWithTuple(func, boundArgs);
    };

    // Get the unmangled name of func
    std::string functionName = getUnmangledName(func);

    if (!functionName.empty()) {
        // Construct the reverse function name
        std::string reverseFunctionName = "__undo_" + functionName;

        // Use dlsym to get the reverse function pointer
        void* symbol = dlsym(RTLD_DEFAULT, reverseFunctionName.c_str());
        if (!symbol) {
            std::cerr << "Could not find reverse handler: " << reverseFunctionName << "\n";
            std::cerr << "dlsym error: " << dlerror() << std::endl;
            // Handle error as appropriate
            return Event(timestamp, funcCall, nullptr);
        } else {
            // Cast the symbol to the function pointer type
            using UndoFuncType = FuncType;

            UndoFuncType undoFunc = reinterpret_cast<UndoFuncType>(symbol);

            // Create a bound undo function call
            auto undoFuncCall = [undoFunc, boundArgs]() mutable {
                callWithTuple(undoFunc, boundArgs);
            };

            return Event(timestamp, funcCall, undoFuncCall);
        }
    } else {
        std::cerr << "Failed to get unmangled function name.\n";
        return Event(timestamp, funcCall, nullptr);
    }
}