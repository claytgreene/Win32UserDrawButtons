// WindowsRoundFlatButtons.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Win32UserDrawButtons.h"
#include "winuser.h"
#include "commctrl.h"
#include "strsafe.h"
#include "Windowsx.h"
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <iostream>

using namespace std;

constexpr auto MAX_LOADSTRING = 100;

// Define classes here
class MouseTrackEvents
{
    bool m_bMouseTracking;

public:
    MouseTrackEvents() : m_bMouseTracking(false)
    {
    }

    void OnMouseMove(HWND hwnd)
    {
        if (!m_bMouseTracking)
        {
            // Enable mouse tracking.
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(tme);
            tme.hwndTrack = hwnd;
            tme.dwFlags = TME_HOVER | TME_LEAVE;
            tme.dwHoverTime = HOVER_DEFAULT;
            TrackMouseEvent(&tme);
            m_bMouseTracking = true;
        }
    }
    void Reset(HWND hwnd)
    {
        m_bMouseTracking = false;
    }
};


// Define enums here

enum FocusState { FS_NULL, FS_FOCUSED, FS_NOTFOCUSED };          // enum used by DrawControl


// Define constants here

// Color constants.
const COLORREF rgbRed = 0x000000FF;
const COLORREF rgbGreen = 0x0000FF00;
const COLORREF rgbBlue = 0x00FF0000;
const COLORREF rgbBlack = 0x00000000;
const COLORREF rgbWhite = 0x00FFFFFF;


// Define Global Variables here

HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hwndButton3;
MouseTrackEvents mouseTrack;
TCHAR szBuff[200] = { 0 };                      // String buffer  
size_t bufferSize = ARRAYSIZE(szBuff);          // Size of szBuff 
TCHAR szDebugBuff[1028] = { 0 };                      // String buffer  
size_t debugbufferSize = ARRAYSIZE(szDebugBuff);          // Size of szBuff
HWND hWnd;                                      // handle to the main app window.  Caution as to scope is needed since hWnd/hwnd are also declared as local variables 
HWND hwndDebugDetailsDialog;                    // handle to the Debug Details Dialog
WNDPROC oldWndProc;
int controlidMainWindow;                        // Stores the controlID that gets assigned to the main app window.  This will change every time the app is run

//Font Globals
static HFONT hfontButtons = NULL;
HDC hdcButtonFont;
LOGFONT logFontButtons;

//Below 2 variables work together
bool bWinMsgDebugEnabled = true;                                           //If true all Windows Msgs are displayed in the debug output window
bool bShowMsgsFromUnknownControls = true;                                   //Only used if bWinMsgDebugEnable = trure.  If bShowMsgsFromUnknownControls is also true then all Windows Msgs are displayed even for Unknown Controls.

int controlUnderMouse = 0;              //Used for subclassed ownerdraw controls for highlighting when the mouse if over the control.

//Need these for bitmaps loaded in the WM_CREATE event.  Want to keep these bitmaps loaded in memory as they are used frequently.
HBITMAP hbitAeroNormal;
HBITMAP hbitAeroHover;
HBITMAP hbitAeroPressed;
HBITMAP hbitAeroHoverNoText;
HBITMAP hbitVMwareSmall;
HBITMAP hbitAeroNormalNoText;           //Used IDB_AERO_NORMMAL_NOTEXT
HBITMAP hbitAeroPressedNoText;           //Used IDB_AERO_PRESSED_NOTEXT

BITMAP          bitmap01;
HDC             hdcMem01;
HGDIOBJ         oldBitmap01;


//Define STRUCT objects here

struct ControlProp                     //This struct is used by the 'DrawControl' function.  Most properties are only used when a passed parameter to DrawControl is NULL
{
    int controlID = -1;                 //Might seem redundant when used in the buttonProps map (below) but if you copy the map entry into another variable you might need this.
    std::string controlName = "";       //Name of the control.  Not needed much for coding but can be convenient to put a name here in case the ControlID is not a friendly name
    HBITMAP hBitmap = hbitAeroNormalNoText;         //If 'DrawControl' is called without a bitmap param then this bitmap will be used.  NOTE:  This bimap must already by loaded into a HBITMAP.
    FocusState hasFocus = FS_NULL;              //Set to FS_FOCUSED if the control has focus and the DrawControl function will draw the focus rectangle on the control.  Set to FS_NOTFOCUSED and DrawControl will not draw a focus rectable.  If passed param set to FS_NULL DrawControl will check buttonProps for the last value and use it.
    int fontHeight = 12;                //If 'DrawControl' is called without a FontSize param then this value is used
    int fontWeight = FW_NORMAL;         //Can be set to FW_NORMAL or FW_BOLD
    int  drawTextStyle = DT_SINGLELINE | DT_CENTER | DT_VCENTER;      //If 'DrawControl' is called without FontStyle bits being passed a a param then this value is used
    COLORREF   textColor = 0x00000000;      //If 'DrawControl' is called without TextColor being passed as a param then this value is used
};                 


//Define MAP objects here

std::map <int, ControlProp> buttonProps;        //the int will be a window's controlID


// Define Forward declarations of functions included in this code module:

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);

//Below are the WinProc message handlers for the main app window and all dialogs
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    DebugDetailsProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    NewBtnProc(HWND, UINT, WPARAM, LPARAM);

std::wstring GetControlNameGivenID(int controlId);
std::wstring GetWindowsMsgNameGivenMsg(int windowsMsg);
std::wstring GetTimeDate();
std::wstring GetDRAWITEMSTRUCTasString(LPDRAWITEMSTRUCT dis);
std::wstring GetControlTypeFromDRAWITEMSTRUCT(LPDRAWITEMSTRUCT dis);
std::wstring GetItemActionFromDRAWITEMSTRUCT(LPDRAWITEMSTRUCT dis);
std::wstring GetItemStateFromDRAWITEMSTRUCT(LPDRAWITEMSTRUCT dis);
std::wstring ConvertHWDNtoString(HWND hwnd);
std::wstring ConvertHDCtoString(HDC hdc);
void StretchImage(HDC hdcDest, HDC hdcSrc, int destX, int destY, int destWidth, int destHeight, int srcX, int srcY, int srcWidth, int srcHeight);
void DrawControl(int controlID, HBITMAP hbitPreLoadedBitmap = NULL, FocusState hasFocus = FS_NULL, int fontWeight = -1, int fontHeight = -1, COLORREF textColor = -1, int drawTextStyle = -1, LPCWSTR buttonText = NULL);
map<int, wstring> SplitStringW(std::wstring strToSplit);
std::wstring trim(const std::wstring& str);
void expandDebugMsg(map<int, wstring>mapDebugMsg);
wstring GetSubMessage(UINT msg, WPARAM wparam, LPARAM lparam);
std::wstring PadString(std::wstring srcString, char padChar, int numChars);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINDOWSROUNDFLATBUTTONS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSROUNDFLATBUTTONS));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            //Finally fixed the tab key focus issue.  Need to change 'hwnd' in the line below to 'hWnd' (the handle to the main window for the app).
            if (!IsDialogMessage(hWnd, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

        }
    }
    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSROUNDFLATBUTTONS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WINDOWSROUNDFLATBUTTONS);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   else
   {
       controlidMainWindow = GetDlgCtrlID(hWnd);            //This gets the controlID assigned to the main app window.  Note:  this controlID is just so that we can identify messages to the main window.  Don't try to use this value in any win32 function calls that require a controlID.
   }

    // InitCommonControlsEx() is required on Windows XP if an application manifest specifies use of ComCtl32.dll version 6 or later to enable visual styles.  Otherwise, any window creation will fail.
   INITCOMMONCONTROLSEX InitCtrls;
   InitCtrls.dwSize = sizeof(InitCtrls);
   // Set this to include all the common control classes you want to use
   // in your application.
   InitCtrls.dwICC = ICC_STANDARD_CLASSES;
   InitCommonControlsEx(&InitCtrls);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//

