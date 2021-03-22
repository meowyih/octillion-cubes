// OctillionD2D1.cpp : 定義應用程式的進入點。
//

#include <windowsx.h>

#include "framework.h"
#include "OctillionD2D1.h"
#include "Kernel.hpp"
#include "error/macrolog.hpp"

#define MAX_LOADSTRING 100

Kernel g_kernel;

// 全域變數:
HINSTANCE hInst;                                // 目前執行個體
WCHAR szTitle[MAX_LOADSTRING];                  // 標題列文字
WCHAR szWindowClass[MAX_LOADSTRING];            // 主視窗類別名稱

// 這個程式碼模組所包含之函式的向前宣告:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

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
    LoadStringW(hInstance, IDC_OCTILLIOND2D1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 執行應用程式初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_OCTILLIOND2D1));

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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_OCTILLIOND2D1));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
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

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   // note: don't put init here, do it in WM_CREATE
   // init kernel
   // g_kernel.init(hWnd);

   // update timer
   // SetTimer(hWnd, 100, 100, NULL);

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
    switch (message)
    {
    case WM_CREATE:
    {
        // init direct2d resource
        if (g_kernel.init_d2d() == false)
        {
            MessageBox(NULL, L"Failed to initialize direct2d.", NULL, MB_OK);
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            return false;
        }

        // trigger the first timer callback
        g_kernel.timer_callback();

        // update timer
        SetTimer(hWnd, 100, 100, NULL);
        break;
    }
    case WM_CAPTURECHANGED:
    {
        // when losing mouse, it could cause WM_LBUTTONDOWN without WM_LBUTTONUP. 
        break;
    }
    case WM_LBUTTONUP:
    {
        break;
    }
    case WM_LBUTTONDOWN:
    {
        POINTS pt;
        pt = MAKEPOINTS(lParam);
        LOG_D("OctillionD2D1") << "WM_POINTERDOWN" << " " << pt.x << "," << pt.y;
        break;
    }
    case WM_MOUSEMOVE:
    {
        POINTS pt;
        pt = MAKEPOINTS(lParam);
        // LOG_D("OctillionD2D1") << "WM_MOUSEMOVE" << " " << pt.x << "," << pt.y;
        break;
    }
    case WM_SIZE:
    {
        g_kernel.init_screen(hWnd);
#if 0
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        switch (wParam)
        {
        case SIZE_RESTORED:
            LOG_D("OctillionD2D1") << "SIZE_RESTORED" << " " << width << "," << height;
            g_kernel.resize(width, height);
            break;
        case SIZE_MINIMIZED:
            LOG_D("OctillionD2D1") << "SIZE_MINIMIZED" << " " << width << "," << height;
            break;
        case SIZE_MAXSHOW:
            LOG_D("OctillionD2D1") << "SIZE_MAXSHOW" << " " << width << "," << height;
            g_kernel.resize(width, height);
            break;
        case SIZE_MAXIMIZED:
            LOG_D("OctillionD2D1") << "SIZE_MAXIMIZED" << " " << width << "," << height;
            g_kernel.resize(width, height);
            break;
        case SIZE_MAXHIDE:
            LOG_D("OctillionD2D1") << "SIZE_MAXHIDE" << " " << width << "," << height;
            break;
        }
#endif
    }
    break;
    case WM_KEYDOWN:
    {
        UINT keycode;
        if (wParam == VK_PROCESSKEY)
        {
            keycode = ImmGetVirtualKey(hWnd); // require Imm32.lib
        }
        else
        {
            keycode = wParam;
        }
        g_kernel.keypress(keycode);
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case WM_TIMER:
    {
        if (wParam == 100)
        {
            g_kernel.timer_callback(); // notify game kernel to calculate the new state
            InvalidateRect(hWnd, NULL, FALSE); // ready for draw
        }
        break;
    }
    case WM_PAINT:
    {
        {            
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此新增任何使用 hdc 的繪圖程式碼...
            // g_kernel.update();
            g_kernel.paint();
            EndPaint(hWnd, &ps);
        }
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    
    return 0;
}