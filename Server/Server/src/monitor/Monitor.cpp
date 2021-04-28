#include <monitor/Monitor.h>
#include <message/MsgTypes.h>
#include <Shellapi.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Dwmapi.lib")


// Function prototype
std::string MBFromW(LPCWSTR pwsz);

// Global pointer that points to the class itself
Monitor *m;


Monitor::Monitor()
{
	m = this;
}

Monitor::Monitor(std::shared_ptr<BlockingQueue<Message>> msgQueue)
{
	//this->msgQueue = msgQueue;
	m = this;
}

void Monitor::setMesgQueue(std::shared_ptr<BlockingQueue<Message>> msgQueue)
{
	this->msgQueue = msgQueue;
}

void Monitor::start()
{
	// Retrieve current active processes
	retrieveActiveProcesses();

	// Get the window that has currently the focus
	focusedWnd = GetForegroundWindow();

	// Register the hook for new window event
	hookWndCreate = SetWinEventHook(
							EVENT_OBJECT_SHOW,
							EVENT_OBJECT_SHOW,
							NULL,
							MngCreation,
							0,
							0,
							WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNTHREAD //| WINEVENT_SKIPOWNPROCESS
						);

	// Register the hook for close window event
	hookWndClose = SetWinEventHook(
							EVENT_OBJECT_DESTROY,
							EVENT_OBJECT_DESTROY,
							NULL,
							MngDestroy,
							0,
							0,
							WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS | WINEVENT_SKIPOWNTHREAD
						);

	// Register the hook for focus change event
	hookFocusChange = SetWinEventHook(
							EVENT_SYSTEM_FOREGROUND,
							EVENT_SYSTEM_FOREGROUND,
							NULL,
							MngFocusChg,
							0,
							0,
							WINEVENT_OUTOFCONTEXT //| WINEVENT_SKIPOWNTHREAD//| WINEVENT_SKIPOWNPROCESS
						);
}

void Monitor::stop()
{
	// Unregister the hook for new window event
	UnhookWinEvent(hookWndCreate);

	// Unregister the hook for close window event
	UnhookWinEvent(hookWndClose);

	// Unregister the hook for focus change events
	UnhookWinEvent(hookFocusChange);

	// Empty the process list
	{
		// Get the lock in order to guarantee a safe access to the process list
		std::lock_guard<std::mutex> l(processList_mutex);
		processList.clear();
	}
}

void Monitor::postProcessList()
{
	std::vector<Process> list;

	// Retrieve the list of processes
	{
		// Protect the shared access to the list of process
		std::lock_guard<std::mutex> l(processList_mutex);

		for (std::map<int, Process>::iterator it = processList.begin(); it != processList.end(); ++it)
		{
			list.push_back(it->second);
		}
	}

	// Get the window that has currently the focus
	{
		std::lock_guard<std::mutex> lg(focusedWdn_mutex);
		focusedWnd = GetForegroundWindow();

		// Post the process list message and the current focused process
		Message processListMsg(PROCESS_LIST, reinterpret_cast<int>(focusedWnd), list);
		msgQueue->put(processListMsg);

		// Post the focus change message
		Message focusChangeMsg(FOCUS_CHANGE, reinterpret_cast<int>(focusedWnd));
		msgQueue->put(focusChangeMsg);
	}
}

void  Monitor::changeFocusedWindow(HWND hwnd)
{
	// Attach foreground window thread to our thread
	AttachThreadInput(GetWindowThreadProcessId(::GetForegroundWindow(), NULL), GetCurrentThreadId(), TRUE);

	// Do our stuff here ;-)
	BringWindowToTop(hwnd);
	ShowWindow(hwnd, SW_RESTORE);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd); //Just playing safe

	//Detach the attached thread
	AttachThreadInput(GetWindowThreadProcessId(::GetForegroundWindow(), NULL), GetCurrentThreadId(), FALSE);
}

void Monitor::retrieveActiveProcesses()
{
	EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(this));
}