//Blow block is from Subclassing MyButton3.  This is where we'll now receive windows messages for this button.
LRESULT CALLBACK NewBtnProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //Get the control name and windows msg name and output them into the debug window
    int controlID = GetDlgCtrlID(hWnd);
    std::wstring msgStr = std::to_wstring(msg);
    std::wstring msgName = GetWindowsMsgNameGivenMsg(msg);
    std::wstring controlName = GetControlNameGivenID(controlID) ;
    std::wstring currDateTime = GetTimeDate();
    std::wstring subMsg = GetSubMessage(msg, wParam, lParam);
    std::wstring wParamStr = std::to_wstring(static_cast<UINT>(wParam));
    std::wstring lParamStr = std::to_wstring(lParam);

    controlName = PadString(controlName, ' ', 15);
    msgStr = PadString(msgStr, ' ', 6);
    msgName = PadString(msgName, ' ', 20);
    wParamStr = PadString(wParamStr, ' ', 10);
    lParamStr = PadString(lParamStr, ' ', 11);

    StringCchPrintf(szBuff, bufferSize, TEXT("%s | %p | %s | %s | %s | %s | %s | %s | %s \n"), currDateTime.c_str(), hWnd, controlName.c_str(), msgStr.c_str(), msgName.c_str(), wParamStr.c_str(), lParamStr.c_str(), L"NewBtnProc", subMsg.c_str());
    if (bWinMsgDebugEnabled == true)
    {
        if (bShowMsgsFromUnknownControls == false && (controlName.substr(0, 15) != L"Unknown Control"))        //NOTE:  Change the operator to == if you want to see msgs from Unknown Controls
        {
            //do nothing
        }
        else
        {
            //Display all windows messages even ones for Unknown Controls
            OutputDebugString(szBuff);
        }
    }
   
    switch (msg)
    {
        case WM_COMMAND:    //Should never get WM_COMMAND or WM_NOFIY messages in a subclassed message pump.  These messages are sent to the parent controls when an event occurs in a child.
            break;
        case WM_NOTIFY:     //Should never get WM_COMMAND or WM_NOFIY messages in a subclassed message pump.  These messages are sent to the parent controls when an event occurs in a child.
            break;
        case WM_SETFOCUS:
        {   
            //HWND hwndControlWithFocus = GetFocus();
            HWND hwndControlWithFocus = hWnd;                           // handle of window getting focu
            HWND hwndControlLostFocus = (HWND)wParam;                   // handle of window losing focus
            controlID = GetDlgCtrlID(hwndControlWithFocus);
            controlName = GetControlNameGivenID(controlID);
            StringCchPrintf(szBuff, bufferSize, TEXT("%s : %s : %s : %s \n"), currDateTime.c_str(), controlName.c_str(), msgName.c_str(), L"NewBtnProc");
            //OutputDebugString(szBuff);

            switch (controlID)
            {
            case IDC_BUTTON1:
            {
                DrawControl(controlID, NULL, FS_FOCUSED);
                return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
            }
            case IDC_BUTTON3:
            {
                //return 0;
                break;
            }
                
            case IDC_BUTTON6:
            {
                DrawControl(controlID, NULL, FS_FOCUSED);
                return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
            }
            default:
                return 0;
            }
        }
        break;
        case WM_KILLFOCUS:
        {
            //HWND hwndControlWithFocus = GetFocus();
            HWND hwndControlLostFocus = hWnd;                           // handle of window losing focus
            HWND hwndControlWithFocus = (HWND)wParam;                   // handle of window getting focu
            controlID = GetDlgCtrlID(hwndControlLostFocus);
            controlName = GetControlNameGivenID(controlID);
            StringCchPrintf(szBuff, bufferSize, TEXT("%s : %s : %s : %s \n"), currDateTime.c_str(), controlName.c_str(), msgName.c_str(), L"NewBtnProc");
            //OutputDebugString(szBuff);

            switch (controlID)
            {
            case IDC_BUTTON1:
            {
                DrawControl(controlID, NULL, FS_NOTFOCUSED);
                return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
            }
            case IDC_BUTTON3:
            {
                //return 0;
                break;
            }
            case IDC_BUTTON6:
            {
                DrawControl(controlID, NULL, FS_NOTFOCUSED);
                return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
            }
            default:
                return 0;
            }
        }
        break;
        case WM_PAINT:          //NOTE: WM_PAINT is sent when app first starts and any time the main app window is resized.
        {
            switch (GetDlgCtrlID(hWnd))
            {
            case IDC_BUTTON1:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                DrawControl(IDC_BUTTON1, hbitAeroNormalNoText);
                DeleteDC(hdcMem01);
                EndPaint(hWnd, &ps);
                ////OutputDebugString(L"WM_PAINT MyButton3\n");
                return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
            }
            case IDC_BUTTON3:
            {
                // DON"T PUT CODE HERE FORT BUTTON3.  Even though it's a sublcassed control it's not OWNERDRAW.  It's BS_BITMAP.  Found if I try to put code here to set the bitmap that it stops displaying properly.  Hover highlighting stops working.  If I resize the main app window the button disappears.
                // Because of the issue described above I set the bitmap for this button in the WM_CREATE code.
                break;
            }
            break;
            case IDC_BUTTON6:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                DrawControl(IDC_BUTTON6, hbitAeroNormalNoText);
                DeleteDC(hdcMem01);
                EndPaint(hWnd, &ps);
                return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
            }
            break;
            default:
                break;
            }
        }
        break;
        case WM_MOUSEMOVE:
        {
            //StringCchPrintf(szBuff, bufferSize, TEXT("%s,x:%04d y:%04d,\n"), L"Mouse Move : ", LOWORD(lParam), HIWORD(lParam));
            //OutputDebugString(szBuff);
            if (controlUnderMouse != GetDlgCtrlID(hWnd))
            {
                //Mouse is now over an ownerdraw subclassed control.  Need to highlight the control

                //First need to Un-highlight the previous control that the mouse was under
                switch (controlUnderMouse)
                {
                case IDC_BUTTON1:
                {
                    DrawControl(IDC_BUTTON1, hbitAeroNormalNoText);
                    return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
                }
                break;
                case IDC_BUTTON6:
                {
                    DrawControl(IDC_BUTTON6, hbitAeroNormalNoText);
                    return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
                }
                break;
                default:
                    break;
                }
                //Now need to highlight the control that the mouse is currently over
                controlUnderMouse = GetDlgCtrlID(hWnd);
                // Below line is used to update the status bar with the control name that the mouse is currently over
                SetWindowText(GetDlgItem(GetParent(hWnd), IDC_STATUSBAR), GetControlNameGivenID(controlUnderMouse).c_str());
                switch (controlUnderMouse)
                {
                case IDC_BUTTON1:
                {
                    DrawControl(IDC_BUTTON1, hbitAeroHoverNoText);
                    return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
                }
                break;
                case IDC_BUTTON6:
                {
                    DrawControl(IDC_BUTTON6, hbitAeroHoverNoText);
                    return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
                }
                break;
                default:
                    break;
                }
            }
            break;
        }
        break;
        case WM_LBUTTONDOWN:
        {
            //StringCchPrintf(szBuff, bufferSize, TEXT("%s : %s : %s : %s \n"), currDateTime.c_str(), controlName.c_str(), msgName.c_str(), L"NewBtnProc");
            //OutputDebugString(szBuff);
            switch (GetDlgCtrlID(hWnd))
            {
            case IDC_BUTTON1:
                {
                    DrawControl(IDC_BUTTON1, hbitAeroPressedNoText);
                    SetFocus(hWnd);
                    return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
                }
                break;
            case IDC_BUTTON3:
                {
                    SendMessage(hwndButton3, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbitAeroPressed);
                    return 0;
                    break;
                }
            case IDC_BUTTON6:
            {
                DrawControl(IDC_BUTTON6, hbitAeroPressedNoText,FS_NULL, FW_BOLD);
                return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
            }
            default:
                break;
            }
        }
        break;
        case WM_LBUTTONUP:
        {
            //StringCchPrintf(szBuff, bufferSize, TEXT("%s : %s : %s : %s \n"), currDateTime.c_str(), controlName.c_str(), msgName.c_str(), L"NewBtnProc");
            //OutputDebugString(szBuff);
            switch (GetDlgCtrlID(hWnd))
            {
            case IDC_BUTTON1:
            {
                if (controlUnderMouse == IDC_BUTTON1)
                {
                    DrawControl(IDC_BUTTON1, hbitAeroHoverNoText);
                }
                else
                {
                    DrawControl(IDC_BUTTON1, NULL);
                }
                return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
            }
            case IDC_BUTTON3:
            {
                SendMessage(hwndButton3, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbitAeroHover);
                return 0;
                break;
            }
            case IDC_BUTTON6:
            {
                if (controlUnderMouse == IDC_BUTTON6)
                {
                    DrawControl(IDC_BUTTON6, hbitAeroHoverNoText, FS_NULL, FW_NORMAL);
                }
                else
                {
                    DrawControl(IDC_BUTTON6, NULL, FS_NULL, FW_NORMAL);
                }
                return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
            }
            default:
                break;
            }
        }
        break;
        case WM_LBUTTONDBLCLK:
        {
            //StringCchPrintf(szBuff, bufferSize, TEXT("%s : %s : %s : %s \n"), currDateTime.c_str(), controlName.c_str(), msgName.c_str(), L"NewBtnProc");
            //OutputDebugString(szBuff);

            //This code is needed in order to make Button's background look double-clicked (not single clicked).  We are making it looked 'Pressed' for the 2nd time.
            switch (GetDlgCtrlID(hWnd))
            {
            case IDC_BUTTON1:
            {
                DrawControl(IDC_BUTTON1, hbitAeroPressedNoText);
                return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
            }
            case IDC_BUTTON3:
            {
                SendMessage(hwndButton3, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbitAeroPressed);
                return 0;
                break;
            }
            break;
            case IDC_BUTTON6:
            {
                DrawControl(IDC_BUTTON6, hbitAeroPressedNoText,FS_NULL, FW_BOLD);
                //OutputDebugString(L"WM_PAINT MyButton3\n");
                return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
            }
            }
        }
        break;
        default:
            break;
    }
    return CallWindowProc(oldWndProc, hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //Get the control name and windows msg name and output them into the debug window
    int controlID = GetDlgCtrlID(hWnd);
    std::wstring msgStr = std::to_wstring(message);
    std::wstring msgName = GetWindowsMsgNameGivenMsg(message);
    std::wstring controlName = GetControlNameGivenID(controlID);
    std::wstring currDateTime = GetTimeDate();
    std::wstring subMsg = GetSubMessage(message, wParam, lParam);
    std::wstring wParamStr = std::to_wstring(static_cast<UINT>(wParam));
    std::wstring lParamStr = std::to_wstring(lParam);

    controlName = PadString(controlName, ' ', 15);
    msgStr = PadString(msgStr, ' ', 6);
    msgName = PadString(msgName, ' ', 20);
    wParamStr = PadString(wParamStr, ' ', 10);
    lParamStr = PadString(lParamStr, ' ', 11);

    StringCchPrintf(szBuff, bufferSize, TEXT("%s | %p | %s | %s | %s | %s | %s | %s | %s \n"), currDateTime.c_str(), hWnd, controlName.c_str(), msgStr.c_str(), msgName.c_str(), wParamStr.c_str(), lParamStr.c_str(), L"WndProc   ", subMsg.c_str());

    if (bWinMsgDebugEnabled == true)
    {
        if (bShowMsgsFromUnknownControls == false && (controlName.substr(0, 15) != L"Unknown Control"))        //NOTE:  Change the operator to == if you want to see msgs from Unknown Controls
        {
            //do nothing
        }
        else
        {
            OutputDebugString(szBuff);
        }
    }

    switch (message)
    {
    case WM_KILLFOCUS:
    {
        HWND hwndControlWithFocus = GetFocus();
        controlID = GetDlgCtrlID(hwndControlWithFocus);
        controlName = GetControlNameGivenID(controlID);
        StringCchPrintf(szBuff, bufferSize, TEXT("%s : %s : %s : %s \n"), currDateTime.c_str(), controlName.c_str(), msgName.c_str(), L"WndProc");
        //OutputDebugString(szBuff);
        break;
    }
    break;
    case WM_SETFOCUS:
    {
        HWND hwndControlWithFocus = GetFocus();
        controlID = GetDlgCtrlID(hwndControlWithFocus);
        controlName = GetControlNameGivenID(controlID);
        StringCchPrintf(szBuff, bufferSize, TEXT("%s : %s : %s : %s \n"), currDateTime.c_str(), controlName.c_str(), msgName.c_str(), L"WndProc");
        //OutputDebugString(szBuff);
        break;
    }
    break;
    case WM_NOTIFY:
    {
        LPNMHDR header = reinterpret_cast<LPNMHDR>(lParam);

        switch (header->code)
        {
        case BCN_HOTITEMCHANGE:
        {
            NMBCHOTITEM* hot_item = reinterpret_cast<NMBCHOTITEM*>(lParam);

            // Handle to the button
            HWND button_handle = header->hwndFrom;

            // ID of the button, if you're using resources
            UINT_PTR button_id = header->idFrom;

            // You can check if the mouse is entering or leaving the hover area
            bool entering = hot_item->dwFlags & HICF_ENTERING;
            std::string myStr = std::to_string(entering);

            controlName = GetControlNameGivenID(button_id);
            msgName = L"WM_NOTIFY/BCN_HOTITEMCHANGE";
            StringCchPrintf(szBuff, bufferSize, TEXT("%s : %s : %s : %s \n"), currDateTime.c_str(), controlName.c_str(), msgName.c_str(), L"WndProc");
            //OutputDebugString(szBuff);

            if (button_id == IDC_BUTTON3)
            {
                if (entering == true)
                {
                    SendMessage(hwndButton3, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbitAeroHover);
                }
                else
                {
                    SendMessage(hwndButton3, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbitAeroNormal);
                }
                //StringCchPrintf(szBuff, bufferSize, TEXT("%s,%s,%s"), L"Button3 HotItem : ", myStr.c_str(), L"\n");
                //OutputDebugString(szBuff);
                return 0;
            }
            break;
        }
        default:
            break;
        }
    }
    break;
    case WM_MOUSEMOVE:
    {
        if (controlUnderMouse != GetDlgCtrlID(hWnd))
        {
            //Mouse is now over main app windows...Mouse is no longer an ownerdraw subclassed control.  Need to UN-highlight the control
                
            switch (controlUnderMouse)
            {
            case IDC_BUTTON1:
            {
                DrawControl(IDC_BUTTON1, hbitAeroNormalNoText);
                //return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
            }
            break;
            case IDC_BUTTON6:
            {
                DrawControl(IDC_BUTTON6, hbitAeroNormalNoText);
                //return 0;    //Found that I wasn't getting the BN_CLICKED msg in the parent windowproc when this was active
            }
            break;
            default:
                break;
            }
            //Update the controlUnderMouse with the new controlID
            controlUnderMouse = GetDlgCtrlID(hWnd);
            SetWindowText(GetDlgItem(hWnd, IDC_STATUSBAR), GetControlNameGivenID(controlUnderMouse).c_str());
        }
        mouseTrack.OnMouseMove(hWnd);  // Start tracking.
        //MessageBox(hWnd, L"Start Tracking", L"Start Tracking", MB_OK);
        //StringCchPrintf(szBuff, bufferSize, TEXT("%s,x:%04d y:%04d,%s"), L"Mouse Move : ", LOWORD(lParam), HIWORD(lParam), L"\n");
        //OutputDebugString(szBuff);
        // TODO: Handle the mouse-move message.
        //POINT pt;
        //int xPos = GET_X_LPARAM(lParam);
        //int yPos = GET_Y_LPARAM(lParam);
        //pt.x = xPos;
        //pt.y = yPos;
        //HWND hwndUnderMouse = WindowFromPoint(pt); // Get the window handle at that position
        //if (hwndButton3 == hwndUnderMouse)
        //{
        //    OutputDebugString(L"Mouse is over MyButton3\n");
        //}
        return 0;
        break;
    }
    break;
    case WM_MOUSELEAVE:
    {
        // TODO: Handle the mouse-leave message.
        //MessageBox(hWnd, L"Mouse Leave", L"Mouse Leave", MB_OK);
        //OutputDebugString(L"Mouse Leave\n");
        mouseTrack.Reset(hWnd);
        return 0;
        break;
    }
    break;
    case WM_MOUSEHOVER:
    {
        // TODO: Handle the mouse-hover message.
        //MessageBox(hWnd, L"Mouse Hover", L"Mouse Hover", MB_OK);
        //OutputDebugString(L"Mouse Hover\n");
        mouseTrack.Reset(hWnd);
        return 0;
        break;
    }
    break;
    case WM_CREATE:
    {
        //Load and set button background to use bitmap
        hbitAeroNormal = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AERO_NORMAL));
        hbitAeroHover = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AERO_HOVER));
        hbitAeroPressed = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AERO_PRESSED));
        hbitAeroHoverNoText = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_AERO_HOVER_NOTEXT));
        hbitVMwareSmall = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_VMWARE_SMALL));
        hbitAeroNormalNoText = (HBITMAP)LoadImage(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDB_AERO_NORMAL_NOTEXT),
            IMAGE_BITMAP,
            NULL,
            NULL,
            LR_DEFAULTCOLOR);
        hbitAeroPressedNoText = (HBITMAP)LoadImage(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDB_AERO_PRESSED_NOTEXT),
            IMAGE_BITMAP,
            NULL,
            NULL,
            LR_DEFAULTCOLOR);

        //Create Font
        const TCHAR* fontName = _T("Segoe UI");
        const long nFontSize = 12;
        hdcButtonFont = GetDC(hWnd);
        logFontButtons = { 0 };
        logFontButtons.lfHeight = -MulDiv(nFontSize, GetDeviceCaps(hdcButtonFont, LOGPIXELSY), 72);
        logFontButtons.lfWeight = FW_NORMAL;
        _tcscpy_s(logFontButtons.lfFaceName, fontName);
        hfontButtons = CreateFontIndirect(&logFontButtons);
        //ReleaseDC(hWnd, hdcButtonFont);     //Don't release the device context.  I've declared it as a global so I can use this font throughout the app.

        //Create Status Bar at bottom of main app window

        HWND hwndStatus = CreateWindowEx(
            0,                       // no extended styles
            STATUSCLASSNAME,         // name of status bar class
            (PCTSTR)NULL,           // no text when first created
            SBARS_SIZEGRIP |         // includes a sizing grip
            WS_CHILD | WS_VISIBLE,   // creates a visible child window
            0, 0, 0, 0,              // ignores size and position
            hWnd,              // handle to parent window
            (HMENU)IDC_STATUSBAR,       // child window identifier
            hInst,                   // handle to application instance
            NULL);                   // no window creation data

        HWND hwndStatusBar = GetDlgItem(hWnd, IDC_STATUSBAR);
        //Don't use SendMessage or SendMessageA to update text in the StatusBar control.  Text comes out looking like Chinese.  Use SendWindowText instead.
        //SendMessageA(hwndStatusBar, SB_SETTEXT, 0, (LPARAM)"Some new text");
        SetWindowText(hwndStatusBar, L"New Staus Bar Text");

        //Create Static Controls

        HWND hwndStatic1 = CreateWindowEx(0, L"STATIC", L"My Static 1",
            WS_VISIBLE | WS_CHILD, 
            600,
            50,
            400,
            400,
            hWnd,
            (HMENU)IDC_STATIC_1,
            NULL,
            NULL);
        //Change the text in the static control
        SetWindowText(hwndStatic1, L"This app demonstrates several techniques.  Subclassing controls.  Ownerdraw controls.  Using and resizing bitmaps for control backgrounds.  Drawing text on top of bitmap backgrounds.  Detecting when to highlight subclassed controls when the mouse is over them.  How to update StatusBar text.  Changing the font used for button text.\n\nButton1: Subclassed BS_OWNERDRAW\nButton2: BS_OWNERDRAW\nButton3: Subclassed BS_BITMAP\nButton4: Standard Button\nButton5: Standard button\nButton6: Subclassed BS_OWNERDRAW Resized Background");

        //Create Buttons

        HWND hwndButton1 = CreateWindowEx(0, L"BUTTON", L"Button1\nSubclassed Ownerdraw",
            WS_CLIPSIBLINGS | WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY | BS_PUSHBUTTON,    //NOTE:  BS_NOTIFY will not send WM_NOTIFY message for BCN_HOTITEMCHANGE if BS_OWNERDRAW is set.
            50, 
            50, 
            203, 
            100, 
            hWnd, 
            (HMENU)IDC_BUTTON1, 
            NULL, 
            NULL);
        //Set the button properties which are used as defaults in the DrawControl function if no params are passed
        ControlProp buttonProp;
        buttonProp.controlName = "Button1";
        buttonProp.controlID = IDC_BUTTON1;
        buttonProp.hBitmap = hbitAeroNormalNoText;
        buttonProp.drawTextStyle = DT_CENTER | DT_CENTER;
        buttonProp.fontHeight = 12;
        buttonProp.textColor = 0x00000000;   //Black
        buttonProps.insert(std::pair<int, ControlProp>(buttonProp.controlID, buttonProp));
        // create a region to shape the button
        //HRGN hRgn = CreateRoundRectRgn(0, 0, 200, 100, 40, 40);
        //SetWindowRgn(hwndButton1, hRgn, TRUE);
        //Change the text font to Bold
        //logFontButtons.lfWeight = FW_BOLD;
        //hfontButtons = CreateFontIndirect(&logFontButtons);
        //Subclass MyButton1.  It will now use 'NewBtnProc' for WM_MOUSEMOVE, WM_LMOUSEUP/DOWN and WM_RMOUSEUP/DOWN and WM_PAINT messages.  Multiple buttons can be set to use 'NewBtnProc'
        logFontButtons.lfWeight = FW_BOLD;
        oldWndProc = (WNDPROC)SetWindowLongPtr(hwndButton1, GWLP_WNDPROC, (LONG_PTR)NewBtnProc);

        HWND hwndButton2 = CreateWindow(L"BUTTON", L"My Button 2",      // Button text 
            WS_CLIPSIBLINGS | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP | BS_NOTIFY | BS_PUSHBUTTON,  //NOTE:  BS_NOTIFY will not send WM_NOTIFY message for BCN_HOTITEMCHANGE if BS_OWNERDRAW is set.
            50,         // x position 
            175,         // y position 
            200,        // Button width
            100,        // Button height
            hWnd,     // Parent window
            HMENU(IDC_BUTTON2),       
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL);      // Pointer not needed.
        //Set the button properties which are used as defaults in the DrawControl function if no params are passed
        buttonProp.controlName = "Button2";
        buttonProp.controlID = IDC_BUTTON2;
        buttonProp.hBitmap = hbitAeroNormalNoText;
        buttonProp.drawTextStyle = DT_SINGLELINE | DT_CENTER | DT_CENTER;
        buttonProp.fontHeight = 12;
        buttonProp.textColor = 0x00000000;   //Black
        buttonProps.insert(std::pair<int, ControlProp>(buttonProp.controlID, buttonProp));
        //Change the text font to Bold
        logFontButtons.lfWeight = FW_BOLD;
        hfontButtons = CreateFontIndirect(&logFontButtons);
        SendMessage(hwndButton2, WM_SETFONT, (WPARAM)hfontButtons, (LPARAM)MAKELONG(TRUE, 0));

        hwndButton3 = CreateWindowEx(NULL,
            L"BUTTON",
            L"My Button 3",
            WS_CHILD | WS_VISIBLE| WS_TABSTOP | BS_NOTIFY | BS_PUSHBUTTON | BS_BITMAP,    //NOTE:  BS_NOTIFY will not send WM_NOTIFY message for BCN_HOTITEMCHANGE if BS_OWNERDRAW is set.
            50,
            300,
            205,
            100,
            hWnd,
            (HMENU)IDC_BUTTON3,
            GetModuleHandle(NULL),
            NULL);
        //Set the button properties which are used as defaults in the DrawControl function if no params are passed
        buttonProp.controlName = "Button3";
        buttonProp.controlID = IDC_BUTTON3;
        buttonProp.hBitmap = hbitAeroNormal;
        buttonProp.drawTextStyle = DT_SINGLELINE | DT_CENTER | DT_CENTER;
        buttonProp.fontHeight = 12;
        buttonProp.textColor = 0x00000000;   //Black
        buttonProps.insert(std::pair<int, ControlProp>(buttonProp.controlID, buttonProp));
        //Load button background to use bitmap
        SendMessage(hwndButton3, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbitAeroNormal);

        //Below line can be used to change the button's text but not when a bitmap is loaded.
        //BOOL bSuccess = SetWindowTextA(hwndButton3, "New Btn Text");

        // Below line subclasses MyButton3 so we can receive mouse messages when mouse is over our button.  There's a new message pump 'NewBtnProc' for this.
        oldWndProc = (WNDPROC)SetWindowLongPtr(hwndButton3, GWLP_WNDPROC, (LONG_PTR)NewBtnProc);


        HWND hwndButton4 = CreateWindowEx(NULL,
            L"BUTTON",
            L"Disable Button 3",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_NOTIFY,     //NOTE:  BS_NOTIFY will not send WM_NOTIFY message for BCN_HOTITEMCHANGE if BS_OWNERDRAW is set.
            50,
            425,
            205,
            102,
            hWnd,
            (HMENU)IDC_BUTTON4,
            GetModuleHandle(NULL),
            NULL);
        //Set the button properties which are used as defaults in the DrawControl function if no params are passed
        buttonProp.controlName = "Button4";
        buttonProp.controlID = IDC_BUTTON4;
        buttonProp.hBitmap = hbitAeroNormalNoText;
        buttonProp.drawTextStyle = DT_SINGLELINE | DT_CENTER | DT_CENTER;
        buttonProp.fontHeight = 12;
        buttonProp.textColor = 0x00000000;   //Black
        buttonProps.insert(std::pair<int, ControlProp>(buttonProp.controlID, buttonProp));

        HWND hwndButton5 = CreateWindowEx(NULL,
            L"BUTTON",
            L"My Button 5",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_NOTIFY | BS_PUSHBUTTON, //NOTE:  BS_NOTIFY will not send WM_NOTIFY message for BCN_HOTITEMCHANGE if BS_OWNERDRAW is set.
            50,
            550,
            205,
            102,
            hWnd,
            (HMENU)IDC_BUTTON5,
            GetModuleHandle(NULL),
            NULL);
        //Set the button properties which are used as defaults in the DrawControl function if no params are passed
        buttonProp.controlName = "Button5";
        buttonProp.controlID = IDC_BUTTON5;
        buttonProp.hBitmap = hbitAeroNormalNoText;
        buttonProp.drawTextStyle = DT_SINGLELINE | DT_CENTER | DT_CENTER;
        buttonProp.fontHeight = 12;
        buttonProp.textColor = 0x00000000;   //Black
        buttonProps.insert(std::pair<int, ControlProp>(buttonProp.controlID, buttonProp));
        //Change the text fontsize to 8 Not Bold
        //logFontButtons.lfHeight = -MulDiv(8, GetDeviceCaps(hdcButtonFont, LOGPIXELSY), 72);
        logFontButtons.lfWeight = FW_BOLD;
        hfontButtons = CreateFontIndirect(&logFontButtons);
        SendMessage(hwndButton5, WM_SETFONT, (WPARAM)hfontButtons, (LPARAM)MAKELONG(TRUE, 0));

        HWND hwndButton6 = CreateWindowEx(0, L"BUTTON", L"Button6\nSubclassed Ownerdraw\nResized Background",
            WS_CLIPSIBLINGS | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP |BS_NOTIFY |BS_PUSHBUTTON,     //NOTE:  BS_NOTIFY will not send WM_NOTIFY message for BCN_HOTITEMCHANGE if BS_OWNERDRAW is set.
            275,
            50,
            104,
            50,
            hWnd,
            (HMENU)IDC_BUTTON6,
            NULL,
            NULL);
        //Set the button properties which are used as defaults in the DrawControl function if no params are passed
        buttonProp.controlName = "Button6";
        buttonProp.controlID = IDC_BUTTON6;
        buttonProp.hBitmap = hbitAeroNormalNoText;
        buttonProp.drawTextStyle = DT_CENTER | DT_CENTER;
        buttonProp.fontHeight = 7;
        buttonProp.textColor = 0x00000000;   //Black
        buttonProps.insert(std::pair<int, ControlProp>(buttonProp.controlID, buttonProp));
        // create a region to shape the button
        //HRGN hRgn = CreateRoundRectRgn(0, 0, 200, 100, 40, 40);
        //SetWindowRgn(hwndButton1, hRgn, TRUE);

        //Subclass the button.  It will now use 'NewBtnProc' for WM_MOUSEMOVE, WM_LMOUSEUP/DOWN and WM_RMOUSEUP/DOWN and WM_PAINT messages.  Multiple buttons can be set to use 'NewBtnProc'
        oldWndProc = (WNDPROC)SetWindowLongPtr(hwndButton6, GWLP_WNDPROC, (LONG_PTR)NewBtnProc);


        break;
    }
    break;
    case WM_COMMAND:
    {
        switch (HIWORD(wParam))
        {
        case BN_DBLCLK:
            //OutputDebugString(L"BN_DBLCLK\n");
            break;
        case BN_SETFOCUS:
        {
            msgName = L"WM_COMMAND/BN_SETFOCUS";
            controlID = LOWORD(wParam);
            controlName = GetControlNameGivenID(controlID);
            StringCchPrintf(szBuff, bufferSize, TEXT("%s : %s : %s : %s \n"), currDateTime.c_str(), controlName.c_str(), msgName.c_str(), L"WndProc");
            //OutputDebugString(szBuff);
            break;
        }
        break;
        case BN_KILLFOCUS:
        {
            msgName = L"WM_COMMAND/BN_KILLFOCUS";
            controlID = LOWORD(wParam);
            controlName = GetControlNameGivenID(controlID);
            StringCchPrintf(szBuff, bufferSize, TEXT("%s : %s : %s : %s \n"), currDateTime.c_str(), controlName.c_str(), msgName.c_str(), L"WndProc");
            //OutputDebugString(szBuff);
            break;
        }
        break;
        case BN_UNHILITE:
            //OutputDebugString(L"BN_UNHILITE\n");
            break;
        case BN_DISABLE:
            //OutputDebugString(L"BN_DISABLE\n");
            break;
        case BN_HILITE:
            //OutputDebugString(L"BN_HILITE\n");
            break;
        case BN_PAINT:
            //OutputDebugString(L"BN_PAINT\n");
            break;

        case BN_CLICKED:
            switch (LOWORD(wParam))
            {
            case IDC_BUTTON1:
            {
                //SetFocus(hWnd);
                HWND hwndStatusBar = GetDlgItem(hWnd, IDC_STATUSBAR);
                //Don't use SendMessage or SendMessageA to update text in the StatusBar control.  Text comes out looking like Chinese.  Use SendWindowText instead.
                //SendMessageA(hwndStatusBar, SB_SETTEXT, 0, (LPARAM)"Some new text");
                SetWindowText(hwndStatusBar, L"WM_COMMAND/BN_CLICKED : Button1");

                //const int iResponse = MessageBox(hWnd, L"Are you sure?", L"Set Btn 5 Bitmap", MB_YESNO);
                //switch (iResponse)
                //{
                //    case IDYES:
                //    {
                //        //Load the bitmap onto the button that has text.  Note:  This bitmap will be used like an icon to the left of the text.
                //        //The bitmap must be smaller than the button (like you would use and icon).  If you try to use an image to replace the entire button background you will make a mess.
                //        SendMessage(GetDlgItem(hWnd,IDC_BUTTON5), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbitVMwareSmall);
                //    }
                //}
                break;
            }
            break;
            case IDC_BUTTON2:
            {
                HWND hwndStatusBar = GetDlgItem(hWnd, IDC_STATUSBAR);
                //Don't use SendMessage or SendMessageA to update text in the StatusBar control.  Text comes out looking like Chinese.  Use SendWindowText instead.
                //SendMessageA(hwndStatusBar, SB_SETTEXT, 0, (LPARAM)"Some new text");
                SetWindowText(hwndStatusBar, L"WM_COMMAND/BN_CLICKED : Button2");
            }  
            break;
            case IDC_BUTTON3:
            {
                //OutputDebugString(L"Button3 Clicked\n");
                HWND hwndStatusBar = GetDlgItem(hWnd, IDC_STATUSBAR);
                //Don't use SendMessage or SendMessageA to update text in the StatusBar control.  Text comes out looking like Chinese.  Use SendWindowText instead.
                //SendMessageA(hwndStatusBar, SB_SETTEXT, 0, (LPARAM)"Some new text");
                SetWindowText(hwndStatusBar, L"WM_COMMAND/BN_CLICKED : Button3");
            }
            break;
            case IDC_BUTTON4:
            {
                HWND hwndStatusBar = GetDlgItem(hWnd, IDC_STATUSBAR);
                //Don't use SendMessage or SendMessageA to update text in the StatusBar control.  Text comes out looking like Chinese.  Use SendWindowText instead.
                //SendMessageA(hwndStatusBar, SB_SETTEXT, 0, (LPARAM)"Some new text");
                SetWindowText(hwndStatusBar, L"WM_COMMAND/BN_CLICKED : Button4");

                //Get the button text for Button4
                HWND hwndButton4 = GetDlgItem(hWnd, IDC_BUTTON4);
                const int cTxtLen = GetWindowTextLength(hwndButton4);
                // Allocate memory for the string and copy 
                // the string into the memory. 
                std::wstring title;
                title.reserve(GetWindowTextLength(hwndButton4) + 1);
                GetWindowText(hwndButton4, const_cast<WCHAR*>(title.c_str()), title.capacity());

                const int iResponse = MessageBox(hWnd, L"Are you sure?", title.c_str(), MB_YESNO);
                switch (iResponse)
                {
                case IDYES:
                {
                    //EnableWindow(hwndButton3, false);
                    if (wcscmp(title.c_str(),L"Disable Button 3")== 0)
                    {
                        EnableWindow(GetDlgItem(hWnd, IDC_BUTTON3), false);
                        BOOL bSuccess = SetWindowTextA(GetDlgItem(hWnd, IDC_BUTTON4), "Enable Button 3");
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hWnd, IDC_BUTTON3), true);
                        BOOL bSuccess = SetWindowTextA(GetDlgItem(hWnd, IDC_BUTTON4), "Disable Button 3");
                    }
                    //OutputDebugString(L"Yes\n");
                    break;
                }
                case IDNO:
                    //OutputDebugString(L"No\n");
                    break;
                case IDCANCEL:
                    //OutputDebugString(L"Cancel\n");
                    break;
                default:
                    break;
                }
                break;
            }
            break;
            case IDC_BUTTON5:
            {
                HWND hwndStatusBar = GetDlgItem(hWnd, IDC_STATUSBAR);
                //Don't use SendMessage or SendMessageA to update text in the StatusBar control.  Text comes out looking like Chinese.  Use SendWindowText instead.
                //SendMessageA(hwndStatusBar, SB_SETTEXT, 0, (LPARAM)"Some new text");
                SetWindowText(hwndStatusBar, L"WM_COMMAND/BN_CLICKED : Button5 ");
                break;
            }
            break;
            case IDC_BUTTON6:
            {
                HWND hwndStatusBar = GetDlgItem(hWnd, IDC_STATUSBAR);
                //Don't use SendMessage or SendMessageA to update text in the StatusBar control.  Text comes out looking like Chinese.  Use SendWindowText instead.
                //SendMessageA(hwndStatusBar, SB_SETTEXT, 0, (LPARAM)"Some new text");
                SetWindowText(hwndStatusBar, L"WM_COMMAND/BN_CLICKED : Button6");
                break;
            }
            break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_DEBUG_DETAILS:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_DEBUG_VIEWER), hWnd, DebugDetailsProc);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
    }
    break;
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        //OutputDebugString(GetDRAWITEMSTRUCTasString(dis).c_str());
        if (dis->CtlType == ODT_BUTTON) {
            UINT controlId = dis->CtlID;
            switch (controlId)
            {
            case IDC_BUTTON1:
            {
                //SetDCBrushColor(hdc, RGB(255, 80, 202));
                //RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);

                //// Draw the button text
                //SetBkMode(hdc, TRANSPARENT);
                //SetTextColor(hdc, RGB(0, 0, 0));
                //DrawText(hdc, L"Set Btn5 Bitmap", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                //Below line doesn't work
                //SendMessage(GetDlgItem(hWnd, IDC_BUTTON1), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbitVMwareSmall);
                break;
            }
            break;
            case IDC_BUTTON2:
            {
                // Draw the button with rounded corners
                HDC hdc = dis->hDC;
                RECT rect = dis->rcItem;
                HBRUSH hBrush = CreateSolidBrush(RGB(255, 80, 202));
                // Set the background color
                //FillRect(hdc, &rect, (HBRUSH)(COLOR_BTNFACE + 1));
                FillRect(hdc, &rect, hBrush);
                // Create a rounded rectangle
                int radius = 10; // Radius for rounded corners
                SetDCBrushColor(hdc, RGB(255, 80, 202));
                RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);

                // Draw the button text
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(0, 0, 0));
                DrawText(hdc, L"Rounded Btn 2", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            break;
            case IDC_BUTTON5:
            {
                // Draw the button with rounded corners
                HDC hdc = dis->hDC;
                RECT rect = dis->rcItem;
                HBRUSH hBrush = CreateSolidBrush(RGB(255, 80, 202));
                // Set the background color
                //FillRect(hdc, &rect, (HBRUSH)(COLOR_BTNFACE + 1));
                FillRect(hdc, &rect, hBrush);
                // Create a rounded rectangle
                int radius = 10; // Radius for rounded corners
                SetDCBrushColor(hdc, RGB(255, 80, 202));
                RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);

                // Draw the button text
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(0, 0, 0));
                DrawText(hdc, L"My Btn 5", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                //OutputDebugString(L"WM_DRAWITEM My Btn 5\n");
                break;
            }
            break;
            case IDC_BUTTON6:
            {
                //OutputDebugString(L"WM_DRAWITEM My Btn 6\n");
                break;
            }
            }

        }
        return TRUE;
    }
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for Debug Details Dialog (IDD_DEBUG_DETAILS)
INT_PTR CALLBACK DebugDetailsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        hwndDebugDetailsDialog = hDlg;
        return (INT_PTR)TRUE;

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_BTN_DETAILS:
        {
            HWND hwndEditDebug;
            hwndEditDebug = GetDlgItem(hDlg, IDC_EDIT_DEBUG);
            std::wstring debugMsg;
            map<int, wstring>mapDebugMsg;

            debugMsg.reserve(GetWindowTextLength(hwndEditDebug) + 1);
            GetWindowText(hwndEditDebug, const_cast<WCHAR*>(debugMsg.c_str()), debugMsg.capacity());

            //MessageBox(hDlg, debugMsg.c_str(), L"IDC_BTN_DETAILS", MB_OK);

            mapDebugMsg = SplitStringW(debugMsg.c_str());
            expandDebugMsg(mapDebugMsg);
        }   
        break;
        case ID_BTN_OK:
            // Don't put a break on this case.  We cant it to 'fall through' to the BTN_CANCEL case as both messages should close the dialog
        case ID_BTN_CANCEL:
        {
            // Close the dialog
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
        default:
            break;
        }
    }
    break;
    case WM_CLOSE:
    {
        EndDialog(hDlg, 0);
        return (INT_PTR)TRUE;
    }
    break;
    }

    return (INT_PTR)FALSE;
}

