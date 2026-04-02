#pragma once

class CMyTabCtl
{
public:
	int OnInitCtl();
	void OnTcnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	void DoDataExchange(CDataExchange* pDX, int iIDC);
	void OnThreadDestroy();
private:
	LPCTSTR m_arrtabCtlTitle[4] = { _T("Temlate"), _T("Cascade"), _T("HOG"), _T("QRCode") };
	CDialog** m_parrDlg;
	CTabCtrl m_tabCtl;
};