int Monitor::getProcessPath(DWORD pid, LPWSTR lpExeName, DWORD *size)
{
	HANDLE hProcess;


	// Open the process exe file
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	if (hProcess == NULL)
		return -1;

	// Get the full path of the exe file
	QueryFullProcessImageName(hProcess, 0, lpExeName, size);

	// Close the process exe file
	if (CloseHandle(hProcess) == FALSE)
		return -1;

	return 0;
}

HICON Monitor::getHICONFromHWND(HWND hWnd)
{
	// Get the window icon
	HICON hIcon = (HICON)(SendMessageW(hWnd, WM_GETICON, ICON_BIG, 0));
	if (hIcon == 0) {
		// Alternative method. Get from the window class
		hIcon = reinterpret_cast<HICON>(GetClassLongPtrW(hWnd, GCLP_HICONSM));
	}
	// Alternative: get the first icon from the main module 
	if (hIcon == 0) {
		hIcon = LoadIcon(GetModuleHandleW(0), MAKEINTRESOURCE(0));
	}
	// Alternative method. Use OS default icon
	if (hIcon == 0) {
		hIcon = LoadIcon(0, IDI_APPLICATION);
	}

	return hIcon;
}

HICON Monitor::getHICONFromProcessPath(LPWSTR lpExeName)
{
	HANDLE hProcess;
	HICON largeIcon;

	// Extract the icon from the exe file
	int res = ExtractIconEx((LPCTSTR)lpExeName, 0, &largeIcon, NULL, 1);
	if (res == 0)
		return NULL;

	return largeIcon;
}

BOOL CALLBACK Monitor::enumWindowsProc(HWND hWnd, LPARAM lParam)
{
	return m->_enumWindowsProc(hWnd);
}

BOOL Monitor::_enumWindowsProc(HWND hWnd)
{
	DWORD processId;
	LPWSTR lpExeName = new WCHAR[MAX_PATH];
	DWORD size = MAX_PATH;
	HICON hIcon;
	int length;

	// Check if the window has to be considered or not
	if (!isAltTabWindow(hWnd))
		return TRUE;

	// Get the window text length
	length = GetWindowTextLength(hWnd);

	// Get the window name
	std::string text(length, '\0');
	GetWindowTextA(hWnd, &text[0], length + 1);

	// Get the process Id
	GetWindowThreadProcessId(hWnd, &processId);

	// Create the process
	Process p(processId, hWnd, text);

	// Retrieve the full path of the process, convert it to string and set to the process
	if (getProcessPath(processId, lpExeName, &size) < 0)
		return NULL;

	std::string processPath = MBFromW(lpExeName);
	p.setFullPath(processPath);

	// Retrieve the HICON associated to the created window and set it to the converter
	hIcon = getHICONFromProcessPath(lpExeName);
	if (hIcon == NULL)
		hIcon = getHICONFromHWND(hWnd);
	
	iconConverter.setHICON(hIcon);
	
	// Get the icon size and allocate a buffer of bytes
	size_t nBytes = iconConverter.getIconSize();
	BYTE *buffer = new BYTE[nBytes];

	// Retrieve the icon bytes
	iconConverter.getIconBytes(buffer, nBytes);
	p.setIconBytes(buffer, nBytes);

	// Reset the image converter and deallocate the buffer
	iconConverter.reset();
	delete[] buffer;

	// Add the process to the map
	{
		std::lock_guard<std::mutex> l(processList_mutex);
		processList.insert(std::pair<int, Process>(reinterpret_cast<int>(hWnd), p));
	}

	return TRUE;
}

