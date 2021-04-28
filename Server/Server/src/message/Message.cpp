#include <message/Message.h>


Message::Message(): hwnd(0)
{
}

Message::Message(std::string msg_type): hwnd(0)
{
	this->msg_type = msg_type;
}

Message::Message(std::string msg_type, const int hwnd) {
	this->msg_type = msg_type;
	this->hwnd = hwnd;
}

Message::Message(std::string msg_type, const Process p): hwnd(0)
{
	this->msg_type = msg_type;
	this->process_list.push_back(p);
}

Message::Message(std::string msg_type, const Process p, const int hwnd)
{
	this->msg_type = msg_type;
	this->process_list.push_back(p);
	this->hwnd = hwnd;
}

Message::Message(std::string msg_type, std::vector<Process> process): hwnd(0)
{
	this->msg_type = msg_type;
	this->process_list = process;
}

Message::Message(std::string msg_type, const int hwnd, std::vector<Process> process)
{
	this->msg_type = msg_type;
	this->hwnd = hwnd;
	this->process_list = process;
}

std::string Message::getMsgType() const
{
	return this->msg_type;
}

int Message::getHwnd() const
{
	return this->hwnd;
}

std::vector<Process> Message::getProcessList() const
{
	return this->process_list;
}

void Message::setProcessList(std::vector<Process> list)
{
	this->process_list = list;
}

std::vector<int> Message::getCommands() const
{
	return this->commands;
}
