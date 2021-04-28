#pragma once

#include <cereal/access.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include "Process.h"


class Message
{
public:
	Message();
	Message(std::string msg_type);
	Message(std::string msg_type, const int hwnd);
	Message(std::string msg_type, const Process p);
	Message(std::string msg_type, const Process p, const int hwnd);
	Message(std::string msg_type, std::vector<Process> process);
	Message(std::string msg_type, const int hwnd, std::vector<Process> process);
	
	std::string	getMsgType() const;
	int getHwnd() const;
	std::vector<Process> getProcessList() const;
	std::vector<int> getCommands() const;

	void setProcessList(std::vector<Process> list);

private:
	std::string msg_type;
	int hwnd;
	std::vector<Process> process_list;
	std::vector<int> commands;

	friend class cereal::access;

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar(CEREAL_NVP(msg_type), CEREAL_NVP(hwnd), CEREAL_NVP(process_list), CEREAL_NVP(commands));
	}
};