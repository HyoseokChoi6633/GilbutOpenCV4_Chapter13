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

	size_t nSize = sizeof(BITMAPINFOHEADER) + (iBPP == 8 ? (256 * sizeof(RGBQUAD)) : 0);

	// 1. 배열 형태의 unique_ptr 할당 (자동으로 delete[] 호출됨)
	m_smartBitmapInfo = std::make_unique<BYTE[]>(nSize);

    // [중요] 할당된 메모리 주소를 BITMAPINFO 포인터로 변환하여 할당합니다.
    BITMAPINFO* pBitmapInfo = reinterpret_cast<BITMAPINFO*>(m_smartBitmapInfo.get());

    // 3. 헤더 초기화
    ZeroMemory(&pBitmapInfo->bmiHeader, sizeof(BITMAPINFOHEADER));
    pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pBitmapInfo->bmiHeader.biPlanes = 1;
    pBitmapInfo->bmiHeader.biBitCount = (WORD)iBPP;
    pBitmapInfo->bmiHeader.biCompression = BI_RGB;
    pBitmapInfo->bmiHeader.biWidth = iWidth;
    pBitmapInfo->bmiHeader.biHeight = iHeight * (fOrigin ? 1 : -1);

    // 4. 색상 테이블 설정 (8bit 인 경우)
    if (iBPP == 8) {
        for (int i = 0; i < 256; i++) {
            pBitmapInfo->bmiColors[i].rgbBlue = (BYTE)i;
            pBitmapInfo->bmiColors[i].rgbGreen = (BYTE)i;
            pBitmapInfo->bmiColors[i].rgbRed = (BYTE)i;
            pBitmapInfo->bmiColors[i].rgbReserved = 0;
        }
    }
}

PBITMAPINFO CMyBitmapInfo::GetBitmapInfoPtr()
{
    // m_smartBitmapInfo.get()은 BYTE*를 반환합니다.
    // 이를 BITMAPINFO*로 안전하게 형변환합니다.
    return reinterpret_cast<PBITMAPINFO>(m_smartBitmapInfo.get());
}

void CMyBitmapInfo::Reset()
{
	m_smartBitmapInfo.reset();
}
