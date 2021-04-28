#include <controller/Controller.h>


Controller::Controller(HWND statusLabel, HWND startButton, HWND stopButton): active(false)
{
	std::shared_ptr<BlockingQueue<Message>> msgQueue = std::make_shared<BlockingQueue<Message>>(20);
	
	this->statusLabel = statusLabel;
	this->startButton = startButton;
	this->stopButton = stopButton;
	
	// init the monitor
	monitor.setMesgQueue(msgQueue);

	// init the connection manager
	connection.setMsgQueue(msgQueue);
	connection.setMonitor(&(this->monitor));
}

void Controller::start()
{
	// start the monitor
	monitor.start();

	// start the controller thread
	active = true;
	tc = std::thread(&Controller::operator(), this);
}

void Controller::stop()
{
	{
		std::lock_guard<std::mutex> ul(activeMutex);
		if (active == false)
			return;
		else
			active = false;
	}
	tc.join();

	monitor.stop();
	connection.stop();
}


void Controller::operator()()
{
	std::string clientIp;

	try
	{
		clientIp = connection.start();
	}
	catch (...)
	{
		SetWindowText(statusLabel, L"Connection error");
		ShowWindow(statusLabel, SW_HIDE);
		ShowWindow(statusLabel, SW_SHOW);
		return;
	}
	
	// Create the message
	wchar_t wcharMsg[300] = L"Connection established with client ";
	wchar_t wcharIp[300];
	size_t origsize = clientIp.length() + 1;
	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, wcharIp, origsize, clientIp.c_str(), _TRUNCATE);
	wcsncat_s(wcharMsg, wcharIp, sizeof(wcharIp));

	SetWindowText(statusLabel, wcharMsg);
	ShowWindow(statusLabel, SW_HIDE);
	ShowWindow(statusLabel, SW_SHOW);

	EnableWindow(stopButton, TRUE);

	std::unique_lock<std::mutex> ul(activeMutex);
	while (active)
	{
		ul.unlock();

		// Sleep and wait for changes
		std::this_thread::sleep_for(std::chrono::seconds(1));
		
		bool res;
		{
			ul.lock();
			res = active;
			ul.unlock();
		}

		if (res == true)
		{
			if (!connection.isConnectionOpen())
			{
				monitor.stop();
				connection.resume();

				SetWindowText(statusLabel, L"Connection closed by the client");
				ShowWindow(statusLabel, SW_HIDE);
				ShowWindow(statusLabel, SW_SHOW);

				EnableWindow(stopButton, FALSE);
				EnableWindow(startButton, TRUE);
				break;
			}
			else if (connection.hasConnectionError())
			{
				monitor.stop();
				connection.resume();

				SetWindowText(statusLabel, L"Connection error");
				ShowWindow(statusLabel, SW_HIDE);
				ShowWindow(statusLabel, SW_SHOW);

				EnableWindow(stopButton, FALSE);
				EnableWindow(startButton, TRUE);
				break;
			}
		}
				
		ul.lock();
	}
}