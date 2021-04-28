#include <stdafx.h>
#include <monitor/Dispatcher.h>


void Dispatcher::sendCommands(const std::vector<int> commands)
{
	// Allocate the needed memory
	INPUT *input = new INPUT[commands.size()];

	// Press the keys
	for (int i = 0; i < commands.size(); i++)
	{
		input[i].type = INPUT_KEYBOARD;
		input[i].ki.wVk = commands.at(i);
		input[i].ki.dwFlags = 0; // 0 for key press
		input[i].ki.wScan = 0;
		input[i].ki.time = 0;
		input[i].ki.dwExtraInfo = 0;
		SendInput(1, &input[i], sizeof(INPUT));
	}

	// Release the keys
	for (int i = 0; i < commands.size(); i++)
	{
		input[i].ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
		SendInput(1, &input[i], sizeof(INPUT));
	}

	// free the allocated memory
	delete[] input;
}
