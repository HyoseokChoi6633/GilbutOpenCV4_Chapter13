#include "pch.h"
#include "CMyDIB.h"
#include "CMyBitmapInfo.h"

CMyDIB::CMyDIB() :
    m_smartBitmap(nullptr)
{
}

CMyDIB::~CMyDIB()
{
    Reset();
}

HBITMAP CMyDIB::MatToDIB(Mat* pImg, bool fOrigin)
{
    HBITMAP hBMP = NULL;

    if (!pImg || pImg->empty()) {
        return hBMP;
    }

    Mat tmp;
    switch (pImg->channels()) {
    case 1:
        cvtColor(*pImg, tmp, COLOR_GRAY2BGR);
        break;
    case 3:
        tmp = pImg->clone();
        break;
    case 4:
        cvtColor(*pImg, tmp, COLOR_BGRA2BGR);
        break;
    default:
        return hBMP;
    }
    
    int iBPP = tmp.channels() * 8;

    CMyBitmapInfo objBitmapInfo;
    objBitmapInfo.CreateBitmapInfo(tmp.cols, tmp.rows, iBPP, fOrigin);

    // Pointer to access the pixels of bitmap
    BYTE* pPixels = 0, * pP;
    hBMP = CreateDIBSection(NULL, objBitmapInfo.GetBitmapInfoPtr(), DIB_RGB_COLORS, (void**)&pPixels, NULL, 0);

    if (!hBMP)
        return hBMP; // return if invalid bitmaps

    // SetBitmapBits( hBitmap, lBmpSize, pBits);
    // Directly Write
    int left_width = ((iBPP * tmp.cols + 31) / 32) * 4;
    pP = pPixels;

    for (int y = tmp.rows-1, row = 0; row < tmp.rows; row++, y--) {
        for (int x = 0, col = 0; col < tmp.cols; x += tmp.channels(), col++) {

             pP[x + 0] = tmp.at<Vec3b>(y, col).val[0];
             pP[x + 1] = tmp.at<Vec3b>(y, col).val[1];
             pP[x + 2] = tmp.at<Vec3b>(y, col).val[2];
        }
        pP += left_width;
    }

    Reset();

    m_smartBitmap.reset(hBMP);

    tmp.release();

    return hBMP;
}

HBITMAP CMyDIB::GetDIB()
{
    return m_smartBitmap.get();
}

void CMyDIB::Reset()
{
    m_smartBitmap.reset();
}
