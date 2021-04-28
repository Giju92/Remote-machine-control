// Main.cpp : Defines the entry point for the application.

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdafx.h>
#include <res/TrayIcon.h>
#include <controller/Controller.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#pragma comment(lib, "IPHLPAPI.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

/* variables */
UINT WM_TASKBAR = 0;
HWND Hwnd;
HMENU Hmenu;
HFONT hFont;
NOTIFYICONDATA notifyIconData;
Controller *controller;
HDC hdcStatic;

HWND label;
HWND startButton;
HWND stopButton;
HWND hWndList;
HWND statusLabel;


/* procedures prototype definitions */
LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
void InitNotifyIconData(HWND Hwnd, HINSTANCE hInstance, TCHAR[]);
WNDCLASSEX initWindowClass(HINSTANCE, TCHAR[]);
void minimize();
void restore();
void addIPToList(HWND hWndList);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	WNDCLASSEX wincl;        /* Data structure for the windowclass */
	MSG messages;            /* Here messages to the application are saved */
	WM_TASKBAR = RegisterWindowMessageA("TaskbarCreated");

	//TODO put in another place the name definitions??
	TCHAR szClassName[] = TEXT("Server");
	TCHAR TIPMsg[64] = TEXT("Server");

	hFont = CreateFont(18, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));

	/* Initialize the window class and register it. In case of failure quit the program */
	wincl = initWindowClass(hInstance, szClassName);
	if (!RegisterClassEx(&wincl))
		return 0;
	
	/* Create the window */
	Hwnd = CreateWindowEx(
				0,                   /* Extended possibilites for variation */
				szClassName,         /* Classname */
				szClassName,		 /* Title Text */
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
				CW_USEDEFAULT,       /* Windows decides the position */
				CW_USEDEFAULT,       /* where the window ends up on the screen */
				500,                 /* The programs width */
				360,                 /* and height in pixels */
				HWND_DESKTOP,        /* The window is a child-window to desktop */
				nullptr,             /* No menu */
				hInstance,			 /* Program Instance handler */
				nullptr              /* No Window Creation data */
			);

	/* Initialize the NOTIFYICONDATA structure only once */
	InitNotifyIconData(Hwnd, hInstance, TIPMsg);

	/* Make the window visible on the screen */
	ShowWindow(Hwnd, nCmdShow);

	/* Run the message loop. It will run until GetMessage() returns 0 */
	while (GetMessage(&messages, nullptr, 0, 0))
	{
		/* Translate virtual-key messages into character messages */
		TranslateMessage(&messages);
		/* Send message to WindowProcedure */
		DispatchMessage(&messages);
	}

	return messages.wParam;
}

