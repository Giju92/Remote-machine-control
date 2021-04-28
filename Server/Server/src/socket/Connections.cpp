#include <socket/Connections.h>
#include <message/Message.h>
#include <message/MsgTypes.h>
#include <monitor/Dispatcher.h>
#include <thread>


Connections::Connections(): monitor(nullptr), readActive(true), connectionOpen(false), connectionError(false)
{
	this->msgQueue = std::make_shared<BlockingQueue<Message>>(20);
}

void Connections::setMonitor(Monitor* monitor)
{
	this->monitor = monitor;
}

void Connections::setMsgQueue(std::shared_ptr<BlockingQueue<Message>> msgQueue)
{
	this->msgQueue = msgQueue;
}

std::string Connections::start()
{
	std::string clientIp;
	

	try
	{
		clientIp = cs.open();
	}
	catch (std::exception & e)
	{
		throw new std::runtime_error(e.what());
	}
	
	connectionOpen = true;

	// start the reader thread
	tr = std::thread(&Connections::read, this);

	// start the writer thread
	tw = std::thread(&Connections::write, this);

	return clientIp;
}

void Connections::stop()
{
	// stop the reading thread
	{
		std::lock_guard<std::mutex> lg(readActiveMutex);
		readActive = false;
	}

	// post the close message 
	Message closeMsg(CONNECTION_CLOSED);
	msgQueue->put(closeMsg);

	// close the message queue (this closes the writing thread)
	msgQueue->close();

	// Join the killed thread
	tr.join();
	tw.join();
	
	{
		std::lock_guard<std::mutex> lg(connectionOpenMutex);
		connectionOpen = false;
	}

	cs.close();
}

boolean Connections::isConnectionOpen()
{
	boolean res;

	{
		std::lock_guard<std::mutex> lg(connectionOpenMutex);
		res = connectionOpen;
	}
	
	return res;
}

boolean Connections::hasConnectionError()
{
	boolean res;

	{
		std::lock_guard<std::mutex> lg(connectionErrorMutex);
		res = connectionError;
	}

	return res;
}

void Connections::resume()
{
	// close the message queue (this closes the writing thread)
	msgQueue->close();

	// stop the reading thread
	{
		std::lock_guard<std::mutex> lg(readActiveMutex);
		readActive = false;
	}

	tr.join();
	tw.join();

	cs.close();
}

void Connections::read()
{
	Message m;
	int res;

	// Wait for client messages
	std::unique_lock<std::mutex> ul(readActiveMutex);
	while (readActive)
	{
		ul.unlock();

		try
		{
			res = cs.readMessage(m);
		}
		catch (...)
		{
			// Notify the controller
			std::lock_guard<std::mutex> lg(connectionErrorMutex);
			connectionError = true;
			break;
		}

		// check if the message has been received
		if (res == 0)
		{
			if (m.getMsgType().compare(CONNECTION_CLOSED) == 0)
			{
				// Notify the controller
				std::lock_guard<std::mutex> lg(connectionOpenMutex);
				connectionOpen = false;
				break;
			}
			else if (m.getMsgType().compare(PROCESS_LIST_REQ) == 0)
			{
				// Notify the monitor to send its process list
				monitor->postProcessList();
			}
			else if (m.getMsgType().compare(COMMAND) == 0)
			{
				// Dispatch the received commands
				Dispatcher::sendCommands(m.getCommands());
			}
			else if (m.getMsgType().compare(FOCUS_CHANGE_REQ) == 0)
			{
				// Set the focus to the requested window
				monitor->changeFocusedWindow(HWND(m.getHwnd()));
			}
			else if (m.getMsgType().compare(WINDOW_CLOSE_REQ) == 0)
			{
				// Close the requested window
				PostMessage(HWND(m.getHwnd()), WM_CLOSE, NULL, NULL);
			}
		}
		
		ul.lock();
	}
}

void Connections::write()
{
	while (1)
	{
		Message m;
		if (msgQueue->get(m) == false)
		{
			break;
		}

		try
		{
			cs.sendMessage(m);
		}
		catch (...)
		{
			std::lock_guard<std::mutex> lg(connectionErrorMutex);
			connectionError = true;
		}
	}
}