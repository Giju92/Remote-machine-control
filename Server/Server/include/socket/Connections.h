#pragma once
#include <socket/ClientSocket.h>
#include <queue/BlockingQueue.h>
#include <monitor/Monitor.h>

class Connections
{
public:
	Connections();
	void setMonitor(Monitor* monitor);
	void setMsgQueue(std::shared_ptr<BlockingQueue<Message>> msgQueue);
	std::string start();
	void stop();
	boolean isConnectionOpen();
	boolean hasConnectionError();
	void resume();

private:
	void read();
	void write();

private:
	ClientSocket cs;
	Monitor* monitor;
	
	std::shared_ptr<BlockingQueue<Message>> msgQueue;

	std::thread tr;
	std::thread tw;

	bool readActive;
	std::mutex readActiveMutex;

	bool connectionOpen;
	std::mutex connectionOpenMutex;

	bool connectionError;
	std::mutex connectionErrorMutex;
};