/* This function is called by the Windows function DispatchMessage() */
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_TASKBAR && !IsWindowVisible(Hwnd))
	{
		minimize();
		return 0;
	}

	/* handle the messages */
	switch (message)
	{
		case WM_ACTIVATE:
			/* Send a message to the taskbar's status area for adding the tray icon */
			Shell_NotifyIcon(NIM_ADD, &notifyIconData);
			break;

		case WM_CTLCOLORSTATIC:
		{
			hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(0, 0, 0));
			SetBkColor(hdcStatic, RGB(255, 255, 255));
			return (LRESULT)GetStockObject(NULL_BRUSH);
		}

		case WM_CREATE:
		{
			ShowWindow(Hwnd, SW_HIDE);

			/* Create the tray icon's menu */
			Hmenu = CreatePopupMenu();
			AppendMenu(Hmenu, MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));

			statusLabel = CreateWindow(L"STATIC",
				L"",
				WS_CHILD | WS_VISIBLE | SS_LEFT,
				20, 275, 250, 40,
				hwnd,
				(HMENU)3,
				NULL,
				NULL);

			startButton = CreateWindow(TEXT("button"),
				TEXT("Start"),
				WS_VISIBLE | WS_CHILD | WS_BORDER,
				300, 280, 80, 25,
				hwnd,
				(HMENU)1,
				NULL, NULL);

			stopButton = CreateWindow(TEXT("button"),
				TEXT("Stop"),
				WS_VISIBLE | WS_CHILD | WS_DISABLED | WS_BORDER,
				390, 280, 80, 25,
				hwnd,
				(HMENU)2,
				NULL, NULL);

			label = CreateWindow(L"STATIC",
				L"List of available interfaces:",
				WS_CHILD | WS_VISIBLE | SS_LEFT,
				20, 15, 200, 25,
				hwnd,
				(HMENU)4,
				NULL,
				NULL);

			hWndList = CreateWindowEx(WS_EX_CLIENTEDGE, 
				TEXT("listbox"),
				L"", 
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_AUTOVSCROLL | LBS_NOSEL,
				20, 40, 450, 230,
				hwnd, 
				(HMENU)105, 
				NULL, 
				NULL);

			addIPToList(hWndList);

			// Set the font
			SendMessage(label, WM_SETFONT, WPARAM(hFont), TRUE);
			SendMessage(hWndList, WM_SETFONT, WPARAM(hFont), TRUE);
			SendMessage(startButton, WM_SETFONT, WPARAM(hFont), TRUE);
			SendMessage(statusLabel, WM_SETFONT, WPARAM(hFont), TRUE);
			SendMessage(stopButton, WM_SETFONT, WPARAM(hFont), TRUE);
		}
		break;

		case WM_COMMAND:
			if (LOWORD(wParam) == 1) {
				controller = new Controller(statusLabel, startButton, stopButton);
				controller->start();

				EnableWindow(startButton, FALSE);
				SetWindowText(statusLabel, L"Waiting for client connection...");
				ShowWindow(statusLabel, SW_HIDE);
				ShowWindow(statusLabel, SW_SHOW);
			}

			if (LOWORD(wParam) == 2) {
				EnableWindow(stopButton, FALSE);
				EnableWindow(startButton, TRUE);

				controller->stop();
				delete controller;
				controller = nullptr;

				SetWindowText(statusLabel, L"Connection closed");
				ShowWindow(statusLabel, SW_HIDE);
				ShowWindow(statusLabel, SW_SHOW);
			}
			break;

		case WM_SYSCOMMAND:
			/* This event is fired by the top right buttons of the window (close, restore, minimize).
			In WM_SYSCOMMAND messages, the four low-order bits of the wParam parameter
			are used internally by the system. To obtain the correct result when testing the value of wParam,
			an application must combine the value 0xFFF0 with the wParam value by using the bitwise AND operator.*/
			switch (LOWORD(wParam))
			{
				case SC_MINIMIZE:
					minimize();
					return 0;

				case SC_CLOSE:
					/* Send a message to the taskbar's status area for removing the tray icon */
					Shell_NotifyIcon(NIM_DELETE, &notifyIconData);

					/* Send to the system a request to quit the application */
					PostQuitMessage(0);
					return 0;
			}
			break;

		// Our user defined WM_SYSICON message.
		case WM_SYSICON:
			{
				switch (wParam)
				{
					case ID_TRAY_APP_ICON:
						SetForegroundWindow(Hwnd);
						break;
				}

				if (lParam == WM_LBUTTONUP)
				{
					/* Click on the tray icon with left button of the mouse */
					restore();
				}
				else if (lParam == WM_RBUTTONDOWN)
				{
					/* Click on the tray icon with right button of the mouse */

					/* Get current mouse position */
					POINT curPoint;
					GetCursorPos(&curPoint);
					SetForegroundWindow(Hwnd);

					/* Show the menu and wait for the pressed button */
					UINT clicked = TrackPopupMenu(Hmenu, TPM_RETURNCMD | TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hwnd, nullptr);
					
					/* send benign message to window to make sure the menu goes away */
					SendMessage(hwnd, WM_NULL, 0, 0);

					/* Check the pressed button ID */
					if (clicked == ID_TRAY_EXIT)
					{
						/* Send a message to the taskbar's status area for removing the tray icon */
						Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
						
						/* Send to the system a request to quit the application */
						PostQuitMessage(0);
					}
				}
			}
			break;

		// intercept the hittest message..
		case WM_NCHITTEST:
			{
				UINT uHitTest = DefWindowProc(hwnd, WM_NCHITTEST, wParam, lParam);
				if (uHitTest == HTCLIENT)
					return HTCAPTION;
				
				return uHitTest;
			}

		case WM_CLOSE:
			minimize();
			return 0;

		case WM_DESTROY:
			// Perform cleanup tasks
			if (controller != nullptr)
			{
				controller->stop();
				delete controller;
			}

			/* Send to the system a request to quit the application */
			PostQuitMessage(0);
			break;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

