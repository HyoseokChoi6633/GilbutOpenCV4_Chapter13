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

	// 다이얼로그 ID 정의
	int arriIDC[4] = { IDD_DISP3_TAB, IDD_DISP3_TAB, IDD_DISP1_TAB, IDD_DISP1_TAB };

	// 1. 다이얼로그 동적 생성 및 등록
	for (i = 0; i < iTabSize; i++) {
		std::unique_ptr<CDialog> pDlg;

		if (i < 2) pDlg = std::make_unique<CDlgDisp3TabPage>();
		else       pDlg = std::make_unique<CDlgDisp1TabPage>();

		pDlg->Create(arriIDC[i], &m_tabCtl);
		pDlg->MoveWindow(0, 20, rtTab.Width(), rtTab.Height() - 20);
		pDlg->ShowWindow(i == 0 ? SW_SHOW : SW_HIDE);

		m_vDlgList.push_back(std::move(pDlg));
	}

	// 2. 초기화 루틴
	for (i = 0; i < iTabSize; i++) {
		// 벡터에 저장된 포인터 사용
		CDialog* pDlg = m_vDlgList[i].get();

		if (i < 2) iRetVal = ((CDlgDisp3TabPage*)pDlg)->OnInitProgram(i);
		else       iRetVal = ((CDlgDisp1TabPage*)pDlg)->OnInitProgram(i);

		if (iRetVal == -1) break;
	}

	// 3. 예외 처리
	if (iRetVal == -1) {
		if (AfxMessageBox(_T("카메라가 시스템에 없습니다.\n그래도 실행하시겠습니까?"), MB_ICONQUESTION | MB_YESNO) != IDYES) {
			return -1;
		}
		iRetVal = 0;
	}

	return iRetVal;
}

void CMyTabCtl::OnTcnSelchange(NMHDR* pNMHDR, LRESULT* pResult)
{
	static int iPrevSel = 0;
	int iSelect = m_tabCtl.GetCurSel();

	const int iTabSize = _countof(m_arrtabCtlTitle);

	for (int i = 0; i < iTabSize; i++) {
		m_vDlgList[i]->ShowWindow(i == iSelect ? SW_SHOW : SW_HIDE);
	}

	if (iSelect >= 2) {
		((CDlgDisp1TabPage*)m_vDlgList[iSelect].get())->OnPlayVideo();
	}

	if (iPrevSel >= 2) {
		((CDlgDisp1TabPage*)m_vDlgList[iPrevSel].get())->OnPauseVideo();
	}

	iPrevSel = iSelect;
}

void CMyTabCtl::OnDispTypeChange(EDisplayMode eDisplayMode)
{
	int iSelect = m_tabCtl.GetCurSel();

	if (iSelect >= 2) {
		((CDlgDisp1TabPage*)m_vDlgList[iSelect].get())->SetDisplayMode(eDisplayMode);
	}
	else {
		((CDlgDisp3TabPage*)m_vDlgList[iSelect].get())->SetDisplayMode(eDisplayMode);
	}
}

void CMyTabCtl::DoDataExchange(CDataExchange* pDX, int iIDC)
{
	DDX_Control(pDX, iIDC, m_tabCtl);
}

void CMyTabCtl::OnThreadDestroy()
{
	((CDlgDisp1TabPage*)m_vDlgList[2].get())->ReleaseThread();
	((CDlgDisp1TabPage*)m_vDlgList[3].get())->ReleaseThread();
}

void CMyTabCtl::SetHogSkipFrame(bool bHogSkipFrame) {
	((CDlgDisp1TabPage*)m_vDlgList[2].get())->SetHogSkipFrame(bHogSkipFrame);
}

void CMyTabCtl::SetShowFPS(bool bShowFPS) {
	((CDlgDisp1TabPage*)m_vDlgList[2].get())->SetShowFPS(bShowFPS);
	((CDlgDisp1TabPage*)m_vDlgList[3].get())->SetShowFPS(bShowFPS);
}