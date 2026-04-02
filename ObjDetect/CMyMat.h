#pragma once

#include "CMyDIB.h"

class CMyMat
{
public:
	CMyMat();
	~CMyMat();

	int LoadImg(CString strImgFile, int iFlag = IMREAD_GRAYSCALE);
	void CreateDIB(bool fOrigin);

	void DispMat(CStatic* pPicControl, bool fRatio);
	void DispDIB(CPaintDC* pDC, CWnd* pWnd, CStatic* pPicControl, bool fRatio);

	Mat& GetMat();
	void SetMat(const Mat& img);

	void setLabel(const vector<Point>& pts, const String& label);

	CMyMat& operator=(const Mat& matRight);
	CMyMat& operator+=(const Scalar& scalRIght);

	bool empty();

private:
	Mat m_Img;

	CMyDIB m_objImgDIB;

	void ImgRatio(CSize szImg, CRect& rtImg);
};