std::wstring GetControlNameGivenID(int controlId)
{
    std::wstring controlName = L"Unknown Control";
    std::wstring strControlID;
    strControlID = std::to_wstring(controlId);

    if (controlId == controlidMainWindow)
    {
        controlName = L"MainWindow";
    }
    else
    {
        switch (controlId)
        {
        case IDC_BUTTON1:
            controlName = L"Button1";
            break;
        case IDC_BUTTON2:
            controlName = L"Button2";
            break;
        case IDC_BUTTON3:
            controlName = L"Button3";
            break;
        case IDC_BUTTON4:
            controlName = L"Button4";
            break;
        case IDC_BUTTON5:
            controlName = L"Button5";
            break;
        case IDC_BUTTON6:
            controlName = L"Button6";
            break;
        default:
            controlName = L"Unknown Control (" + strControlID + L")";
            break;
        }
    }

    return controlName;

}

std::wstring GetWindowsMsgNameGivenMsg(int windowsMsg)
{
    std::wstring stringMsg = L"Unknown Message";
    std::wstring strMsgID;
    strMsgID = std::to_wstring(windowsMsg);

    switch (windowsMsg)
    {
            case 0: stringMsg = L"WM_NULL"; break;
            case 1: stringMsg = L"WM_CREATE"; break;
            case 2: stringMsg = L"WM_DESTROY"; break;
            case 3: stringMsg = L"WM_MOVE"; break;
            case 5: stringMsg = L"WM_SIZE"; break;
            case 6: stringMsg = L"WM_ACTIVATE"; break;
            case 7: stringMsg = L"WM_SETFOCUS"; break;
            case 8: stringMsg = L"WM_KILLFOCUS"; break;
            case 10: stringMsg = L"WM_ENABLE"; break;
            case 11: stringMsg = L"WM_SETREDRAW"; break;
            case 12: stringMsg = L"WM_SETTEXT"; break;
            case 13: stringMsg = L"WM_GETTEXT"; break;
            case 14: stringMsg = L"WM_GETTEXTLENGTH"; break;
            case 15: stringMsg = L"WM_PAINT"; break;
            case 16: stringMsg = L"WM_CLOSE"; break;
            case 17: stringMsg = L"WM_QUERYENDSESSION"; break;
            case 18: stringMsg = L"WM_QUIT"; break;
            case 19: stringMsg = L"WM_QUERYOPEN"; break;
            case 20: stringMsg = L"WM_ERASEBKGND"; break;
            case 21: stringMsg = L"WM_SYSCOLORCHANGE"; break;
            case 22: stringMsg = L"WM_ENDSESSION"; break;
            case 24: stringMsg = L"WM_SHOWWINDOW"; break;
            case 25: stringMsg = L"WM_CTLCOLOR"; break;
            case 26: stringMsg = L"WM_WININICHANGE"; break;
            case 27: stringMsg = L"WM_DEVMODECHANGE"; break;
            case 28: stringMsg = L"WM_ACTIVATEAPP"; break;
            case 29: stringMsg = L"WM_FONTCHANGE"; break;
            case 30: stringMsg = L"WM_TIMECHANGE"; break;
            case 31: stringMsg = L"WM_CANCELMODE"; break;
            case 32: stringMsg = L"WM_SETCURSOR"; break;
            case 33: stringMsg = L"WM_MOUSEACTIVATE"; break;
            case 34: stringMsg = L"WM_CHILDACTIVATE"; break;
            case 35: stringMsg = L"WM_QUEUESYNC"; break;
            case 36: stringMsg = L"WM_GETMINMAXINFO"; break;
            case 38: stringMsg = L"WM_PAINTICON"; break;
            case 39: stringMsg = L"WM_ICONERASEBKGND"; break;
            case 40: stringMsg = L"WM_NEXTDLGCTL"; break;
            case 42: stringMsg = L"WM_SPOOLERSTATUS"; break;
            case 43: stringMsg = L"WM_DRAWITEM"; break;
            case 44: stringMsg = L"WM_MEASUREITEM"; break;
            case 45: stringMsg = L"WM_DELETEITEM"; break;
            case 46: stringMsg = L"WM_VKEYTOITEM"; break;
            case 47: stringMsg = L"WM_CHARTOITEM"; break;
            case 48: stringMsg = L"WM_SETFONT"; break;
            case 49: stringMsg = L"WM_GETFONT"; break;
            case 50: stringMsg = L"WM_SETHOTKEY"; break;
            case 51: stringMsg = L"WM_GETHOTKEY"; break;
            case 55: stringMsg = L"WM_QUERYDRAGICON"; break;
            case 57: stringMsg = L"WM_COMPAREITEM"; break;
            case 61: stringMsg = L"WM_GETOBJECT"; break;
            case 65: stringMsg = L"WM_COMPACTING"; break;
            case 68: stringMsg = L"WM_COMMNOTIFY"; break;
            case 70: stringMsg = L"WM_WINDOWPOSCHANGING"; break;
            case 71: stringMsg = L"WM_WINDOWPOSCHANGED"; break;
            case 72: stringMsg = L"WM_POWER"; break;
            case 73: stringMsg = L"WM_COPYGLOBALDATA"; break;
            case 74: stringMsg = L"WM_COPYDATA"; break;
            case 75: stringMsg = L"WM_CANCELJOURNAL"; break;
            case 78: stringMsg = L"WM_NOTIFY"; break;
            case 80: stringMsg = L"WM_INPUTLANGCHANGEREQUEST"; break;
            case 81: stringMsg = L"WM_INPUTLANGCHANGE"; break;
            case 82: stringMsg = L"WM_TCARD"; break;
            case 83: stringMsg = L"WM_HELP"; break;
            case 84: stringMsg = L"WM_USERCHANGED"; break;
            case 85: stringMsg = L"WM_NOTIFYFORMAT"; break;
            case 123: stringMsg = L"WM_CONTEXTMENU"; break;
            case 124: stringMsg = L"WM_STYLECHANGING"; break;
            case 125: stringMsg = L"WM_STYLECHANGED"; break;
            case 126: stringMsg = L"WM_DISPLAYCHANGE"; break;
            case 127: stringMsg = L"WM_GETICON"; break;
            case 128: stringMsg = L"WM_SETICON"; break;
            case 129: stringMsg = L"WM_NCCREATE"; break;
            case 130: stringMsg = L"WM_NCDESTROY"; break;
            case 131: stringMsg = L"WM_NCCALCSIZE"; break;
            case 132: stringMsg = L"WM_NCHITTEST"; break;
            case 133: stringMsg = L"WM_NCPAINT"; break;
            case 134: stringMsg = L"WM_NCACTIVATE"; break;
            case 135: stringMsg = L"WM_GETDLGCODE"; break;
            case 136: stringMsg = L"WM_SYNCPAINT"; break;
            case 160: stringMsg = L"WM_NCMOUSEMOVE"; break;
            case 161: stringMsg = L"WM_NCLBUTTONDOWN"; break;
            case 162: stringMsg = L"WM_NCLBUTTONUP"; break;
            case 163: stringMsg = L"WM_NCLBUTTONDBLCLK"; break;
            case 164: stringMsg = L"WM_NCRBUTTONDOWN"; break;
            case 165: stringMsg = L"WM_NCRBUTTONUP"; break;
            case 166: stringMsg = L"WM_NCRBUTTONDBLCLK"; break;
            case 167: stringMsg = L"WM_NCMBUTTONDOWN"; break;
            case 168: stringMsg = L"; break;WM_NCMBUTTONUP"; break;
            case 169: stringMsg = L"; break;WM_NCMBUTTONDBLCLK"; break;
            case 171: stringMsg = L"WM_NCXBUTTONDOWN"; break;
            case 172: stringMsg = L"WM_NCXBUTTONUP"; break;
            case 173: stringMsg = L"WM_NCXBUTTONDBLCLK"; break;
            case 176: stringMsg = L"EM_GETSEL"; break;
            case 177: stringMsg = L"EM_SETSEL"; break;
            case 178: stringMsg = L"EM_GETRECT"; break;
            case 179: stringMsg = L"EM_SETRECT"; break;
            case 180: stringMsg = L"EM_SETRECTNP"; break;
            case 181: stringMsg = L"EM_SCROLL"; break;
            case 182: stringMsg = L"EM_LINESCROLL"; break;
            case 183: stringMsg = L"EM_SCROLLCARET"; break;
            case 185: stringMsg = L"EM_GETMODIFY"; break;
            case 187: stringMsg = L"EM_SETMODIFY"; break;
            case 188: stringMsg = L"EM_GETLINECOUNT"; break;
            case 189: stringMsg = L"EM_LINEINDEX"; break;
            case 190: stringMsg = L"EM_SETHANDLE"; break;
            case 191: stringMsg = L"EM_GETHANDLE"; break;
            case 192: stringMsg = L"EM_GETTHUMB"; break;
            case 193: stringMsg = L"EM_LINELENGTH"; break;
            case 194: stringMsg = L"EM_REPLACESEL"; break;
            case 195: stringMsg = L"EM_SETFONT"; break;
            case 196: stringMsg = L"EM_GETLINE"; break;
            //case 197: stringMsg = L"EM_LIMITTEXT"; break;
            case 197: stringMsg = L"EM_SETLIMITTEXT"; break;
            case 198: stringMsg = L"EM_CANUNDO"; break;
            case 199: stringMsg = L"EM_UNDO"; break;
            case 200: stringMsg = L"EM_FMTLINES"; break;
            case 201: stringMsg = L"EM_LINEFROMCHAR"; break;
            case 202: stringMsg = L"EM_SETWORDBREAK"; break;
            case 203: stringMsg = L"; break;EM_SETTABSTOPS"; break;
            case 204: stringMsg = L"; break;EM_SETPASSWORDCHAR"; break;
            case 205: stringMsg = L"; break;EM_EMPTYUNDOBUFFER"; break;
            case 206: stringMsg = L"; break;EM_GETFIRSTVISIBLELINE"; break;
            case 207: stringMsg = L"; break;EM_SETREADONLY"; break;
            case 209: stringMsg = L"; break;EM_SETWORDBREAKPROC"; break;
            //case 209: stringMsg = L"; break;EM_GETWORDBREAKPROC"; break;
            case 210: stringMsg = L"; break;EM_GETPASSWORDCHAR"; break;
            case 211: stringMsg = L"; break;EM_SETMARGINS"; break;
            case 212: stringMsg = L"; break;EM_GETMARGINS"; break;
            case 213: stringMsg = L"; break;EM_GETLIMITTEXT"; break;
            case 214: stringMsg = L"EM_POSFROMCHAR"; break;
            case 215: stringMsg = L"EM_CHARFROMPOS"; break;
            case 216: stringMsg = L"EM_SETIMESTATUS"; break;
            case 217: stringMsg = L"EM_GETIMESTATUS"; break;
            case 224: stringMsg = L"SBM_SETPOS"; break;
            case 225: stringMsg = L"SBM_GETPOS"; break;
            case 226: stringMsg = L"SBM_SETRANGE"; break;
            case 227: stringMsg = L"SBM_GETRANGE"; break;
            case 228: stringMsg = L"SBM_ENABLE_ARROWS"; break;
            case 230: stringMsg = L"SBM_SETRANGEREDRAW"; break;
            case 233: stringMsg = L"SBM_SETSCROLLINFO"; break;
            case 234: stringMsg = L"SBM_GETSCROLLINFO"; break;
            case 235: stringMsg = L"SBM_GETSCROLLBARINFO"; break;
            case 240: stringMsg = L"BM_GETCHECK"; break;
            case 241: stringMsg = L"BM_SETCHECK"; break;
            case 242: stringMsg = L"BM_GETSTATE"; break;
            case 243: stringMsg = L"BM_SETSTATE"; break;
            case 244: stringMsg = L"BM_SETSTYLE"; break;
            case 245: stringMsg = L"BM_CLICK"; break;
            case 246: stringMsg = L"BM_GETIMAGE"; break;
            case 247: stringMsg = L"BM_SETIMAGE"; break;
            case 248: stringMsg = L"BM_SETDONTCLICK"; break;
            case 255: stringMsg = L"WM_INPUT"; break;
            case 256: stringMsg = L"WM_KEYDOWN"; break;
            //case 256: stringMsg = L"WM_KEYFIRST"; break;
            case 257: stringMsg = L"WM_KEYUP"; break;
            case 258: stringMsg = L"WM_CHAR"; break;
            case 259: stringMsg = L"WM_DEADCHAR"; break;
            case 260: stringMsg = L"WM_SYSKEYDOWN"; break;
            case 261: stringMsg = L"WM_SYSKEYUP"; break;
            case 262: stringMsg = L"WM_SYSCHAR"; break;
            case 263: stringMsg = L"WM_SYSDEADCHAR"; break;
            case 264: stringMsg = L"WM_KEYLAST"; break;
            case 265: stringMsg = L"WM_UNICHAR"; break;
            //case 265: stringMsg = L"WM_WNT_CONVERTREQUESTEX"; break;
            case 266: stringMsg = L"WM_CONVERTREQUEST"; break;
            case 267: stringMsg = L"WM_CONVERTRESULT"; break;
            case 268: stringMsg = L"WM_INTERIM"; break;
            case 269: stringMsg = L"WM_IME_STARTCOMPOSITION"; break;
            case 270: stringMsg = L"WM_IME_ENDCOMPOSITION"; break;
            case 271: stringMsg = L"WM_IME_COMPOSITION"; break;
            //case 271: stringMsg = L"WM_IME_KEYLAST"; break;
            case 272: stringMsg = L"WM_INITDIALOG"; break;
            case 273: stringMsg = L"WM_COMMAND"; break;
            case 274: stringMsg = L"WM_SYSCOMMAND"; break;
            case 275: stringMsg = L"WM_TIMER"; break;
            case 276: stringMsg = L"WM_HSCROLL"; break;
            case 277: stringMsg = L"WM_VSCROLL"; break;
            case 278: stringMsg = L"WM_INITMENU"; break;
            case 279: stringMsg = L"WM_INITMENUPOPUP"; break;
            case 280: stringMsg = L"WM_SYSTIMER"; break;
            case 287: stringMsg = L"WM_MENUSELECT"; break;
            case 288: stringMsg = L"WM_MENUCHAR"; break;
            case 289: stringMsg = L"WM_ENTERIDLE"; break;
            case 290: stringMsg = L"WM_MENURBUTTONUP"; break;
            case 291: stringMsg = L"WM_MENUDRAG"; break;
            case 292: stringMsg = L"WM_MENUGETOBJECT"; break;
            case 293: stringMsg = L"WM_UNINITMENUPOPUP"; break;
            case 294: stringMsg = L"WM_MENUCOMMAND"; break;
            case 295: stringMsg = L"WM_CHANGEUISTATE"; break;
            case 296: stringMsg = L"WM_UPDATEUISTATE"; break;
            case 297: stringMsg = L"WM_QUERYUISTATE"; break;
            case 306: stringMsg = L"WM_CTLCOLORMSGBOX"; break;
            case 307: stringMsg = L"WM_CTLCOLOREDIT"; break;
            case 308: stringMsg = L"WM_CTLCOLORLISTBOX"; break;
            case 309: stringMsg = L"WM_CTLCOLORBTN"; break;
            case 310: stringMsg = L"WM_CTLCOLORDLG"; break;
            case 311: stringMsg = L"WM_CTLCOLORSCROLLBAR"; break;
            case 312: stringMsg = L"WM_CTLCOLORSTATIC"; break;
            //case 512: stringMsg = L"WM_MOUSEFIRST"; break;
            case 512: stringMsg = L"WM_MOUSEMOVE"; break;
            case 513: stringMsg = L"WM_LBUTTONDOWN"; break;
            case 514: stringMsg = L"WM_LBUTTONUP"; break;
            case 515: stringMsg = L"WM_LBUTTONDBLCLK"; break;
            case 516: stringMsg = L"WM_RBUTTONDOWN"; break;
            case 517: stringMsg = L"WM_RBUTTONUP"; break;
            case 518: stringMsg = L"WM_RBUTTONDBLCLK"; break;
            case 519: stringMsg = L"WM_MBUTTONDOWN"; break;
            case 520: stringMsg = L"WM_MBUTTONUP"; break;
            case 521: stringMsg = L"WM_MBUTTONDBLCLK"; break;
            //case 521: stringMsg = L"WM_MOUSELAST"; break;
            case 522: stringMsg = L"WM_MOUSEWHEEL"; break;
            case 523: stringMsg = L"WM_XBUTTONDOWN"; break;
            case 524: stringMsg = L"WM_XBUTTONUP"; break;
            case 525: stringMsg = L"WM_XBUTTONDBLCLK"; break;
            case 528: stringMsg = L"WM_PARENTNOTIFY"; break;
            case 529: stringMsg = L"WM_ENTERMENULOOP"; break;
            case 530: stringMsg = L"WM_EXITMENULOOP"; break;
            case 531: stringMsg = L"WM_NEXTMENU"; break;
            case 532: stringMsg = L"WM_SIZING"; break;
            case 533: stringMsg = L"WM_CAPTURECHANGED"; break;
            case 534: stringMsg = L"WM_MOVING"; break;
            case 536: stringMsg = L"WM_POWERBROADCAST"; break;
            case 537: stringMsg = L"WM_DEVICECHANGE"; break;
            case 544: stringMsg = L"WM_MDICREATE"; break;
            case 545: stringMsg = L"WM_MDIDESTROY"; break;
            case 546: stringMsg = L"WM_MDIACTIVATE"; break;
            case 547: stringMsg = L"WM_MDIRESTORE"; break;
            case 548: stringMsg = L"WM_MDINEXT"; break;
            case 549: stringMsg = L"WM_MDIMAXIMIZE"; break;
            case 550: stringMsg = L"WM_MDITILE"; break;
            case 551: stringMsg = L"WM_MDICASCADE"; break;
            case 552: stringMsg = L"WM_MDIICONARRANGE"; break;
            case 553: stringMsg = L"WM_MDIGETACTIVE"; break;
            case 560: stringMsg = L"WM_MDISETMENU"; break;
            case 561: stringMsg = L"WM_ENTERSIZEMOVE"; break;
            case 562: stringMsg = L"WM_EXITSIZEMOVE"; break;
            case 563: stringMsg = L"WM_DROPFILES"; break;
            case 564: stringMsg = L"WM_MDIREFRESHMENU"; break;
            case 640: stringMsg = L"WM_IME_REPORT"; break;
            case 641: stringMsg = L"WM_IME_SETCONTEXT"; break;
            case 642: stringMsg = L"WM_IME_NOTIFY"; break;
            case 643: stringMsg = L"WM_IME_CONTROL"; break;
            case 644: stringMsg = L"WM_IME_COMPOSITIONFULL"; break;
            case 645: stringMsg = L"WM_IME_SELECT"; break;
            case 646: stringMsg = L"WM_IME_CHAR"; break;
            case 648: stringMsg = L"WM_IME_REQUEST"; break;
            //case 656: stringMsg = L"WM_IMEKEYDOWN"; break;
            case 656: stringMsg = L"WM_IME_KEYDOWN"; break;
            //case 657: stringMsg = L"WM_IMEKEYUP"; break;
            case 657: stringMsg = L"WM_IME_KEYUP"; break;
            case 672: stringMsg = L"WM_NCMOUSEHOVER"; break;
            case 673: stringMsg = L"WM_MOUSEHOVER"; break;
            case 674: stringMsg = L"WM_NCMOUSELEAVE"; break;
            case 675: stringMsg = L"WM_MOUSELEAVE"; break;
            case 768: stringMsg = L"WM_CUT"; break;
            case 769: stringMsg = L"WM_COPY"; break;
            case 770: stringMsg = L"WM_PASTE"; break;
            case 771: stringMsg = L"WM_CLEAR"; break;
            case 772: stringMsg = L"WM_UNDO"; break;
            case 773: stringMsg = L"WM_RENDERFORMAT"; break;
            case 774: stringMsg = L"WM_RENDERALLFORMATS"; break;
            case 775: stringMsg = L"WM_DESTROYCLIPBOARD"; break;
            case 776: stringMsg = L"WM_DRAWCLIPBOARD"; break;
            case 777: stringMsg = L"WM_PAINTCLIPBOARD"; break;
            case 778: stringMsg = L"WM_VSCROLLCLIPBOARD"; break;
            case 779: stringMsg = L"WM_SIZECLIPBOARD"; break;
            case 780: stringMsg = L"WM_ASKCBFORMATNAME"; break;
            case 781: stringMsg = L"WM_CHANGECBCHAIN"; break;
            case 782: stringMsg = L"WM_HSCROLLCLIPBOARD"; break;
            case 783: stringMsg = L"WM_QUERYNEWPALETTE"; break;
            case 784: stringMsg = L"WM_PALETTEISCHANGING"; break;
            case 785: stringMsg = L"WM_PALETTECHANGED"; break;
            case 786: stringMsg = L"WM_HOTKEY"; break;
            case 791: stringMsg = L"WM_PRINT"; break;
            case 792: stringMsg = L"WM_PRINTCLIENT"; break;
            case 793: stringMsg = L"WM_APPCOMMAND"; break;
            case 856: stringMsg = L"WM_HANDHELDFIRST"; break;
            case 863: stringMsg = L"WM_HANDHELDLAST"; break;
            case 864: stringMsg = L"WM_AFXFIRST"; break;
            case 895: stringMsg = L"WM_AFXLAST"; break;
            case 896: stringMsg = L"WM_PENWINFIRST"; break;
            case 897: stringMsg = L"WM_RCRESULT"; break;
            case 898: stringMsg = L"WM_HOOKRCRESULT"; break;
            //case 899: stringMsg = L"WM_GLOBALRCCHANGE"; break;
            case 899: stringMsg = L"WM_PENMISCINFO"; break;
            case 900: stringMsg = L"WM_SKB"; break;
            //case 901: stringMsg = L"WM_HEDITCTL"; break;
            case 901: stringMsg = L"WM_PENCTL"; break;
            case 902: stringMsg = L"WM_PENMISC"; break;
            case 903: stringMsg = L"WM_CTLINIT"; break;
            case 904: stringMsg = L"WM_PENEVENT"; break;
            case 911: stringMsg = L"WM_PENWINLAST"; break;
            case 1024: stringMsg = L"WM_USER"; break;
            default:
                stringMsg = L"Unknown Msg (" + strMsgID + L")";
                break;
    }
    return stringMsg;
}

