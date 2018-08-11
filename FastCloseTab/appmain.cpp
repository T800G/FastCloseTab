#include "stdafx.h"
#include <shellapi.h>
#include "resource.h"
#include "kbdinput.h"

#define APPWNDCLASSNAME  _T("FastCloseTab")
#define TRAYWNDCLASSNAME  _T("FastCloseTabTrayWnd")
#define WM_TRAY_NOTIFY (WM_APP+11)
#define WM_BROWSER_SHOW (WM_APP+12)
#define WM_BROWSER_HIDE (WM_APP+13)

// Globals
HWND g_hMainWnd;
HWND g_hButtonWnd;
HWINEVENTHOOK  g_hEventHook;
TCHAR g_szInfo[256];
HBITMAP g_hBtnBmp;
HICON g_hBtnIco;

#define WEXTRA  2

#define BWC_FFOX 1
#define BWC_IE 2
#define BWC_CHROME 3

LRESULT CALLBACK appWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);//forward declaration

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

HICON CreateIconFromBitmap(HWND hWnd, HBITMAP hBmp, int w, int h)
{
	HICON hico=NULL;
	if (!hWnd || !IsWindow(hWnd)) return NULL;
	HDC hWndDC = GetDC(hWnd);
	if (!hWndDC) return NULL;
    HDC hMemDC = CreateCompatibleDC(hWndDC);
    HDC hMemDC2 = CreateCompatibleDC(hWndDC);
	if (hMemDC && hMemDC2)
	{
		int dL = (w > h) ? w : h;
		int dS = (w < h) ? w : h;

		ICONINFO ii;
		ii.fIcon = TRUE;
		ii.hbmMask = CreateCompatibleBitmap(hWndDC, dL,dL);
		ii.hbmColor = CreateCompatibleBitmap(hWndDC, dL,dL);
		if (ii.hbmMask && ii.hbmColor)
		{
			HBITMAP hBmpOld, hBmpOld2;
			RECT rc;
			//fill mask with black
			rc.left=rc.top=0;
			rc.right=rc.bottom=dL;
			hBmpOld= (HBITMAP)SelectObject(hMemDC, ii.hbmMask);
			FillRect(hMemDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
			ii.hbmMask = (HBITMAP)SelectObject(hMemDC, hBmpOld);

			hBmpOld = (HBITMAP)SelectObject(hMemDC, ii.hbmColor);
			hBmpOld2 = (HBITMAP)SelectObject(hMemDC2, hBmp);
			HBRUSH bkgBrush = CreateSolidBrush(GetPixel(hMemDC2, 0,0));
			if (bkgBrush)
			{
				rc.left=rc.top=0;
				rc.right=rc.bottom=dL;
				FillRect(hMemDC, &rc, bkgBrush);
				DeleteObject(bkgBrush);
			
				BITMAP bm;
				SecureZeroMemory(&bm, sizeof(BITMAP));
				if (GetObject(hBmp, sizeof(BITMAP), &bm))
				{
					SetStretchBltMode(hMemDC, HALFTONE);
					SetBrushOrgEx(hMemDC2, (dL-dS)/2, (dL-dS)/2, NULL);
					StretchBlt(hMemDC, (dL-dS)/2, (dL-dS)/2, dS,dS, hMemDC2, 0,0, bm.bmWidth, bm.bmHeight, SRCCOPY);
				}
			}

			hBmp = (HBITMAP)SelectObject(hMemDC2, hBmpOld2);
			ii.hbmColor = (HBITMAP)SelectObject(hMemDC, hBmpOld);

			hico = ::CreateIconIndirect(&ii);

		}
		DeleteObject(ii.hbmColor);
		DeleteObject(ii.hbmMask);

		DeleteDC(hMemDC);
		DeleteDC(hMemDC2);
	}
	ReleaseDC(hWnd, hWndDC);

return hico;
}

////////////////////////////////////////////////////////////////////////////////
BOOL IsBrowserWindow(HWND hWnd)
{
	if (!IsWindow(hWnd)) return FALSE;
	if (GetWindowLong(hWnd ,GWL_STYLE) & WS_OVERLAPPEDWINDOW)
	if (GetClassName(hWnd, g_szInfo, _countof(g_szInfo))>0)
	{
		if (lstrcmp(g_szInfo, _T("MozillaWindowClass"))==0) return BWC_FFOX;
		if (lstrcmp(g_szInfo, _T("IEFrame"))==0) return BWC_IE;	
		if (lstrcmp(g_szInfo, _T("Chrome_WidgetWin_1"))==0) return BWC_CHROME;	
	}
return 0;
}
/////////////////////////////////////////////////
BOOL IsSameProcessWnd(HWND hwnd1, HWND hwnd2)
{
	DWORD pid1, pid2;
	DWORD tid1 = GetWindowThreadProcessId(hwnd1, &pid1);
	DWORD tid2 = GetWindowThreadProcessId(hwnd2, &pid2);
	if (tid1 == tid1 && pid1 == pid2) return TRUE;
return FALSE;
}
/////////////////////////////////////////////////
void ShowTrayMenu(HWND hWnd, LONG resID)
{
	POINT mousepos;
	if (!GetCursorPos(&mousepos)) return;
	HMENU htrayMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(resID));
	if (htrayMenu)
	{
		SetForegroundWindow(hWnd);
		TrackPopupMenuEx(GetSubMenu(htrayMenu,0), TPM_RIGHTALIGN, mousepos.x, mousepos.y, hWnd, NULL);
		DestroyMenu(htrayMenu);
		PostMessage(hWnd, WM_NULL, 0, 0);
	}
}
////////////////////////////////////////////////////////////////////////////////
void CALLBACK WinEventProcCallback(HWINEVENTHOOK hook, DWORD dwEvent, HWND hwnd, LONG idObject,
								   LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	if (idObject == OBJID_WINDOW)
	{
		if ((dwEvent == EVENT_OBJECT_DESTROY) && IsSameProcessWnd(g_hMainWnd, hwnd)) SetParent(g_hMainWnd, HWND_DESKTOP);

		if ((idChild == CHILDID_SELF) && (GetParent(hwnd) == NULL)) //only top level windows
			switch (dwEvent)
			{
				case EVENT_SYSTEM_FOREGROUND:
					if (!IsWindowVisible(hwnd)) return; //event is fired multiple times until window is visible
					if (IsBrowserWindow(hwnd))
					{
						//connect to browser window
						SetParent(g_hMainWnd, hwnd);
						PostMessage(g_hMainWnd, WM_BROWSER_SHOW, (WPARAM)hwnd, 0);
					}
				break;
				case EVENT_OBJECT_LOCATIONCHANGE:
					if (IsBrowserWindow(hwnd)) PostMessage(g_hMainWnd, WM_BROWSER_SHOW, (WPARAM)hwnd, 0);
				break;
				case EVENT_OBJECT_HIDE: //disconnect from browser window
					if (GetParent(g_hMainWnd) == hwnd)
					{
						SetParent(g_hMainWnd, HWND_DESKTOP);
						ShowWindow(g_hMainWnd, SW_HIDE);
					}
				break;
			}
	}
}
//////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK trayWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_EXIT:
					g_hButtonWnd=NULL;
					DestroyWindow(g_hMainWnd);
					DestroyWindow(hWnd);
				break;
			}
		break;
		case WM_TRAY_NOTIFY:
			if (LOWORD(lParam) == WM_RBUTTONUP) ShowTrayMenu(hWnd, IDM_TRAY);
		break;
		case WM_DESTROY: PostQuitMessage(0); break;
		default: return DefWindowProc(hWnd, message, wParam, lParam);
	}