void InitNotifyIconData(HWND Hwnd, HINSTANCE hInstance, TCHAR TIPMsg[])
{
	memset(&notifyIconData, 0, sizeof(NOTIFYICONDATA));

	notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	notifyIconData.hWnd = Hwnd; // set the window handler to which the tray icon's messeges are sent
	notifyIconData.uID = ID_TRAY_APP_ICON; // set the ID of the created tray icon
	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP; // mark the valid fields of the structure
	notifyIconData.uCallbackMessage = WM_SYSICON; // Set up our Windows Message
	notifyIconData.hIcon = (HICON)LoadIcon(hInstance, MAKEINTRESOURCE(ICO1)); // read the icon from the resource and set it
	_tcscpy_s(notifyIconData.szTip, _countof(notifyIconData.szTip), TIPMsg); // set the tip message
}

//
//  FUNCTION: initWindowClass()
//
//  PURPOSE: Registers the window class.
//
WNDCLASSEX initWindowClass(HINSTANCE hInstance, TCHAR szClassName[])
{
	WNDCLASSEX wincl;

	/* The Window structure */
	wincl.hInstance = hInstance;
	wincl.lpszClassName = szClassName;
	wincl.lpfnWndProc = WindowProcedure;      /* This function is called by the OS */
	wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
	wincl.cbSize = sizeof(WNDCLASSEX);

	/* Use default icon and mouse-pointer */
	wincl.hIcon = LoadIcon(wincl.hInstance, MAKEINTRESOURCE(ICO0));
	wincl.hIconSm = LoadIcon(wincl.hInstance, MAKEINTRESOURCE(ICO2));
	wincl.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wincl.lpszMenuName = nullptr;              /* No menu */
	wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
	wincl.cbWndExtra = 0;                      /* structure or the window instance */
	wincl.hbrBackground = (HBRUSH)(CreateSolidBrush(RGB(255, 255, 255)));
	
	return wincl;
}

void minimize()
{
	// hide the main window
	ShowWindow(Hwnd, SW_HIDE);
}

void restore()
{
	ShowWindow(Hwnd, SW_SHOW);
}

void addIPToList(HWND hWndList)
{
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	UINT i;


	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(sizeof(IP_ADAPTER_INFO));
	if (pAdapterInfo == NULL) {
		return;
	}

	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		FREE(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(ulOutBufLen);
		if (pAdapterInfo == NULL) {
			return;
		}
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		pAdapter = pAdapterInfo;

		while (pAdapter)
		{
			// Select only Ethernet and Wi-Fi adapters
			if (pAdapter->Type == MIB_IF_TYPE_ETHERNET || pAdapter->Type == IF_TYPE_IEEE80211)
			{
				// Get adpater name and convert to wchar_t*
				size_t origsize = strlen(pAdapter->Description) + 1;
				size_t convertedChars = 0;
				wchar_t adapterName[200];
				mbstowcs_s(&convertedChars, adapterName, origsize, pAdapter->Description, _TRUNCATE);

				// Get adapter IP address and convert to wchar_t*
				std::string ip = (pAdapter->IpAddressList.IpAddress.String);
				origsize = ip.length() + 1;
				wchar_t wcharIP[30];
				mbstowcs_s(&convertedChars, wcharIP, origsize, ip.c_str(), _TRUNCATE);

				// Add the entry to the listBox
				SendMessage(hWndList, LB_ADDSTRING, 0, (LPARAM)adapterName);
				SendMessage(hWndList, LB_ADDSTRING, 0, (LPARAM)wcharIP);
				SendMessage(hWndList, LB_ADDSTRING, 0, (LPARAM)L"");
			}

			pAdapter = pAdapter->Next;
		}
	}
	else {
		return;
	}

	if (pAdapterInfo)
		FREE(pAdapterInfo);
}