wstring GetSubMessage(UINT msg, WPARAM wparam, LPARAM lparam)
{
    wstring cmd_NotifMsg = L"";

    switch (msg)
    {
    case WM_COMMAND:
    {
        switch (HIWORD(wparam))
        {
        case BN_DBLCLK:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_DBLCLK";
        }
        break;
        case BN_SETFOCUS:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_SETFOCUS";
        }
        break;;
        case BN_KILLFOCUS:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_KILLFOCUS";
        }
        break;;
        case BN_UNHILITE:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_UNHILITE";
        }
        break;
        case BN_DISABLE:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_DISABLE";
        }
        break;
        case BN_HILITE:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_HILITE";
        }
        break;
        case BN_PAINT:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_PAINT";
        }
        break;
        case BN_CLICKED:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_CLICKED | ";
            switch (LOWORD(wparam))
            {
            case IDC_BUTTON1:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDC_BUTTON1";
            }
            break;
            case IDC_BUTTON2:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDC_BUTTON2";
            }
            break;
            case IDC_BUTTON3:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDC_BUTTON3";
            }
            break;
            case IDC_BUTTON4:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDC_BUTTON4";
            }
            break;
            case IDC_BUTTON5:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDC_BUTTON5";
            }
            break;
            case IDC_BUTTON6:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDC_BUTTON6";
            }
            break;
            case IDM_ABOUT:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDM_ABOUT";
            }
            break;
            case IDM_DEBUG_DETAILS:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDM_DEBUG_DETAILS";
            }
            break;
            case IDM_EXIT:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDM_EXIT";
            }
            break;
            default:
                break;
            }
        }
        break;
        default:
            break;
        }
    }
    break;
    case WM_NOTIFY:
        break;
    default:
        break;
    }
    return cmd_NotifMsg;
}

