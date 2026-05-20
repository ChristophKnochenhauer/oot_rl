#include <iostream>
#include <soh_lib.h>

int main(int argc, char** argv) {
    std::cout << "=== SoH Library Test ===\n";
    std::cout << "Initializing...\n";
    SoH_Init(argc, argv);
    
    std::cout << "Init complete. Running 5 frames...\n";
    for (int i = 0; i < 5; i++) {
        std::cout << "  Frame " << i << " requested...\n" << std::flush;
        SoH_StepFrame();
        std::cout << "  Frame " << i << " done\n";
    }
    
    std::cout << "Shutting down...\n";
    SoH_Shutdown();
    
    std::cout << "=== Done ===\n";
    return 0;
}
