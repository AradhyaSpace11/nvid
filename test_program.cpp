#include <iostream>
#include <vector>

// Declaration of the function from the tracker
extern "C" void end_initialization_phase();

int main() {
    std::cout << "===== INITIALIZATION PHASE =====" << std::endl;
    
    // These allocations happen during initialization, so they won't trigger warnings
    std::cout << "Allocating initialization memory..." << std::endl;
    int *init_arr = new int[10];
    std::vector<int> init_vec(5, 42);
    
    // Mark the end of initialization
    end_initialization_phase();
    
    std::cout << "\n===== RUNTIME PHASE =====" << std::endl;
    
    // These allocations happen during runtime, so they will be logged as warnings
    std::cout << "Allocating runtime memory (will trigger warnings)..." << std::endl;
    int *runtime_arr = new int[20];
    std::vector<double> runtime_vec(10, 3.14);
    
    // Cleanup
    std::cout << "\nCleaning up memory..." << std::endl;
    delete[] init_arr;
    delete[] runtime_arr;
    
    std::cout << "Program complete." << std::endl;
    return 0;
}