//#define _GNU_SOURCE
#include <iostream>
#include <cstdlib>
#include <dlfcn.h>
#include <execinfo.h>
#include <mutex>

static std::mutex tracker_mutex;
static void* (*real_malloc)(size_t) = nullptr;
static void (*real_free)(void*) = nullptr;
static bool initialization_phase = true; // Start in initialization phase

// Function to call when initialization is complete
extern "C" void end_initialization_phase() {
    std::lock_guard<std::mutex> lock(tracker_mutex);
    std::cerr << "[Memory Tracker] Initialization phase complete. Now tracking runtime allocations." << std::endl;
    initialization_phase = false;
}

// Load original memory functions before main() runs
__attribute__((constructor)) void initialize() {
    real_malloc = (void* (*)(size_t)) dlsym(RTLD_NEXT, "malloc");
    real_free = (void (*)(void*)) dlsym(RTLD_NEXT, "free");

    if (!real_malloc || !real_free) {
        std::cerr << "Error: dlsym() failed to load memory functions!" << std::endl;
        std::_Exit(EXIT_FAILURE);
    }
}

// Stack trace function
void print_stack_trace() {
    void *buffer[10];
    int frames = backtrace(buffer, 10);
    char **symbols = backtrace_symbols(buffer, frames);

    if (symbols) {
        std::cerr << "  Stack trace:" << std::endl;
        for (int i = 1; i < frames; i++) {  // Skip this function itself
            std::cerr << "    [" << i << "] " << symbols[i] << std::endl;
        }
        free(symbols);
    }
}

// Hooked new
void* operator new(size_t size) {
    if (!real_malloc) initialize();
    void* ptr = real_malloc(size);

    if (!initialization_phase) {
        std::lock_guard<std::mutex> lock(tracker_mutex);
        std::cerr << "[RUNTIME ALLOCATION WARNING] new(" << size << ") -> " << ptr << std::endl;
        print_stack_trace();
    }

    return ptr;
}

// Hooked new[]
void* operator new[](size_t size) {
    if (!real_malloc) initialize();
    void* ptr = real_malloc(size);

    if (!initialization_phase) {
        std::lock_guard<std::mutex> lock(tracker_mutex);
        std::cerr << "[RUNTIME ALLOCATION WARNING] new[](" << size << ") -> " << ptr << std::endl;
        print_stack_trace();
    }

    return ptr;
}

// Hooked delete
void operator delete(void* ptr) noexcept {
    if (!real_free) initialize();

    if (!initialization_phase) {
        std::lock_guard<std::mutex> lock(tracker_mutex);
        std::cerr << "[RUNTIME DEALLOCATION] delete(" << ptr << ")" << std::endl;
        print_stack_trace();
    }

    real_free(ptr);
}

// Hooked delete[]
void operator delete[](void* ptr) noexcept {
    if (!real_free) initialize();

    if (!initialization_phase) {
        std::lock_guard<std::mutex> lock(tracker_mutex);
        std::cerr << "[RUNTIME DEALLOCATION] delete[](" << ptr << ")" << std::endl;
        print_stack_trace();
    }

    real_free(ptr);
}

//g++ -shared -fPIC memory_tracker.cpp -o libmemorytracker.so -ldl -rdynamic
//g++ test_program.cpp -o test_program -rdynamic -L. -lmemorytracker
//export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
//LD_PRELOAD=./libmemorytracker.so ./test_program

//