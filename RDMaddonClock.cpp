// RDMaddonClock.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "RDMaddonClock.h"

//#define MYDEBUG
//#define USE_TRANSPARENCY

#ifdef USE_TRANSPARENCY
	extern "C" BOOL SetLayeredWindowAttributes(
	  HWND     hwnd,
	  COLORREF crKey,
	  BYTE     bAlpha,
	  DWORD    dwFlags
	);

	#define LWA_ALPHA	0x00000002
	#define LWA_COLORKEY	0x00000001
	#define WS_EX_LAYERED	0x00080000
	#define WS_EX_TRANSPARENT	0x00000020L
#endif

SYSTEM_POWER_STATUS_EX pwrStatus;

//window position
DWORD screenX=0;
DWORD screenY=0;
DWORD dwLeft=100;
DWORD dwTop=100;
DWORD blockWidth=40;
DWORD blockHeight=40;
DWORD blockCount=1;
DWORD blockMargin=1;
// main window size, 
//	width = blockwidth+2*blockMargin
//	height = blockCount*blockHeight+blockCount*blockMargin*2-2*blockMargin
//======================================================================
DWORD FLOATWIDTH	=	16; //100 //200                     // Width of floating wnd
DWORD FLOATHEIGHT	=	80; //100 //100                     // Height of floating wnd
//======================================================================
// 
// vars for the drawings 
// 
//colors
unsigned long colorText=RGB(0,255,0);

unsigned long colorWinBackground=RGB(0,0,0);
static HBRUSH hBackground = CreateSolidBrush( colorWinBackground );

DWORD dwTimerID=1000;
UINT hTimer=NULL;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE			g_hInst;			// current instance
//HWND				g_hWndCommandBar;	// command bar handle
HWND g_hWnd=NULL;

// Forward declarations of functions included in this code module:
ATOM			MyRegisterClass(HINSTANCE, LPTSTR);
BOOL			InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

SIZE getTextSizeExtent(HWND hwnd, TCHAR* fontName, int FontSize, TCHAR* text){
	HDC hdc=GetDC(hwnd);
	PLOGFONT plf;
	HFONT hfnt, hfntPrev;
	plf = (PLOGFONT) LocalAlloc(LPTR, sizeof(LOGFONT));
	// Specify a font typeface name and weight.
	lstrcpy(plf->lfFaceName, fontName);
	plf->lfHeight = -((FontSize * GetDeviceCaps(hdc, LOGPIXELSY)) / 72);//12;
	plf->lfWeight = FW_BOLD;
    plf->lfEscapement = 90*10;
	plf->lfOrientation = 90*10;
	plf->lfPitchAndFamily=FIXED_PITCH;
	SetTextAlign(hdc, TA_BASELINE);
	hfnt = CreateFontIndirect(plf);
	hfntPrev = (HFONT)SelectObject(hdc, hfnt);
	SIZE textSize;
	//calc textbox
	GetTextExtentPoint32(hdc, text, wcslen(text), &textSize); 
	
	TEXTMETRIC tm;
	GetTextMetrics(hdc, &tm);
	
	textSize.cy-= tm.tmDescent;

	SelectObject(hdc, hfntPrev);
	DeleteObject(hfnt);
	ReleaseDC(hwnd,hdc);
	return textSize;
}


int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR    lpCmdLine,
                   int       nCmdShow)
{
	MSG msg;

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow)) 
	{
		//Do not end before quit message has been received
		//return FALSE;
	}
	else{
		HACCEL hAccelTable;
		hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RDMADDONBATT2));

		// Main message loop:
		while (GetMessage(&msg, NULL, 0, 0)) 
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
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
//  COMMENTS:
//
ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE | CS_DBLCLKS;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RDMADDONBATT2));
	wc.hCursor       = 0;
	wc.hbrBackground = hBackground;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClass;

	return RegisterClass(&wc);
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
    HWND hWnd;
    TCHAR szTitle[MAX_LOADSTRING];		// title bar text
    TCHAR szWindowClass[MAX_LOADSTRING];	// main window class name

    g_hInst = hInstance; // Store instance handle in our global variable


    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING); 
    LoadString(hInstance, IDC_RDMADDONBATT2, szWindowClass, MAX_LOADSTRING);


    if (!MyRegisterClass(hInstance, szWindowClass))
    {
    	return FALSE;
    }

	HWND hWndTS = FindWindow(L"TSSHELLWND", NULL);