BOOL Monitor::isAltTabWindow(HWND hwnd)
{
	TITLEBARINFO ti;
	HWND hwndTry, hwndWalk = NULL;

	// Skip invisible windows
	if (!IsWindowVisible(hwnd))
		return FALSE;

	// Look for the root window of the hwnd
	hwndTry = GetAncestor(hwnd, GA_ROOTOWNER);
	while (hwndTry != hwndWalk)
	{
		hwndWalk = hwndTry;
		hwndTry = GetLastActivePopup(hwndWalk);
		if (IsWindowVisible(hwndTry))
			break;
	}

	// Check if the root window is equal to the hwnd. If yes consider the window otherwise skip it
	if (hwndWalk != hwnd)
		return FALSE;

	// Removes some task tray programs and "Program Manager"
	ti.cbSize = sizeof(ti);
	GetTitleBarInfo(hwnd, &ti);
	if (ti.rgstate[0] & STATE_SYSTEM_INVISIBLE)
		return FALSE;

	// Remove tool windows and Windows 10 background apps
	if ((GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) && !(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_APPWINDOW))
		return FALSE;
	
	// Check if the window is a Windows 10 app running in background
	if (isWindows10BackgroudApp(hwnd))
		return FALSE;

	// Skip the windows without the title bar
	if (GetWindowTextLength(hwnd) == 0)
		return FALSE;

	return TRUE;
}

BOOL Monitor::isWindows10BackgroudApp(HWND hwnd)
{
	HRESULT hr = S_OK;
	DWMWINDOWATTRIBUTE status;

	hr = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &status, sizeof(BOOL));

	if (SUCCEEDED(hr))
	{
		if (status & DWMWA_CLOAKED)
			return TRUE;
	}

	return FALSE;
}

void CALLBACK Monitor::MngCreation(
	HWINEVENTHOOK hWinEventHook,
	DWORD         event,
	HWND          hwnd,
	LONG          idObject,
	LONG          idChild,
	DWORD         dwEventThread,
	DWORD         dwmsEventTime
)
{
	m->_MngCreation(hWinEventHook,
		event,
		hwnd,
		idObject,
		idChild,
		dwEventThread,
		dwmsEventTime);
}

void Monitor::_MngCreation(
	HWINEVENTHOOK hWinEventHook,
	DWORD         event,
	HWND          hwnd,
	LONG          idObject,
	LONG          idChild,
	DWORD         dwEventThread,
	DWORD         dwmsEventTime
)
{
	DWORD processID;
	LPWSTR lpExeName = new WCHAR[MAX_PATH];
	DWORD size = MAX_PATH;
	HICON hIcon;
	int length = 0;


	// Check if the window has to be considered or not
	if (!isAltTabWindow(hwnd))
		return;

	// Get the window text length
	length = GetWindowTextLength(hwnd);

	// Get the window name
	std::string text(length, '\0');
	GetWindowTextA(hwnd, &text[0], length + 1);

	// Get the ProcessID
	GetWindowThreadProcessId(hwnd, &processID);

	// Create the new process
	Process p(processID, hwnd, text);

	// Retrieve the full path of the process, convert it to string and set to the process
	if (getProcessPath(processID, lpExeName, &size) < 0)
		return;

	std::string processPath = MBFromW(lpExeName);
	p.setFullPath(processPath);

	// Retrieve the HICON associated to the created window and set it to the converter
	hIcon = getHICONFromProcessPath(lpExeName);
	if (hIcon == NULL)
		hIcon = getHICONFromHWND(hwnd);
	iconConverter.setHICON(hIcon);

	// Get the icon size and allocate a buffer of bytes
	size_t nBytes = iconConverter.getIconSize();
	BYTE *buffer = new BYTE[nBytes];

	// Retrieve the icon bytes
	iconConverter.getIconBytes(buffer, nBytes);
	p.setIconBytes(buffer, nBytes);

	// Reset the image converter and deallocate the buffer
	iconConverter.reset();
	delete[] buffer;

	{
		// Protect the shared access to the process list
		std::lock_guard<std::mutex> l(processList_mutex);

		/* Try to add the process to the map.
		* If it is successful send the new process message and focus change message
		* If it is not successfull skip the message sending because there is not a real
		* new process event.
		*/
		std::pair<std::map<int, Process>::iterator, bool> res;
		res = processList.insert(std::pair<int, Process>(reinterpret_cast<int>(hwnd), p));
		if (res.second == true)
		{
			// Update the current process that has the focus
			{
				std::lock_guard<std::mutex> lg(focusedWdn_mutex);
				m->focusedWnd = hwnd;
			}

			// Post the new process message
			Message newProcessMsg(NEW_PROCESS, p);
			m->msgQueue->put(newProcessMsg);
		}
	}
}

