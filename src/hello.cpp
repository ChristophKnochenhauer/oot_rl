#include <iostream>

extern "C" {
    extern int gScreenWidth;
    void Main_LogSystemHeap(void);
}

int main() {
    std::cout << "Hello, World!\n";

    std::cout << "SoH global gScreenWidth lives at: "
              << reinterpret_cast<void*>(&gScreenWidth) << "\n";

    auto fn_ptr = reinterpret_cast<void*>(&Main_LogSystemHeap);
    std::cout << "Main_LogSystemHeap lives at: " << fn_ptr << "\n";

    return 0;
}