#ifndef MYDEBUG
	if(hWndTS==NULL){
		DEBUGMSG(1, (L"### TSSHELLWND not found. EXIT. ###\n"));
		return FALSE;
	}
#else
		hWndTS=GetForegroundWindow();
		DEBUGMSG(1, (L"### using foregroundwindow ###\n"));
#endif

    //hWnd = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
    //    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

	hWnd = CreateWindowEx(
#ifdef USE_TRANSPARENCY
		WS_EX_LAYERED | WS_EX_TRANSPARENT,
#else
		0, //WS_EX_TOPMOST | WS_EX_ABOVESTARTUP, 
#endif
		szWindowClass, 
		NULL /* szTitle */, // NO CAPTION
		WS_VISIBLE, //WS_VISIBLE | WS_EX_ABOVESTARTUP, // | WS_EX_TOOLWINDOW | WS_CHILD | WS_POPUP | WS_NONAVDONEBUTTON, 
		0, 
		0, 
		FLOATWIDTH, 
		FLOATHEIGHT, 
		hWndTS, // NULL, 
		NULL, 
		hInstance, 
		NULL);

#ifdef USE_TRANSPARENCY
	SetLayeredWindowAttributes(hWnd, 0, (BYTE)(128), LWA_ALPHA);
#endif    
	if (!hWnd)
    {
        return FALSE;
    }

	g_hWnd=hWnd;

	LONG lStyle = GetWindowLong(hWnd, GWL_STYLE);
	DEBUGMSG(1, (L"GetWindowLong GWL_SYTLE=%08x\n", lStyle));

	screenX = GetSystemMetrics(SM_CXSCREEN);
	screenY = GetSystemMetrics(SM_CYSCREEN);
	DWORD dwX, dwY, dwW, dwH;

	//position
	//calc textbox
	SIZE textSize=getTextSizeExtent(hWnd, L"Courier", 8, L"00:00");
	dwW=textSize.cy;
	dwH=textSize.cx+2*blockMargin;

	dwX=screenX-dwW;// FLOATWIDTH;
	dwY=0;//screenY/2;//-FLOATHEIGHT;
	//size
	//dwW=blockWidth+2*blockMargin;// + GetSystemMetrics(SM_CXEDGE) * 2;// FLOATWIDTH;
	//dwH=blockCount*blockHeight + blockCount*blockMargin + blockMargin;// + GetSystemMetrics(SM_CYEDGE) * 2;//FLOATHEIGHT;
	//MoveWindow(hWnd, dwX, dwY, dwW, dwH, TRUE);

	SetWindowPos(hWnd, HWND_TOPMOST, dwX, dwY, dwW, dwH, SWP_SHOWWINDOW);

	//move the window
	//SetWindowPos(hWnd, HWND_TOPMOST, dwLeft, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);


    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
	RECT rect;
	static HPEN hPen;
	int iRes=0;
	HGDIOBJ clrPrev;

	HFONT hfnt, hfntPrev; PLOGFONT plf;
	TCHAR myText[6]; SYSTEMTIME myLocalTime;
	COLORREF myTextColor = colorText;
	static BOOL bToggle;

    switch (message) 
    {
        case WM_COMMAND:
            wmId    = LOWORD(wParam); 
            wmEvent = HIWORD(wParam); 
            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_HELP_ABOUT:
                    DialogBox(g_hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, About);
                    break;
                case IDM_FILE_EXIT:
                    DestroyWindow(hWnd);
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        case WM_CREATE:

			hTimer=SetTimer(hWnd, dwTimerID, 1000, NULL);
            break;
		case WM_TIMER:
			if(wParam==dwTimerID){
#ifndef MYDEBUG
				if(FindWindow(L"TSSHELLWND", NULL)==NULL)
					PostQuitMessage(-2);
#endif
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, TRUE);
			}
			break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            
            // TODO: Add any drawing code here...
			GetLocalTime(&myLocalTime);
			if(bToggle)
				wsprintf(myText, L"%02i:%02i", myLocalTime.wHour, myLocalTime.wMinute);
			else
				wsprintf(myText, L"%02i %02i", myLocalTime.wHour, myLocalTime.wMinute);
			bToggle=!bToggle;

            GetClientRect(hWnd, &rect);
			
			iRes=FillRect(hdc, &rect, hBackground);			
			plf = (PLOGFONT) LocalAlloc(LPTR, sizeof(LOGFONT));
