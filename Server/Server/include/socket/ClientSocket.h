#pragma once

#include <stdafx.h>
#include <message/Message.h>


class ClientSocket
{
public:
	ClientSocket();
	std::string open();
	int close() const;
	int readMessage(Message& msg);
	int sendMessage(const Message m);

private:
	SOCKET listenSock;
	SOCKET clientSock;

	int readJsonSize(SOCKET s, int *jsonSize);
	std::string readJson(SOCKET s, int jsonSize);
	void sendJson(SOCKET s, const char *buff, int len);
};
