#include "pch.h"
#include "CMyBitmapInfo.h"
#include <memory>

CMyBitmapInfo::CMyBitmapInfo() :
	m_smartBitmapInfo(nullptr)
{
}

CMyBitmapInfo::~CMyBitmapInfo()
{
	Reset();
}

void CMyBitmapInfo::CreateBitmapInfo(int iWidth, int iHeight, int iBPP, bool fOrigin)
{
	Reset();
	PBITMAPINFO pBitmapInfo = nullptr;

	if (iBPP == 8)
		pBitmapInfo = (BITMAPINFO*) new BYTE[sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD)];
	else // 24 or 32bit
		pBitmapInfo = (BITMAPINFO*) new BYTE[sizeof(BITMAPINFO)];

	m_smartBitmapInfo.reset(pBitmapInfo);
	pBitmapInfo = m_smartBitmapInfo.get();

	pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pBitmapInfo->bmiHeader.biPlanes = 1;
	pBitmapInfo->bmiHeader.biBitCount = iBPP;
	pBitmapInfo->bmiHeader.biCompression = BI_RGB;
	pBitmapInfo->bmiHeader.biSizeImage = 0;
	pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
	pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
	pBitmapInfo->bmiHeader.biClrUsed = 0;
	pBitmapInfo->bmiHeader.biClrImportant = 0;

	if (iBPP == 8)
	{
		for (int i = 0; i < 256; i++)
		{
			pBitmapInfo->bmiColors[i].rgbBlue = (BYTE)i;
			pBitmapInfo->bmiColors[i].rgbGreen = (BYTE)i;
			pBitmapInfo->bmiColors[i].rgbRed = (BYTE)i;
			pBitmapInfo->bmiColors[i].rgbReserved = 0;
		}
	}

	pBitmapInfo->bmiHeader.biWidth = iWidth;
	pBitmapInfo->bmiHeader.biHeight = iHeight * (fOrigin ? 1 : -1);
}

PBITMAPINFO CMyBitmapInfo::GetBitmapInfoPtr()
{
	return m_smartBitmapInfo.get();
}

void CMyBitmapInfo::Reset()
{
	m_smartBitmapInfo.reset();
}
