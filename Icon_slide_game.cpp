#include <windows.h>
#include <CommCtrl.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "Comctl32.lib")

// Robust function to get the correct Desktop ListView handle on Win 10/11
HWND GetDesktopListView() {
    HWND hProgman = FindWindow("Progman", NULL);
    HWND hDefView = NULL;
    
    // Attempt to spawn the worker window if it's not there yet
    SendMessage(hProgman, 0x052C, 0, 0); 

    HWND hWorkerW = NULL;
    do {
        hWorkerW = FindWindowEx(NULL, hWorkerW, "WorkerW", NULL);
        if (hWorkerW) {
            hDefView = FindWindowEx(hWorkerW, NULL, "SHELLDLL_DefView", NULL);
        }
    } while (hWorkerW && !hDefView);

    if (!hDefView) {
        hDefView = FindWindowEx(hProgman, NULL, "SHELLDLL_DefView", NULL);
    }

    return FindWindowEx(hDefView, NULL, "SysListView32", NULL);
}

// Function to perform HitTest across process boundaries
int GetIconIndexFromCursor(HWND hListView) {
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(hListView, &pt);

    // Get the process ID of the desktop (Explorer.exe)
    DWORD pid;
    GetWindowThreadProcessId(hListView, &pid);

    // Open the desktop process with permission to read/write memory
    HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, pid);
    if (!hProcess) return -1;

    // Allocate memory INSIDE the desktop process
    LVHITTESTINFO* pRemoteHitInfo = (LVHITTESTINFO*)VirtualAllocEx(hProcess, NULL, sizeof(LVHITTESTINFO), MEM_COMMIT, PAGE_READWRITE);
    
    // Prepare local structure
    LVHITTESTINFO localHitInfo = { 0 };
    localHitInfo.pt = pt;

    // Copy local structure TO the desktop process
    WriteProcessMemory(hProcess, pRemoteHitInfo, &localHitInfo, sizeof(LVHITTESTINFO), NULL);

    // Send the message telling Explorer to read the memory WE just allocated inside it
    int index = (int)SendMessage(hListView, LVM_HITTEST, 0, (LPARAM)pRemoteHitInfo);

    // Clean up
    VirtualFreeEx(hProcess, pRemoteHitInfo, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return index;
}

int main() {
    // 1. Locate Desktop
    HWND hListView = GetDesktopListView();
    if (!hListView) {
        std::cout << "Could not locate desktop ListView! (Try running as Admin)\n";
        return 1;
    }

    int iconCount = (int)SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0);
    std::cout << "Found " << iconCount << " icons.\n";
    std::cout << "1. Ensure 'Auto arrange icons' is OFF.\n";
    std::cout << "2. Ensure this is compiled in x64 mode.\n";
    std::cout << "3. Click on any desktop icon to select the player...\n";

    int playerIndex = -1;
    bool wasDown = false;

    // 2. Wait for selection
    while (playerIndex == -1) {
        SHORT state = GetAsyncKeyState(VK_LBUTTON);
        if ((state & 0x8000) && !wasDown) {
            wasDown = true;
            // Use the cross-process helper function
            int index = GetIconIndexFromCursor(hListView);
            if (index != -1) {
                playerIndex = index;
            }
        }
        else if (!(state & 0x8000)) {
            wasDown = false;
        }
        Sleep(20);
    }

    std::cout << "Player selected: index " << playerIndex << "\n";

    // 3. Move Icons
    RECT rc;
    GetClientRect(hListView, &rc);

    // For LVM_SETITEMPOSITION, simple SendMessage works because 
    // it passes values (coordinates) directly, not pointers.
    int leftX = 50;
    int rightX = rc.right - 200;
    int y = 50;

    for (int i = 0; i < iconCount; i++) {
        if (i == playerIndex) {
            SendMessage(hListView, LVM_SETITEMPOSITION, i, MAKELPARAM(rightX, rc.bottom / 2));
        } else {
            SendMessage(hListView, LVM_SETITEMPOSITION, i, MAKELPARAM(leftX, y));
            y += 100; // Increased spacing for Win 11 icons
            if (y > rc.bottom - 100) {
                y = 50; 
                leftX += 100; // Create a second column if needed
            }
        }
    }

    std::cout << "Icons moved. Press Enter to exit...";
    std::cin.get(); 

    // Optional: Refresh desktop to ensure visual update
    InvalidateRect(hListView, NULL, TRUE);

    return 0;
}