void CALLBACK Monitor::MngDestroy(
	HWINEVENTHOOK hWinEventHook,
	DWORD         event,
	HWND          hwnd,
	LONG          idObject,
	LONG          idChild,
	DWORD         dwEventThread,
	DWORD         dwmsEventTime
)
{
	m->_MngDestroy(hWinEventHook,
		event,
		hwnd,
		idObject,
		idChild,
		dwEventThread,
		dwmsEventTime);
}

void Monitor::_MngDestroy(
	HWINEVENTHOOK hWinEventHook,
	DWORD         event,
	HWND          hwnd,
	LONG          idObject,
	LONG          idChild,
	DWORD         dwEventThread,
	DWORD         dwmsEventTime
)
{
	if (hwnd && idObject == OBJID_WINDOW)
	{
		try
		{
			// Protect the shared access to the process list
			std::lock_guard<std::mutex> l(processList_mutex);

			// Try to remove the closed window if exists
			if (processList.erase(reinterpret_cast<int>(hwnd)) == 1)
			{
				// Post the close process message
				Message closeProcessMsg(PROCESS_CLOSE, reinterpret_cast<int>(hwnd));
				m->msgQueue->put(closeProcessMsg);
			}
		}
		catch (...)
		{
			return;
		}
	}
}

void CALLBACK Monitor::MngFocusChg(
	HWINEVENTHOOK hWinEventHook,
	DWORD         event,
	HWND          hwnd,
	LONG          idObject,
	LONG          idChild,
	DWORD         dwEventThread,
	DWORD         dwmsEventTime
)
{
	m->_MngFocusChg(hWinEventHook,
		event,
		hwnd,
		idObject,
		idChild,
		dwEventThread,
		dwmsEventTime);
}

void Monitor::_MngFocusChg(
	HWINEVENTHOOK hWinEventHook,
	DWORD         event,
	HWND          hwnd,
	LONG          idObject,
	LONG          idChild,
	DWORD         dwEventThread,
	DWORD         dwmsEventTime
)
{
	if (hwnd && idObject == OBJID_WINDOW)
	{
		HWND newFocusedWnd = GetAncestor(hwnd, GA_ROOTOWNER);

		{
			std::lock_guard<std::mutex> lg(focusedWdn_mutex);
			//focusedWnd = GetForegroundWindow();
			focusedWnd = newFocusedWnd;
			
			// Post the focus change message
			Message focusChangeMsgt(FOCUS_CHANGE, reinterpret_cast<int>(focusedWnd));
			m->msgQueue->put(focusChangeMsgt);

			/*std::map<int, Process>::iterator it;
			{
				// Protect the shared access to the process list
				std::unique_lock<std::mutex> l(processList_mutex);

				// Look for the window that has got the focus
				it = m->processList.find(reinterpret_cast<int>(newFocusedWnd));
			}
			
			bool found = false;
			if (it != m->processList.end())
			{
				if (newFocusedWnd != focusedWnd)
				{	
					// Update the focused process
					focusedWnd = newFocusedWnd;
					found = true;
				}
			}

			if (found == false)
				focusedWnd = 0;

			// Post the focus change message
			Message focusChangeMsg(FOCUS_CHANGE, reinterpret_cast<int>(focusedWnd));
			m->msgQueue->put(focusChangeMsg);*/
		}
	}
}

std::string MBFromW(LPCWSTR pwsz) {
	int cch = WideCharToMultiByte(CP_UTF8, 0, pwsz, -1, 0, 0, NULL, NULL);

	char* psz = new char[cch];

	WideCharToMultiByte(CP_UTF8, 0, pwsz, -1, psz, cch, NULL, NULL);

	std::string st(psz);
	delete[] psz;

	return st;
}