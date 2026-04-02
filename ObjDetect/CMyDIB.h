#pragma once

#include "SSGdi.h"

class CMyDIB
{
public:
	CMyDIB();
	~CMyDIB();

	HBITMAP MatToDIB(Mat* pImg, bool fOrigin);

	HBITMAP GetDIB();
private:
	UniqueHBitmap m_smartBitmap;
	void Reset();
};

