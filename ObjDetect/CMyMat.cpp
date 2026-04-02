#include "pch.h"
#include "CMyMat.h"
#include "CMyBitmapInfo.h"

CMyMat::CMyMat()
{
}

CMyMat::~CMyMat()
{
}

int CMyMat::LoadImg(CString strImgFile, int iFlag)
{
    string strImgFile_a;

#ifdef UNICODE
    wstring strImgFile_w = strImgFile;
    strImgFile_a.assign(strImgFile_w.begin(), strImgFile_w.end());
#else
    strImgFile_a = strImgFile;;
#endif

    if (!m_Img.empty()) {
        m_Img.release();
    }

    if (strImgFile == "") {
        return -1;
    }
    else {
        m_Img = imread(strImgFile_a, iFlag);
    }

    if (m_Img.empty()) {
        return -1;
    }

    CreateDIB(true);

    return 0;
}


void CMyMat::DispMat(CStatic* pPicControl, bool fRatio)
{
    if (!pPicControl || m_Img.empty()) {
        return;
    }

    Mat imgDsp;
    int iWidthChange;
    
    iWidthChange = (((8 * m_Img.cols) + 31) / 32) * 4;
    resize(m_Img, imgDsp, Size(iWidthChange, m_Img.rows));

    CClientDC dc(pPicControl);

    CRect rtClient;
    pPicControl->GetClientRect(rtClient);

    if (fRatio) {
        CSize szImg;;
        szImg.SetSize(imgDsp.cols, imgDsp.rows);
        ImgRatio(szImg, rtClient);
    }

    CMyBitmapInfo objBitmapInfo;
    objBitmapInfo.CreateBitmapInfo(imgDsp.cols, imgDsp.rows, imgDsp.channels() * 8, false);

    SetStretchBltMode(dc.GetSafeHdc(), COLORONCOLOR);
    StretchDIBits(dc.GetSafeHdc(), rtClient.left, rtClient.top, rtClient.Width(), rtClient.Height(), 0, 0, imgDsp.cols, imgDsp.rows, imgDsp.data, objBitmapInfo.GetBitmapInfoPtr(), DIB_RGB_COLORS, SRCCOPY);

    imgDsp.release();
}

void CMyMat::DispDIB(CPaintDC* pDC, CWnd* pWnd, CStatic* pPicControl, bool fRatio)
{
    HBITMAP hDIB = m_objImgDIB.GetDIB();
    if (!hDIB || !pPicControl) {
        return;
    }

    CRect rect;
    pPicControl->GetWindowRect(rect);

    pWnd->ScreenToClient(rect);

    BITMAP bit;
    CDC hMemDC;
    hMemDC.CreateCompatibleDC(pDC);
    hMemDC.SelectObject(hDIB);
    GetObject(hDIB, sizeof(BITMAP), &bit);

    if (fRatio) {
        CSize szImg;;
        szImg.SetSize(bit.bmWidth, bit.bmHeight);
        ImgRatio(szImg, rect);
    }

    pDC->SetStretchBltMode(COLORONCOLOR);
    pDC->StretchBlt(rect.left, rect.top, rect.Width(), rect.Height(), &hMemDC, 0, 0, bit.bmWidth, bit.bmHeight, SRCCOPY);
}

Mat& CMyMat::GetMat()
{
    // TODO: 여기에 return 문을 삽입합니다.
    return m_Img;
}

void CMyMat::SetMat(const Mat& img)
{
    m_Img = img;
}

void CMyMat::setLabel(const vector<Point>& pts, const String& label)
{
    Rect rt = boundingRect(pts);
    rectangle(m_Img, rt, Scalar(0, 0, 255), 2);
    putText(m_Img, label, rt.tl(), FONT_HERSHEY_PLAIN, 2, Scalar(0, 0, 255), 2);
}

CMyMat& CMyMat::operator=(const Mat& matRight)
{
    // 더 간결한 함수 (모든 속성 일치 여부 확인)
    auto areMatsIdentical = [](const cv::Mat& mat1, const cv::Mat& mat2) {
        if (mat1.empty() && mat2.empty()) {
            return true;
        }
        if (mat1.empty() || mat2.empty()) {
            return false;
        }
        if (mat1.size() != mat2.size() || mat1.type() != mat2.type()) {
            return false;
        }
        return cv::countNonZero(mat1 != mat2) == 0;
        };

    if (areMatsIdentical(m_Img, matRight)) {
        return *this;
    }

    m_Img.release();

    SetMat(matRight);
    CreateDIB(false);

    return *this;
}

CMyMat& CMyMat::operator+=(const Scalar& scalRIght)
{
    this->m_Img += scalRIght;
    return *this;
}

bool CMyMat::empty()
{
    return m_Img.empty();
}

void CMyMat::CreateDIB(bool fOrigin)
{
    m_objImgDIB.MatToDIB(&m_Img, fOrigin);
}

void CMyMat::ImgRatio(CSize szImg, CRect& rtImg)
{
    // 영상 비율 계산 및 반영
    float fImageRatio = float(szImg.cx) / float(szImg.cy);
    float fRectRatio = float(rtImg.Width()) / float(rtImg.Height());
    float fScaleFactor;
    if (fImageRatio < fRectRatio) {
        fScaleFactor = float(rtImg.Height()) / float(szImg.cy);
        int rightWithRatio = szImg.cx * fScaleFactor;
        float halfOfDif = ((float)rtImg.Width() - (float)rightWithRatio) / (float)2.0f;
        rtImg.left += halfOfDif;
        rtImg.right -= halfOfDif;
    }
    else {
        fScaleFactor = float(rtImg.Width()) / float(szImg.cx);
        int bottomWithRatio = szImg.cy * fScaleFactor;
        float halfOfDif = ((float)rtImg.Height() - (float)bottomWithRatio) / (float)2.0f;
        rtImg.top += halfOfDif;
        rtImg.bottom -= halfOfDif;
    }
}
