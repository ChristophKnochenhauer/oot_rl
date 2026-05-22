#include <iostream>
#include <soh_lib.h>

extern "C" int Ship_Headless_IsEnabled(void);

int main(int argc, char** argv) {
    std::cout << "Before SetHeadless: " << Ship_Headless_IsEnabled() << "\n";
    SoH_SetHeadless(1);
    std::cout << "After SetHeadless: " << Ship_Headless_IsEnabled() << "\n";
    
    SoH_Init(argc, argv);
    for (int i = 0; i < 5; i++) {
        SoH_StepFrame();
        std::cout << "Frame " << i << " done\n";
    }
    SoH_Shutdown();
    return 0;
}
