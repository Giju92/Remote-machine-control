#pragma once
#include <stdafx.h>
#include <cereal/access.hpp>
#include <cereal/archives/json.hpp>


class Process
{
public:
	Process();
	~Process();
	Process(int id, HWND hwnd, std::string name);
	Process(const Process& source);
	void Process::setIconBytes(BYTE *bytes, int dim);

	int getId() const;
	std::string getName() const;
	HWND getHwnd() const;

	void setFullPath(std::string fullPath);

private:
	int id;
	HWND hwnd;
	std::string name;
	std::string fullPath;
	unsigned char *iconBytes;
	int iconSize;

	friend class cereal::access;

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar(cereal::make_nvp("hwnd", reinterpret_cast<int>(hwnd)), CEREAL_NVP(id), CEREAL_NVP(name), CEREAL_NVP(fullPath), cereal::make_nvp("icon", cereal::base64::encode(iconBytes, iconSize)));
	}
};