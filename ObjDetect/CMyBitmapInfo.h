#pragma once
class CMyBitmapInfo
{
public:
	CMyBitmapInfo();
	~CMyBitmapInfo();
	void CreateBitmapInfo(int iWidth, int iHeight, int iBPP, bool fOrigin);

	PBITMAPINFO GetBitmapInfoPtr();

private:
	// BITMAPINFO가 아닌 BYTE 배열로 관리해야 delete[]가 안전하게 호출됩니다.
	std::unique_ptr<BYTE[]> m_smartBitmapInfo;

	void Reset();
};
