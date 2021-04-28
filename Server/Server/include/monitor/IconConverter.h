#pragma once
#include  "stdafx.h"
#include <objidl.h>
#include <GdiPlus.h>

#pragma comment(lib, "Gdiplus.lib")


class IconConverter
{
public:
	IconConverter();
	~IconConverter();
	bool setHICON(HICON hIcon);
	size_t getIconSize() const;
	bool getIconBytes(BYTE* buffer, size_t dim) const;
	void reset();

private:
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;

	Gdiplus::Bitmap *bitmap;
	HICON hIcon;

	IStream* pIStream;
	size_t size;
};