return 0;
}
//////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK mainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
			g_hButtonWnd=CreateWindow(TEXT("BUTTON"), TEXT("T"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_ICON,
									 -1*GetSystemMetrics(SM_CXEDGE),
									 -1*GetSystemMetrics(SM_CYEDGE),
									 ((LPCREATESTRUCT)lParam)->cx +2*GetSystemMetrics(SM_CXEDGE),
									 ((LPCREATESTRUCT)lParam)->cy +2*GetSystemMetrics(SM_CYEDGE),
									 hWnd, (HMENU)IDC_CLOSETAB, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
			if (g_hButtonWnd==NULL) return (LRESULT)-1; //abort
		break;
		case WM_THEMECHANGED:
		case WM_SETTINGCHANGE:
			DestroyWindow(hWnd);//recreate main wnd
		break;
		case WM_BROWSER_SHOW:
			{
				RECT rc;
				if (GetWindowRect((HWND)wParam, &rc))
				{
					TITLEBARINFOEX tbi;
					SecureZeroMemory(&tbi, sizeof(TITLEBARINFOEX));
					tbi.cbSize=sizeof(TITLEBARINFOEX);
//cross-process/thread call! (this is not safe)
					SendMessage((HWND)wParam, WM_GETTITLEBARINFOEX, 0, (LPARAM)&tbi);
					if (!IsRectEmpty(&tbi.rgrect[5])) //close button
					{
						if (g_hBtnIco!=NULL) DestroyIcon(g_hBtnIco);
						g_hBtnIco = CreateIconFromBitmap(g_hButtonWnd, g_hBtnBmp,
															tbi.rgrect[5].right - tbi.rgrect[5].left +2*GetSystemMetrics(SM_CXEDGE),
															tbi.rgrect[5].bottom - tbi.rgrect[5].top +2*GetSystemMetrics(SM_CYEDGE));
						if (g_hBtnIco) SendMessage(g_hButtonWnd, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)g_hBtnIco);


						MapWindowPoints(GetParent(hWnd),(HWND)wParam, (LPPOINT)&tbi.rgrect[5], 2);
//>>>check DPI rc offset (+2?)
						SetWindowPos(hWnd, HWND_TOP,
										tbi.rgrect[5].left,
										tbi.rgrect[5].top,
										tbi.rgrect[5].right - tbi.rgrect[5].left +GetSystemMetrics(SM_CXEDGE), //.right is just outside of rect //aero metrics?
										tbi.rgrect[5].bottom - tbi.rgrect[5].top,
										SWP_SHOWWINDOW | SWP_NOACTIVATE);

						SetWindowPos(g_hButtonWnd, NULL,
										-1*GetSystemMetrics(SM_CXEDGE),
										-1*GetSystemMetrics(SM_CYEDGE),
										tbi.rgrect[5].right - tbi.rgrect[5].left +2*GetSystemMetrics(SM_CXEDGE),
										tbi.rgrect[5].bottom - tbi.rgrect[5].top +2*GetSystemMetrics(SM_CYEDGE),
										NULL);
					}
				}
			}
		break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_CLOSETAB:
					if (IsBrowserWindow(GetForegroundWindow())) SendCtrlW();
				break;
			}
		break;
		case WM_TRAY_NOTIFY:
			if (LOWORD(lParam)==WM_RBUTTONUP) ShowTrayMenu(hWnd, IDM_TRAY);
		break;
		case WM_ENDSESSION: if (wParam==TRUE) {if (g_hEventHook) { UnhookWinEvent(g_hEventHook); g_hEventHook=NULL; } }; break;
		case WM_DESTROY: PostQuitMessage(0); break;
		default: return DefWindowProc(hWnd, message, wParam, lParam);
	}
