#include <monitor/IconConverter.h>

IconConverter::IconConverter(): bitmap(nullptr), hIcon(nullptr), pIStream(nullptr), size(0)
{
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

IconConverter::~IconConverter()
{
	Gdiplus::GdiplusShutdown(gdiplusToken);
}

bool IconConverter::setHICON(HICON hIcon)
{
	LARGE_INTEGER li;
	STATSTG myStreamStats = { 0 };

	if (hIcon == nullptr)
		return false;

	this->hIcon = hIcon;

	// convert the HICON to Bitmap
	bitmap = new Gdiplus::Bitmap(hIcon);

	// create an empty IStream
	CreateStreamOnHGlobal(NULL, false, &pIStream);
	if (pIStream == NULL)
		return false;

	// Save to PNG
	CLSID pngClsid;
	CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &pngClsid);

	// write the bitmap to the stream in PNG format
	bitmap->Save(pIStream, &pngClsid, NULL);

	// retrieve the icon info
	if (FAILED(pIStream->Stat(&myStreamStats, 0)))
		return false;

	// reset the stream
	li.QuadPart = 0;
	if (FAILED(pIStream->Seek(li, STREAM_SEEK_SET, NULL)))
		return false;

	this->size = myStreamStats.cbSize.QuadPart;
	return true;
}

size_t IconConverter::getIconSize() const
{
	return this->size;
}

bool IconConverter::getIconBytes(BYTE* buffer, size_t dim) const
{
	ULONG bytesSaved = 0;
	LARGE_INTEGER li;

	// reset the stream
	li.QuadPart = 0;
	pIStream->Seek(li, STREAM_SEEK_SET, NULL);

	// check the if the output array is enough big to contain the whole icon
	if (dim < size)
		return false;

	// copy the png bytes to the output buffer
	if (FAILED(pIStream->Read(buffer, size, &bytesSaved)))
		return false;

	// reset the stream again
	li.QuadPart = 0;
	pIStream->Seek(li, STREAM_SEEK_SET, NULL);

	return true;
}

void IconConverter::reset()
{
	// release the stream
	this->pIStream->Release();

	this->hIcon = nullptr;
	
	// delete the bitmap
	if (this->bitmap != nullptr)
	{
		delete this->bitmap;
		this->bitmap = nullptr;
	}
	
	// reset the image size
	this->size = 0;
}
