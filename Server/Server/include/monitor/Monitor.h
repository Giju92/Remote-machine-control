#pragma once
#include <stdafx.h>
#include <message/Message.h>
#include <message/Process.h>
#include "IconConverter.h"
#include <queue/BlockingQueue.h>
#include <map>
#include <mutex>
#include <memory>


class Monitor
{
public:
	Monitor();
	Monitor(std::shared_ptr<BlockingQueue<Message>> msgQueue);
	void setMesgQueue(std::shared_ptr<BlockingQueue<Message>> msgQueue);
	void start();
	void stop();
	void postProcessList();
	void changeFocusedWindow(HWND hwnd);
	

private:
	void retrieveActiveProcesses();
	HICON getHICONFromHWND(HWND hWnd);
	HICON getHICONFromProcessPath(LPWSTR lpExeName);
	int getProcessPath(DWORD pid, LPWSTR lpExeName, DWORD *size);

	static BOOL CALLBACK enumWindowsProc(HWND hWnd, LPARAM lParam);
	BOOL _enumWindowsProc(HWND hWnd);
	BOOL isAltTabWindow(HWND hwnd);
	BOOL isWindows10BackgroudApp(HWND hWnd);

	static void CALLBACK MngCreation(
		HWINEVENTHOOK hWinEventHook,
		DWORD         event,
		HWND          hwnd,
		LONG          idObject,
		LONG          idChild,
		DWORD         dwEventThread,
		DWORD         dwmsEventTime
	);

	void _MngCreation(
		HWINEVENTHOOK hWinEventHook,
		DWORD         event,
		HWND          hwnd,
		LONG          idObject,
		LONG          idChild,
		DWORD         dwEventThread,
		DWORD         dwmsEventTime
	);

	static void CALLBACK Monitor::MngDestroy(
		HWINEVENTHOOK hWinEventHook,
		DWORD         event,
		HWND          hwnd,
		LONG          idObject,
		LONG          idChild,
		DWORD         dwEventThread,
		DWORD         dwmsEventTime
	);

	void Monitor::_MngDestroy(
		HWINEVENTHOOK hWinEventHook,
		DWORD         event,
		HWND          hwnd,
		LONG          idObject,
		LONG          idChild,
		DWORD         dwEventThread,
		DWORD         dwmsEventTime
	);

	static void CALLBACK MngFocusChg(
		HWINEVENTHOOK hWinEventHook,
		DWORD         event,
		HWND          hwnd,
		LONG          idObject,
		LONG          idChild,
		DWORD         dwEventThread,
		DWORD         dwmsEventTime
	);

	void _MngFocusChg(
		HWINEVENTHOOK hWinEventHook,
		DWORD         event,
		HWND          hwnd,
		LONG          idObject,
		LONG          idChild,
		DWORD         dwEventThread,
		DWORD         dwmsEventTime
	);

private:
	IconConverter iconConverter;
	std::shared_ptr<BlockingQueue<Message>> msgQueue;

	HWINEVENTHOOK hookWndCreate;
	HWINEVENTHOOK hookWndClose;
	HWINEVENTHOOK hookFocusChange;
	
	std::mutex processList_mutex;
	std::map<int, Process> processList;

	std::mutex focusedWdn_mutex;
	HWND focusedWnd;
};
