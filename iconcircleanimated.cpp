#include <windows.h>
#include <CommCtrl.h>
#include <cmath>
#include <iostream>

#pragma comment(lib, "Comctl32.lib")

HWND GetDesktopListView() {
    HWND hProgman = FindWindow("Progman", NULL);
    HWND hDefView = NULL;

    // Look for SHELLDLL_DefView inside WorkerW
    HWND hWorkerW = NULL;
    do {
        hWorkerW = FindWindowEx(NULL, hWorkerW, "WorkerW", NULL);
        if (hWorkerW) {
            hDefView = FindWindowEx(hWorkerW, NULL, "SHELLDLL_DefView", NULL);
        }
    } while (hWorkerW && !hDefView);

    // Try Progman as backup
    if (!hDefView) {
        hDefView = FindWindowEx(hProgman, NULL, "SHELLDLL_DefView", NULL);
    }

    if (!hDefView) return NULL;

    return FindWindowEx(hDefView, NULL, "SysListView32", NULL);
}

int main() {
    HWND hListView = GetDesktopListView();
    if (!hListView) {
        std::cout << "Could not locate desktop ListView!" << std::endl;
        return 1;
    }

    int iconCount = (int)SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0);

    if (iconCount <= 0) {
        std::cout << "No icons found." << std::endl;
        return 0;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    double centerX = (screenWidth / 2.0) + 150;
    double centerY = screenHeight / 2.0;

    double radius = std::min(screenWidth, screenHeight) / 2.0;

    double angleOffset = 0.0;   // this will gradually increase, causing rotation

    std::cout << "start\n";

    while (true) {
        angleOffset += 0.02; // how fast the circle rotates

        

        for (int i = 0; i < iconCount; i++) {
            double baseAngle = (2 * M_PI / iconCount) * i;
            double finalAngle = baseAngle + angleOffset;

            int x = (int)(centerX + radius * cos(finalAngle));
            int y = (int)(centerY + radius * sin(finalAngle));

            SendMessage(hListView, LVM_SETITEMPOSITION, i, MAKELPARAM(x, y));
        }

        Sleep(0);  // ~60 FPS animation

        if (GetAsyncKeyState(VK_ESCAPE)) {
            break;
        }
    }

    return 0;
}
