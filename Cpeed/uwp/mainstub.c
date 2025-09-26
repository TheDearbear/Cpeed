#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "main.h"

// This is required to keep parent project C-only
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    return real_main();
}
