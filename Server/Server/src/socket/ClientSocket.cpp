#include <socket/ClientSocket.h>
#include <socket/SocketConf.h>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#pragma comment(lib, "Ws2_32.lib")


ClientSocket::ClientSocket(): listenSock(INVALID_SOCKET), clientSock(INVALID_SOCKET)
{
}

std::string ClientSocket::open()
{
	WSADATA wsaData;
	struct addrinfo *serverIpAddress = NULL, hints;
	sockaddr_in clientIpAddress;
	int clientIpAddressLen;
	int res;

	// Initialize Winsock
	res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != 0) {
		throw new std::runtime_error("ClientSocket set up failed");
	}

	// Set up the hints
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	res = getaddrinfo(NULL, DEFAULT_SERVER_PORT, &hints, &serverIpAddress);
	if (res != 0) {
		WSACleanup();
		throw new std::runtime_error("ClientSocket set up failed");
	}

	// Create a SOCKET for the server to listen for client connections
	listenSock = socket(serverIpAddress->ai_family, serverIpAddress->ai_socktype, serverIpAddress->ai_protocol);
	if (listenSock == INVALID_SOCKET) {
		freeaddrinfo(serverIpAddress);
		WSACleanup();
		throw new std::runtime_error("ClientSocket set up failed");
	}

	// Setup the TCP listening socket
	res = bind(listenSock, serverIpAddress->ai_addr, (int)serverIpAddress->ai_addrlen);
	if (res == SOCKET_ERROR) {
		freeaddrinfo(serverIpAddress);
		closesocket(listenSock);
		WSACleanup();
		throw new std::runtime_error("ClientSocket set up failed");
	}

	freeaddrinfo(serverIpAddress);

	if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
		closesocket(listenSock);
		WSACleanup();
		throw new std::runtime_error("ClientSocket set up failed");
	}

	// Accept a client connection
	clientIpAddressLen = sizeof(clientIpAddress);
	clientSock = accept(listenSock, (sockaddr*)&clientIpAddress, &clientIpAddressLen);
	if (clientSock == INVALID_SOCKET) {
		closesocket(listenSock);
		WSACleanup();
		throw new std::runtime_error("ClientSocket set up failed");
	}

	// get the client IP in string format
	std::string clientIp(inet_ntoa(clientIpAddress.sin_addr));
	return clientIp;
}

int ClientSocket::close() const
{
	// cleanup
	closesocket(clientSock);
	closesocket(listenSock);
	WSACleanup();
	return 0;
}

int ClientSocket::readMessage(Message& msg)
{
	std::string json;
	int json_size;
	int res;


	fd_set sockSet;
	FD_ZERO(&sockSet);
	FD_SET(clientSock, &sockSet);

	timeval timeout;
	timeout.tv_sec = 1;

	res = select(0, &sockSet, nullptr, nullptr, &timeout);
	if(res < 0)
	{
		// error on select
		throw std::runtime_error("Error while doing select");
	}
	else if(res > 0)
	{
		// data arrived
		res = readJsonSize(clientSock, &json_size);
		if (res <= 0) {
			throw std::runtime_error("Error while reading json size");
		}
	}
	else if (res == 0)
	{
		// timeout expired
		return 1;
	}

	try
	{
		json = readJson(clientSock, json_size);
	}
	catch(std::runtime_error& e)
	{
		throw std::runtime_error("Error while reading json");
	}

	// deserialize json to Message object
	try {
		std::stringstream ss;
		ss << json;
		{
			cereal::JSONInputArchive archive(ss);
			archive(msg);
		}
	}
	catch (std::runtime_error& e)
	{
		throw std::runtime_error("Error while parsing json");
	}

	return 0;
}

int ClientSocket::sendMessage(const Message m)
{
	std::stringstream ss;
	int iResult;

	// Serialize the message object to json string
	{
		cereal::JSONOutputArchive archive(ss);
		archive(cereal::make_nvp("Message", m));
	}

	// Compute the message length and convert it to string
	ss.seekg(0, std::ios::end);
	int size = ss.tellg();
	ss.seekg(0, std::ios::beg);

	//int size = ss.str().length();
	std::string msgSize = std::to_string(size);
	msgSize.append("\n");

	// Send the message size
	iResult = send(clientSock, msgSize.c_str(), msgSize.length(), 0);
	if (iResult == SOCKET_ERROR || iResult < msgSize.length()) {
		throw std::runtime_error("Error while sending message size");
	}

	// Send the message
	try
	{
		sendJson(clientSock, ss.str().data(), ss.str().length());
	}
	catch (std::runtime_error& e)
	{
		throw std::runtime_error("Error while sending message");
	}
	
	return 0;
}

int ClientSocket::readJsonSize(SOCKET s, int *msgSize)
{
	char buf[DEFAULT_SIZE_LENGTH + 1];
	int n, rc;
	char c, *ptr;

	ptr = buf;
	for (n = 1; n < DEFAULT_SIZE_LENGTH; n++)
	{
		if ((rc = recv(s, &c, 1, 0)) == 1)
		{
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		}
		else if (rc == 0)
		{
			if (n == 1)
				return 0; /* EOF, no data read */
			else
				break; /* EOF, some data was read */
		}
		else
			return -1; /* error, errno set by read() */
	}
	*ptr = '\0'; /* null terminate like fgets() */

				 // convert the string in int
	sscanf_s(buf, "%d", msgSize);
	return n;
}

std::string ClientSocket::readJson(SOCKET s, const int msgSize)
{
	std::vector<char> buff(msgSize + 1);
	std::string msg;
	int res;


	res = recv(s, buff.data(), msgSize, 0);
	if (res == msgSize) {
		// Message received
		msg.assign(buff.data());
	}
	else if (res == 0) {
		// Connection closed
		throw std::runtime_error("Connection closed by the client");
	}
	else {
		// Unknown error
		throw std::runtime_error("Unknown error");
	}

	return msg;
}

void ClientSocket::sendJson(SOCKET s, const char *buff, int len)
{
	int remaining_bytes, nwrite;
	int index = 0;


	remaining_bytes = len;
	while (remaining_bytes > 0)
	{
		if (remaining_bytes > DEFAULT_BLOCK_SIZE)
		{
			nwrite = send(s, &buff[index], DEFAULT_BLOCK_SIZE, 0);
			if (nwrite < DEFAULT_BLOCK_SIZE)
			{
				throw std::runtime_error("Error while sending message block");
			}
		}
		else
		{
			/* if the remaining bytes are less than the size of the buffer then use only the ramainining_bytes of the buffer */
			nwrite = send(s, &buff[index], remaining_bytes, 0);
			if (nwrite < remaining_bytes)
			{
				throw std::runtime_error("Error while sending message block");
			}
		}

		remaining_bytes = remaining_bytes - nwrite;
		index += nwrite;
	}
}