std::wstring GetTimeDate()
{
    std::wstring currDateTime;

    SYSTEMTIME st;
    GetLocalTime(&st);
    //sprintf_s(buffer, "Current time: %02d:%02d:%02d\n", st.wHour, st.wMinute, st.wSecond);

    StringCchPrintf(szBuff, bufferSize, TEXT("%02d/%02d/%02d %02d:%02d:%02d"),st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    return szBuff;

}

std::wstring GetDRAWITEMSTRUCTasString(LPDRAWITEMSTRUCT dis)
{
    std::wstring currDateTime = GetTimeDate();
    std::wstring resultString = L"Unknown DRAWITEMSTRUCT";
    std::wstring controlType = GetControlTypeFromDRAWITEMSTRUCT(dis);
    std::wstring controlName = GetControlNameGivenID(dis->CtlID);
    std::wstring itemID = std::to_wstring(dis->itemID);
    std::wstring itemAction = GetItemActionFromDRAWITEMSTRUCT(dis);
    std::wstring itemState = GetItemStateFromDRAWITEMSTRUCT(dis);
    //std::wstring hwndItem = ConvertHWDNtoString(dis->hwndItem);
    //std::wstring hdc = ConvertHDCtoString(dis->hDC);
    resultString = currDateTime + L" : WM_DRAWITEMSTRUCT " + controlType + L" : " + controlName + L" : " + itemID + L" : " + itemAction + L" : " + itemState + L" WndProc\n";
    return resultString;
}

std::wstring GetControlTypeFromDRAWITEMSTRUCT(LPDRAWITEMSTRUCT dis)
{
    std::wstring controlType = L"Unknown Control";

    switch (dis->CtlType)
    {
    case ODT_BUTTON: controlType = L"ODT_BUTTON"; break;
    case ODT_COMBOBOX: controlType = L"ODT_COMBOBOX"; break;
    case ODT_LISTBOX: controlType = L"ODT_LISTBOX"; break;
    case ODT_LISTVIEW: controlType = L"ODT_LISTVIEW"; break;
    case ODT_MENU: controlType = L"ODT_MENU"; break;
    case ODT_STATIC: controlType = L"ODT_STATIC"; break;
    case ODT_TAB: controlType = L"ODT_TAB"; break;
    default: controlType = L"Unknown Control"; break;
    }
    return controlType;
}

std::wstring GetItemActionFromDRAWITEMSTRUCT(LPDRAWITEMSTRUCT dis)
{
    std::wstring itemAction = L"Unknown Action";

    switch (dis->itemAction)
    {
    case ODA_DRAWENTIRE: itemAction = L"ODA_DRAWENTIRE"; break;
    case ODA_FOCUS: itemAction = L"ODA_FOCUS"; break;
    case ODA_SELECT: itemAction = L"ODA_SELECT"; break;
    default: itemAction = L"Unknown Action"; break;
    }
    return itemAction;
}

std::wstring GetItemStateFromDRAWITEMSTRUCT(LPDRAWITEMSTRUCT dis)
{
    std::wstring itemState = L"Unknown State";

    switch (dis->itemAction)
    {
    case ODS_CHECKED: itemState = L"ODS_CHECKED"; break;
    case ODS_COMBOBOXEDIT: itemState = L"ODS_COMBOBOXEDIT"; break;
    case ODS_DEFAULT: itemState = L"ODS_DEFAULT"; break;
    case ODS_DISABLED: itemState = L"ODS_DISABLED"; break;
    case ODS_FOCUS: itemState = L"ODS_FOCUS"; break;
    case ODS_GRAYED: itemState = L"ODS_GRAYED"; break;
    case ODS_HOTLIGHT: itemState = L"ODS_HOTLIGHT"; break;
    case ODS_INACTIVE: itemState = L"ODS_INACTIVE"; break;
    case ODS_NOACCEL: itemState = L"ODS_NOACCEL"; break;
    case ODS_NOFOCUSRECT: itemState = L"ODS_NOFOCUSRECT"; break;
    case ODS_SELECTED: itemState = L"ODS_SELECTED"; break;
    default: itemState = L"Unknown State"; break;
    }
    return itemState;
}

std::wstring ConvertHWDNtoString(HWND hwnd)
{
    std::stringstream ssTmp;
    std::wstring sHwnd;
    ssTmp << hwnd;
    sHwnd = std::wstring(ssTmp.str().begin(), ssTmp.str().end());
    return sHwnd;
}

std::wstring ConvertHDCtoString(HDC hdc)
{
    std::stringstream ssTmp;
    std::wstring sHdc;
    ssTmp << hdc;
    sHdc = std::wstring(ssTmp.str().begin(), ssTmp.str().end());
    return sHdc;
}

void StretchImage(HDC hdcDest, HDC hdcSrc, int destX, int destY, int destWidth, int destHeight, int srcX, int srcY, int srcWidth, int srcHeight) 
{
    StretchBlt(hdcDest, destX, destY, destWidth, destHeight, hdcSrc, srcX, srcY, srcWidth, srcHeight, SRCCOPY);
}

void DrawControl(int controlID, HBITMAP hbitPreLoadedBitmap, FocusState hasFocus, int fontWeight, int fontHeight, COLORREF textColor, int drawTextStyle, LPCWSTR buttonText)
{
    //Pass this function the HWND of any button control (and associated optional parameters) and it will stretch the background image to fit the control, set it's font size and text
    //NOTE:  All the params are optional and have default values of NULL (fontWeight and fontHeight defaults to -1).
    //NOTE:  When a param = NULL or -1 then the value currently stored in buttonProps for this control is used.  buttonProps values will be updated at the bottom of this function.
    
    HWND hwndControl = GetDlgItem(hWnd,controlID);
    HDC hdc = GetDC(hwndControl);

    RECT rect;
    GetClientRect(hwndControl, &rect);

    //If no preloaded bitmap param is passed then use hBitmap value in buttonProps
    if (hbitPreLoadedBitmap == NULL)
    {
        hbitPreLoadedBitmap = buttonProps.at(controlID).hBitmap;
    }
    
    if (hbitPreLoadedBitmap != NULL)
    {
        hdcMem01 = CreateCompatibleDC(hdc);
        oldBitmap01 = SelectObject(hdcMem01, hbitPreLoadedBitmap);
        GetObject(hbitPreLoadedBitmap, sizeof(bitmap01), &bitmap01);
        //BitBlt(hdc, 0, 0, bitmap01.bmWidth, bitmap01.bmHeight, hdcMem01, 0, 0, SRCCOPY);
        //Below we use StretchBlt instead of BitBlt because Button6 is smaller than the bitmap image but we want the image to resize proportionate to the button
        StretchBlt(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hdcMem01, 0, 0, bitmap01.bmWidth, bitmap01.bmHeight, SRCCOPY);
    }

    //If ButtonText param wasn't specified then use the current text which we copied into the controls WindowText
    if (buttonText == NULL)
    {
        TCHAR btnText[256];
        GetWindowText(hwndControl, btnText, 256);
        buttonText = btnText;
    }

    //If FontSize param wasn't specified then set it to the value in buttonProps
    if (fontHeight == -1)
    {
        fontHeight = buttonProps.at(controlID).fontHeight;
    }

    //Below is how you set the font for ownerdraw subclassed controls
    logFontButtons.lfHeight = -MulDiv(fontHeight, GetDeviceCaps(hdcButtonFont, LOGPIXELSY), 72);

    //If fontWeight param isn't specified then get value from buttonProps
    if (fontWeight == -1)
    {
        fontWeight = buttonProps.at(controlID).fontWeight;
    }
    logFontButtons.lfWeight = fontWeight;
    hfontButtons = CreateFontIndirect(&logFontButtons);
    SelectFont(hdc, hfontButtons);

    //If no drawTextStyle param was specified then use the value from buttonProps
    if (drawTextStyle == -1)
    {
        drawTextStyle = buttonProps.at(controlID).drawTextStyle;
    }

    //If no textColor param is passed then use the value from buttonProps
    if (textColor == -1)
    {
        textColor = buttonProps.at(controlID).textColor;
    }
    //Below is the only way I've found to put text on a button that uses a background bitmap (one that fills the entire button.
    SetTextColor(hdc, textColor);
    SetBkMode(hdc, TRANSPARENT);
    //Now draw the new text
    DrawText(hdc, buttonText, -1, &rect, drawTextStyle);
    //Whenever we use DrawText on a subclassed ownerdraw button control we need to call SetWindowText also.  This way we can use GetWindowText to find out the text later.  There's no way to do a 'GetDrawText' since you are basically painting the text.
    SetWindowText(hwndControl, buttonText);

    //If hasFocus param is FS_NULL then get the value from buttonProps
    if (hasFocus == FS_NULL)
    {
        hasFocus = buttonProps.at(controlID).hasFocus;
    }

    //If the control has focus then display the focus rectangle
    if (hasFocus == FS_FOCUSED)
    {
        rect.left = rect.left + 2;
        rect.top = rect.top + 2;
        rect.right = rect.right - 2;
        rect.bottom = rect.bottom - 2;
        DrawFocusRect(hdc, &rect);
    }

    //Update the buttonProps values that might have changed.
    buttonProps.at(controlID).hBitmap = hbitPreLoadedBitmap;
    buttonProps.at(controlID).hasFocus = hasFocus;
    buttonProps.at(controlID).fontHeight = fontHeight;
    buttonProps.at(controlID).drawTextStyle = drawTextStyle;
    buttonProps.at(controlID).fontWeight = fontWeight;

    DeleteDC(hdcMem01);
    //SetFocus(hwndControl);
}

map<int, wstring> SplitStringW(std::wstring strToSplit)
{
    map<int, wstring> mapSplitString;
    TCHAR delimiter = '|';
    std::wstringstream iss(strToSplit);
    std::wstring token;
    int i = 0;
    while (std::getline(iss, token, delimiter)) 
    {
        token = trim(token);
        //MessageBox(hWnd,token.c_str(), L"Split String", MB_OK);
        //mapSplitString.insert({ std::make_pair( i, token ) });
        mapSplitString.insert({ i, token });
        i++;
    }

    return mapSplitString;
}

std::wstring trim(const std::wstring& str) 
{
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) ++start;
    auto end = str.end();
    do { --end; } while (end != start && std::isspace(*end));
    return std::wstring(start, end + 1);
}

