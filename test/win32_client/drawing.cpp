// drawing.cpp : 定義應用程式的進入點。
//

#include <windows.h>
#include <windowsx.h> // GET_X_LPARAM / GET_Y_LPARAM
#include "framework.h"
#include "drawing.h"
#include "DrawClass.hpp"
#include "world/worldmap.hpp"
#include "error/macrolog.hpp"

#define MAX_LOADSTRING 100

// 全域變數:
HINSTANCE hInst;                                // 目前執行個體
WCHAR szTitle[MAX_LOADSTRING];                  // 標題列文字
WCHAR szWindowClass[MAX_LOADSTRING];            // 主視窗類別名稱

// 這個程式碼模組所包含之函式的向前宣告:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// drawer
DrawClass g_drawer;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此放置程式碼。

    // 將全域字串初始化
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DRAWING, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 執行應用程式初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DRAWING));

    MSG msg;

    // 主訊息迴圈:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函式: MyRegisterClass()
//
//  用途: 註冊視窗類別。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DRAWING));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

HWND CreateFullscreenWindow(HWND hwnd)
{
    HMONITOR hmon = MonitorFromWindow(hwnd,
        MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    if (!GetMonitorInfo(hmon, &mi)) return NULL;
    return CreateWindowW(szWindowClass,
        szTitle,
        WS_POPUP | WS_VISIBLE,
        mi.rcMonitor.left + 200,
        mi.rcMonitor.top + 200,
        mi.rcMonitor.right - mi.rcMonitor.left - 400,
        mi.rcMonitor.bottom - mi.rcMonitor.top - 400,
        nullptr, nullptr, hInst, nullptr);
}

//
//   函式: InitInstance(HINSTANCE, int)
//
//   用途: 儲存執行個體控制代碼並且建立主視窗
//
//   註解:
//
//        在這個函式中，我們將執行個體控制代碼儲存在全域變數中，
//        並建立及顯示主程式視窗。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 將執行個體控制代碼儲存在全域變數中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_BORDER | WS_MAXIMIZE,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // create child windows without titlebar
   HWND hWndChild = CreateFullscreenWindow(hWnd);

   int iDpi = GetDpiForWindow(hWndChild);

   LOG_D("drawing") << "Dpi is: " << iDpi;

   // show child window instead of main window
   ShowWindow(hWndChild, nCmdShow);
   UpdateWindow(hWndChild);
   
   // update timer
   SetTimer(hWndChild, 100, 100, NULL);

   return TRUE;
}

//
//  函式: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  用途: 處理主視窗的訊息。
//
//  WM_COMMAND  - 處理應用程式功能表
//  WM_PAINT    - 繪製主視窗
//  WM_DESTROY  - 張貼結束訊息然後傳回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UINT key_code = 0;
    int xPos, yPos;
    switch (message)
    {
    case WM_CREATE:
    {
        int iDpi = GetDpiForWindow(hWnd);
        LOG_D("drawing") << "WM_CREATE, dpi:" << iDpi;
    }
    break;
    case WM_DPICHANGED:
    {
        int iDpi = GetDpiForWindow(hWnd);
        LOG_D("drawing") << "WM_DPICHANGED, dpi:" << iDpi;
    }
    break;
    case WM_LBUTTONDOWN:
    {
        RECT rect;
        GetWindowRect(hWnd, &rect);
        xPos = GET_X_LPARAM(lParam);
        yPos = GET_Y_LPARAM(lParam);
        LOG_D("drawing") << "WM_LBUTTONDOWN: " << xPos << "," << yPos;
        g_drawer.point(xPos, (rect.bottom - rect.top) - yPos);
    }
    break;
    case WM_POINTERDOWN:
    {
        RECT rect;
        GetWindowRect(hWnd, &rect);
        xPos = GET_X_LPARAM(lParam);
        yPos = GET_Y_LPARAM(lParam);
        LOG_D("drawing") << "WM_POINTERDOWN: " << xPos << "," << yPos << " rect(top/left):" << rect.top << "," << rect.left
            << " rel:" << (xPos - rect.top) << "," << (yPos - rect.left);
        g_drawer.point((xPos - rect.top), (rect.bottom - rect.top) - (yPos - rect.left));
    }
    break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 剖析功能表選取項目:
            switch (wmId)
            {
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT rect;
            BOOL recalc = GetUpdateRect(hWnd, &rect, FALSE);
            HDC hdc = BeginPaint(hWnd, &ps);
            g_drawer.OnPaint(hWnd, hdc, recalc); // paint the window
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_TIMER:
        if (wParam == 100)
        {
            InvalidateRect(hWnd, NULL, FALSE); // update whole window every 100ms
        }
        break;

    case WM_KEYDOWN:        
        if (wParam == VK_PROCESSKEY)
        {
            key_code = ImmGetVirtualKey(hWnd);
        }
        else 
        {
            key_code = wParam;
        }

        if (key_code == VK_ESCAPE)
        {
            ::SendMessage(hWnd, WM_CLOSE, NULL, NULL);
        }
        else if (key_code == 0x30) // 0
        {
            g_drawer.degree_x_ = g_drawer.degree_y_ = g_drawer.degree_z_ = 0;
        }
        else if (key_code == 0x58) // x
        {
            g_drawer.degree_x_ -= 10;
        } 
        else if (key_code == 0x59) // y
        {
            g_drawer.degree_y_ += 10;
        }
        else if (key_code == 0x5A) // z
        {
            g_drawer.degree_z_ += 10;
        }
        else if (key_code == 0x30) // 1
        {
            g_drawer.scale_ = 1;
        }
        else if (key_code == 0x31) // 2
        {
            g_drawer.scale_ = 2;
        }
        else if (key_code == 0x32) // 2
        {
            g_drawer.scale_ = 3;
        }
        else if (key_code == 0x33) // 2
        {
            g_drawer.scale_ = 4;
        }
        else if (key_code == 0x34) // 2
        {
            g_drawer.scale_ = 5;
        }
        else if (key_code == 0x35) // 2
        {
            g_drawer.scale_ = 6;
        }
        else if (key_code == 0x39) // 2
        {
            g_drawer.scale_ = 10;
        }
        else
        {
            BOOL isConsumed = g_drawer.keypress(key_code);

            if (isConsumed == FALSE)
            {
                LOG_D("main") << "unhandled keycode: " << key_code;
            }
        }

        LOG_D("main") << "degree:" << g_drawer.degree_x_ << "," << g_drawer.degree_y_ << "," << g_drawer.degree_z_ << " scale:" << g_drawer.scale_;

        break;
    
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
