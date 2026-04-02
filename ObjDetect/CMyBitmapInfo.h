#pragma once
class CMyBitmapInfo
{
public:
	CMyBitmapInfo();
	~CMyBitmapInfo();
	void CreateBitmapInfo(int iWidth, int iHeight, int iBPP, bool fOrigin);

	PBITMAPINFO GetBitmapInfoPtr();

private:
	unique_ptr<BITMAPINFO> m_smartBitmapInfo;

	void Reset();
};