//			plf=(PLOGFONT)SelectObject(hdc, GetStockObject(SYSTEM_FONT));

			// Specify a font typeface name and weight.
//			WCHAR lfFaceName[20] = L"Arial";
			lstrcpy(plf->lfFaceName, L"Courier");
			plf->lfHeight = -((8 * GetDeviceCaps(hdc, LOGPIXELSY)) / 72);//12;
			plf->lfWeight = FW_BOLD;
		    plf->lfEscapement = 90*10;
			plf->lfOrientation = 90*10;
			plf->lfPitchAndFamily=FIXED_PITCH;
			hfnt = CreateFontIndirect(plf);
			hfntPrev = (HFONT)SelectObject(hdc, hfnt);

			SetTextAlign(hdc, TA_BASELINE);

			SetBkColor(hdc, RGB(0,0,0));
			//SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, myTextColor);
			
			//SetTextAlign(hdc, TA_CENTER);
			if(!ExtTextOut(hdc, rect.right - 2, rect.bottom , ETO_OPAQUE, &rect, myText, wcslen(myText), NULL))
				DEBUGMSG(1, (L"ExtTextOut failed: %i\n", GetLastError()));

			SelectObject(hdc, hfntPrev);
			SelectObject(hdc, clrPrev);
			DeleteObject(hfnt);
            EndPaint(hWnd, &ps);
#ifdef USE_TRANSPARENCY
			SetLayeredWindowAttributes(hWnd, 0, (BYTE)(180), LWA_ALPHA);
#endif    
            break;
		case WM_LBUTTONDBLCLK:
			if(MessageBox(hWnd, L"Exit?", L"Battery Monitor", MB_YESNO|MB_ICONQUESTION)==IDYES)
				PostQuitMessage(-1);
			break;
        case WM_DESTROY:
            PostQuitMessage(0);
			DeleteObject(hPen);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            RECT rectChild, rectParent;
            int DlgWidth, DlgHeight;	// dialog width and height in pixel units
            int NewPosX, NewPosY;

            // trying to center the About dialog
            if (GetWindowRect(hDlg, &rectChild)) 
            {
                GetClientRect(GetParent(hDlg), &rectParent);
                DlgWidth	= rectChild.right - rectChild.left;
                DlgHeight	= rectChild.bottom - rectChild.top ;
                NewPosX		= (rectParent.right - rectParent.left - DlgWidth) / 2;
                NewPosY		= (rectParent.bottom - rectParent.top - DlgHeight) / 2;
				
                // if the About box is larger than the physical screen 
                if (NewPosX < 0) NewPosX = 0;
                if (NewPosY < 0) NewPosY = 0;
                SetWindowPos(hDlg, 0, NewPosX, NewPosY,
                    0, 0, SWP_NOZORDER | SWP_NOSIZE);
            }
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, message);
            return TRUE;

    }
    return (INT_PTR)FALSE;
}