return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	CoInitialize(NULL);

	WNDCLASSEX traywcex;
	SecureZeroMemory(&traywcex, sizeof(WNDCLASSEX));
	traywcex.cbSize = sizeof(WNDCLASSEX);
	traywcex.hInstance = hInstance;
	traywcex.lpfnWndProc = trayWndProc;
	traywcex.lpszClassName = TRAYWNDCLASSNAME;
	if (!RegisterClassEx(&traywcex)) goto FINALIZE;

	NOTIFYICONDATA trayicon;
	SecureZeroMemory(&trayicon, sizeof(NOTIFYICONDATA));
	trayicon.cbSize = sizeof(NOTIFYICONDATA);
	trayicon.uFlags = (NIF_ICON | NIF_MESSAGE | NIF_TIP);
	trayicon.dwInfoFlags = NIIF_USER;
	trayicon.uCallbackMessage = WM_TRAY_NOTIFY;
	trayicon.uTimeout = 3000;
	LoadString(hInstance, IDS_TRAYTOOLTIP, trayicon.szTip, _countof(trayicon.szTip));
	//we need hwnd to create icon from bitmap so we use resource icon
	trayicon.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_TRAY), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	if (!trayicon.hIcon) goto FINALIZE;

	g_hBtnBmp=(HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_BTN), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	if (!g_hBtnBmp) goto FINALIZE;

	//create message-only window for tray icon messages
	trayicon.hWnd = CreateWindow(traywcex.lpszClassName, NULL, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, HWND_MESSAGE, NULL, traywcex.hInstance, NULL);
	if (!trayicon.hWnd) goto FINALIZE;

	WNDCLASSEX wcex;
	SecureZeroMemory(&wcex, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.hInstance = hInstance;
	wcex.lpfnWndProc = mainWndProc;
	wcex.lpszClassName = APPWNDCLASSNAME;
	if (!RegisterClassEx(&wcex)) goto FINALIZE;

RECREATEWND:
	g_hMainWnd = CreateWindowEx(WS_EX_NOACTIVATE | WS_EX_TOPMOST, APPWNDCLASSNAME, APPWNDCLASSNAME, WS_POPUP, 0,0, 33, 33, NULL, NULL, hInstance, NULL);
	if (g_hMainWnd)
	{
		Shell_NotifyIcon((g_hButtonWnd ? NIM_ADD : NIM_MODIFY), &trayicon);
		if (g_hEventHook==NULL)
			g_hEventHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_OBJECT_LOCATIONCHANGE, NULL, WinEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
		
		{ //is browser already visible?
			HWND hw=GetForegroundWindow();
			if (IsBrowserWindow(hw)) SendMessage(g_hMainWnd, WM_BROWSER_SHOW, (WPARAM)hw, 0);
		}
	
		if (g_hEventHook)
		{
			MSG msg;
			while (GetMessage(&msg, NULL, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		//check if closing brower destroyed our window
		if (g_hButtonWnd!=NULL) goto RECREATEWND;

		if (g_hEventHook) UnhookWinEvent(g_hEventHook);
		Shell_NotifyIcon(NIM_DELETE, &trayicon);
		if (g_hBtnIco) DestroyIcon(g_hBtnIco);
		if (g_hBtnBmp) DeleteObject(g_hBtnBmp);
	}

FINALIZE:
	CoUninitialize();
return 0;
}