void expandDebugMsg(map<int, wstring>mapDebugMsg)
{
    int myVariable1;
    wstring dateTime = mapDebugMsg[0];
    wstring hwnd = mapDebugMsg[1];
    wstring controlName = mapDebugMsg[2];
    wstring message = mapDebugMsg[3];
    wstring msgName = mapDebugMsg[4];
    wstring wParam = mapDebugMsg[5];
    wstring lParam = mapDebugMsg[6];
    wstring winprocName = mapDebugMsg[7];
    wstring cmd_NotifMsg = L"";                // This is for storing the sub-msg that is somtimes embedded in the HIWORD of the wParam of WM_COMMAND and WM_NOTIFY messages.
    int msg = std::stoi(message);
    long lparam = std::stol(lParam);
    int wparam = std::stoi(wParam);

    switch (msg)
    {
    case WM_COMMAND:
    {
        cmd_NotifMsg = L"Sub-Msg : ";
        switch (HIWORD(wparam))
        {
        case BN_DBLCLK:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_DBLCLK\n";
        }
        break;
        case BN_SETFOCUS:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_SETFOCUS\n";
        }
        break;;
        case BN_KILLFOCUS:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_KILLFOCUS\n";
        }
        break;;
        case BN_UNHILITE:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_UNHILITE\n";
        }
        break;
        case BN_DISABLE:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_DISABLE\n";
        }
        break;
        case BN_HILITE:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_HILITE\n";
        }
        break;
        case BN_PAINT:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_PAINT\n";
        }
        break;
        case BN_CLICKED:
        {
            cmd_NotifMsg = cmd_NotifMsg + L"BN_CLICKED\nControlID : ";
            switch (LOWORD(wparam))
            {
            case IDC_BUTTON1:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDC_BUTTON1\n";
            }
            break;
            case IDC_BUTTON2:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDC_BUTTON2\n";
            }
            break;
            case IDC_BUTTON3:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDC_BUTTON3\n";
            }
            break;
            case IDC_BUTTON4:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDC_BUTTON4\n";
            }
            break;
            case IDC_BUTTON5:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDC_BUTTON5\n";
            }
            break;
            case IDC_BUTTON6:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDC_BUTTON6\n";
            }
            break;
            case IDM_ABOUT:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDM_ABOUT\n";
            }
            break;
            case IDM_DEBUG_DETAILS:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDM_DEBUG_DETAILS\n";
            }
            break;
            case IDM_EXIT:
            {
                cmd_NotifMsg = cmd_NotifMsg + L"IDM_EXIT\n";
            }
            break;
            default:
                break;
            }
        }
        break;
        default:
            break;
        }
    }
        break;
    case WM_NOTIFY:
        break;
    default:  
        break;
    }

    // Most msgs will fall through to this code.  The exceptions are WM_COMMAND and WM_NOTIFY which contain sub-messages.  Those are handled earlier in this switch.
    StringCchPrintf(szDebugBuff, debugbufferSize,
        TEXT("timestamp : %s\nhwnd :  %s\ncontrol Name : %s\nmessage : %s\nmsg name : %s\nwParam : %s\nlParam %s\nwinproc name : %s\n\n=========================\n\n%s"),
        dateTime.c_str(), hwnd.c_str(), controlName.c_str(), message.c_str(), msgName.c_str(), wParam.c_str(), lParam.c_str(), winprocName.c_str(), cmd_NotifMsg.c_str());
    SetWindowText(GetDlgItem(hwndDebugDetailsDialog, IDC_STATIC_DEBUG_DETAILS), szDebugBuff);
}

std::wstring PadString(std::wstring srcString, char padChar, int numChars)
{
    std::wstring resultString = srcString;
    int srcStringLen = srcString.length();

    if (srcStringLen < numChars)
    {
        int numPadChars = numChars - srcStringLen;
        resultString.append(numPadChars, padChar);
    }
    return resultString;
}


