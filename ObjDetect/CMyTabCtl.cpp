#include "pch.h"
#include "CMyTabCtl.h"
#include "CDlgDisp3TabPage.h"
#include "CDlgDisp1TabPage.h"

#include "resource.h"

int CMyTabCtl::OnInitCtl()
{
	int i;
	int iRetVal = 0;
	const int iTabSize = _countof(m_arrtabCtlTitle);

	for (i = 0; i < iTabSize; i++) {
		m_tabCtl.InsertItem(i, m_arrtabCtlTitle[i]);
	}

	m_tabCtl.SetCurSel(0);

	CRect rtTab;
	m_tabCtl.GetWindowRect(rtTab);

	m_parrDlg = new CDialog*[iTabSize];

	CDialog* parrDlg[iTabSize];

	parrDlg[0] = new CDlgDisp3TabPage;
	parrDlg[1] = new CDlgDisp3TabPage;
	parrDlg[2] = new CDlgDisp1TabPage;
	parrDlg[3] = new CDlgDisp1TabPage;

	int arriIDC[iTabSize];

	arriIDC[0] = IDD_DISP3_TAB;
	arriIDC[1] = IDD_DISP3_TAB;
	arriIDC[2] = IDD_DISP1_TAB;
	arriIDC[3] = IDD_DISP1_TAB;

	for (i = 0; i < iTabSize; i++) {
		m_parrDlg[i] = parrDlg[i];
		m_parrDlg[i]->Create(arriIDC[i], &m_tabCtl);
		m_parrDlg[i]->MoveWindow(0, 20, rtTab.Width(), rtTab.Height() - 20);
		m_parrDlg[i]->ShowWindow(i == 0 ? SW_SHOW : SW_HIDE);
	}

	for (i = 0; i < iTabSize; i++) {

		switch (i) {
		case 0:
		case 1:
			iRetVal = ((CDlgDisp3TabPage*)parrDlg[i])->OnInitProgram(i);
			break;
		case 2:
		case 3:
			iRetVal = ((CDlgDisp1TabPage*)parrDlg[i])->OnInitProgram(i);
			break;
		}

		if (iRetVal == -1) {
			break;
		}
	}

	if (i == 3 && iRetVal >= -1) {
		if (AfxMessageBox(_T("카메라가 시스템에 없습니다.\n그래도 실행하시겠습니까?"), MB_ICONQUESTION | MB_YESNO) == IDYES) {
			iRetVal = 0;
		}
	}

	return iRetVal;
}

void CMyTabCtl::OnTcnSelchange(NMHDR* pNMHDR, LRESULT* pResult)
{
	int iSelect = m_tabCtl.GetCurSel();

	const int iTabSize = _countof(m_arrtabCtlTitle);

	for (int i = 0; i < iTabSize; i++) {
		m_parrDlg[i]->ShowWindow(i == iSelect ? SW_SHOW : SW_HIDE);
	}

	// 탭을 벗어 났을 때 쓰레드 때문에 난잡해진 코드...
	// 테스트 용으로 바주시길...
	if (iSelect >= 2) {
		OnThreadDestroy();
		((CDlgDisp1TabPage*)m_parrDlg[iSelect])->OnPlayVideo();
	}
	else {
		OnThreadDestroy();
	}
}

void CMyTabCtl::DoDataExchange(CDataExchange* pDX, int iIDC)
{
	DDX_Control(pDX, iIDC, m_tabCtl);
}

void CMyTabCtl::OnThreadDestroy()
{
	((CDlgDisp1TabPage*)m_parrDlg[2])->ReleaseThread();
	((CDlgDisp1TabPage*)m_parrDlg[3])->ReleaseThread();
}
