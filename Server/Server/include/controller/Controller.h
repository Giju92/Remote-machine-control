#pragma once
#include <stdafx.h>
#include <queue/BlockingQueue.h>
#include <socket/Connections.h>

class Controller
{
public:
	Controller(HWND guiHwnd, HWND stopButton, HWND startButton);
	void start();
	void stop();

private:
	Connections connection;
	Monitor monitor;

	std::thread tc;

	bool active;
	std::mutex activeMutex;

	HWND statusLabel;
	HWND stopButton;
	HWND startButton;
private:
	void operator()();
};