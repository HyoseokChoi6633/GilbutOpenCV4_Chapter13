#pragma once

class CMyTabCtl
{
public:
	int OnInitCtl();
	void OnTcnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	void OnDispTypeChange(EDisplayMode eDisplayMode);
	void DoDataExchange(CDataExchange* pDX, int iIDC);
	void OnThreadDestroy();
	void SetHogSkipFrame(bool bHogSkipFrame);
	void SetShowFPS(bool bShowFPS);

private:
	LPCTSTR m_arrtabCtlTitle[4] = { _T("Temlate"), _T("Cascade"), _T("HOG"), _T("QRCode") };
	// CDialog 포인터 배열 대신 vector 사용
	std::vector<std::unique_ptr<CDialog>> m_vDlgList;
	CTabCtrl m_tabCtl;
};

