// dual_orbit_icons.cpp  (Version B - Grid-based)
// This version works even when LVM_GETITEMPOSITION always returns 0,0.

#include <windows.h>
#include <CommCtrl.h>
#include <vector>
#include <cmath>
#include <iostream>

#pragma comment(lib, "Comctl32.lib")

// ----------- Find Desktop ListView -----------
HWND GetDesktopListView() {
    HWND hShellViewWin = NULL;

    HWND hProgman = FindWindowA("Progman", NULL);
    if (hProgman) {
        hShellViewWin = FindWindowExA(hProgman, NULL, "SHELLDLL_DefView", NULL);
    }

    if (!hShellViewWin) {
        HWND hWorkerW = NULL;
        while ((hWorkerW = FindWindowExA(NULL, hWorkerW, "WorkerW", NULL)) != NULL) {
            hShellViewWin = FindWindowExA(hWorkerW, NULL, "SHELLDLL_DefView", NULL);
            if (hShellViewWin)
                break;
        }
    }

    if (!hShellViewWin) return NULL;

    return FindWindowExA(hShellViewWin, NULL, "SysListView32", NULL);
}

// ------- Build a synthetic grid since real positions are 0,0 -------
std::vector<POINT> BuildFallbackGrid(HWND hListView, int count) {
    RECT rc;
    std::vector<POINT> pts;

    if (!GetClientRect(hListView, &rc) || count <= 0)
        return pts;

    const int ICON_W = 100;  
    const int ICON_H = 100;

    int width  = (int)(rc.right - rc.left);
    int height = (int)(rc.bottom - rc.top);

    int cols = std::max(1, width / ICON_W);
    int rows = (count + cols - 1) / cols;

    int totalW = cols * ICON_W;
    int totalH = rows * ICON_H;

    int originX = (width  - totalW) / 2;
    int originY = (height - totalH) / 2;

    for (int r = 0; r < rows && (int)pts.size() < count; r++) {
        for (int c = 0; c < cols && (int)pts.size() < count; c++) {
            POINT p;
            p.x = originX + c * ICON_W + ICON_W / 2;
            p.y = originY + r * ICON_H + ICON_H / 2;
            pts.push_back(p);
        }
    }

    return pts;
}

int main() {
    HWND hListView = GetDesktopListView();
    if (!hListView) {
        std::cout << "Failed to locate the desktop ListView.\n";
        return 1;
    }

    int iconCount = SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0);
    if (iconCount <= 0) {
        std::cout << "No icons found.\n";
        return 0;
    }

    std::cout << "Found " << iconCount << " icons.\n";
    std::cout << "System returns 0,0 — using fallback grid.\n";

    // Build synthetic grid positions
    std::vector<POINT> positions = BuildFallbackGrid(hListView, iconCount);
    if ((int)positions.size() != iconCount) {
        std::cout << "Could not generate fallback grid positions.\n";
        return 1;
    }

    // Split icons evenly: left half, right half
    std::vector<int> leftIcons;
    std::vector<int> rightIcons;

    for (int i = 0; i < iconCount; i++) {
        if (i < iconCount / 2) leftIcons.push_back(i);
        else rightIcons.push_back(i);
    }

    int screenWidth  = GetSystemMetrics(SM_CXSCREEN) + 300;
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    double leftCenterX  = screenWidth * 0.25;
    double rightCenterX = screenWidth * 0.75;
    double centerY      = screenHeight * 0.5;

    double radius = std::min(screenWidth, screenHeight) * 0.22;

    double angleLeft  = 0.0;
    double angleRight = 0.0;

    std::cout << "Animation running. Press CTRL+C to stop.\n";

    while (true) {
        angleLeft  += 0.02;  // speed (left circle)
        angleRight -= 0.02;  // speed (right circle, opposite)

        // Left circle
        for (int i = 0; i < leftIcons.size(); i++) {
            double angle = angleLeft + (2 * M_PI * i / leftIcons.size());

            int x = (int)(leftCenterX  + radius * cos(angle));
            int y = (int)(centerY      + radius * sin(angle));

            SendMessage(hListView, LVM_SETITEMPOSITION, leftIcons[i], MAKELPARAM(x, y));
        }

        // Right circle
        for (int i = 0; i < rightIcons.size(); i++) {
            double angle = angleRight + (2 * M_PI * i / rightIcons.size());

            int x = (int)(rightCenterX + radius * cos(angle));
            int y = (int)(centerY      + radius * sin(angle));

            SendMessage(hListView, LVM_SETITEMPOSITION, rightIcons[i], MAKELPARAM(x, y));
        }

        Sleep(16); // ~60 FPS
    }

    return 0;
}
