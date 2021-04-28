#include <message/Process.h>


Process::Process(): id(0), hwnd(nullptr), iconBytes(nullptr), iconSize(0)
{
}

Process::Process(const Process& source)
{
	this->id = source.id;
	this->hwnd = source.hwnd;
	this->name = source.name;
	this->fullPath = source.fullPath;
	this->iconSize = source.iconSize;

	this->iconBytes = new unsigned char[this->iconSize];
	memcpy(this->iconBytes, source.iconBytes, this->iconSize);
}

Process::Process(int id, HWND hwnd, std::string name): iconBytes(nullptr), iconSize(0)
{
	this->id = id;
	this->hwnd = hwnd;
	this->name = name;
}

void Process::setIconBytes(BYTE *bytes, int dim)
{
	this->iconSize = dim;
	this->iconBytes = new unsigned char[dim];
	
	memcpy(this->iconBytes, bytes, dim);
}

int Process::getId() const
{
	return id;
}

std::string Process::getName() const
{
	return name;
}

HWND Process::getHwnd() const
{
	return this->hwnd;
}

void Process::setFullPath(std::string fullPath)
{
	this->fullPath = fullPath;
}

Process::~Process()
{
	if (iconBytes != nullptr)
	{
		delete[] iconBytes;
		iconBytes = nullptr;
	}
}
