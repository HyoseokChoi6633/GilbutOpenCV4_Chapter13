// CDlgDisp1TabPage.cpp: 구현 파일
//

#include "pch.h"
#include "ObjDetect.h"
#include "afxdialogex.h"
#include "CDlgDisp1TabPage.h"


// CDlgDisp1TabPage 대화 상자

IMPLEMENT_DYNAMIC(CDlgDisp1TabPage, CDialog)

CDlgDisp1TabPage::CDlgDisp1TabPage(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DISP1_TAB, pParent)
{

}

CDlgDisp1TabPage::~CDlgDisp1TabPage()
{
}

void CDlgDisp1TabPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgDisp1TabPage, CDialog)
//	ON_WM_PAINT()
END_MESSAGE_MAP()


// CDlgDisp1TabPage 메시지 처리기


int CDlgDisp1TabPage::OnInitProgram(int iPage)
{
	// TODO: 여기에 구현 코드 추가.
	int iRetVal = 0;
	switch (iPage) {
	case 2:
		if (!m_Video.OnOpenVideo(_T("vtest.avi"))) {
			iRetVal = -1;
		}
		break;
	case 3:
		if (!m_Video.OnOpenVideo(0)) {
			iRetVal = -1;
		}
		break;
	default:
		iRetVal = -1;
		break;
	}

	if (iRetVal == 0) {
		CStatic* pstPic = (CStatic*)GetDlgItem(IDC_PIC_SRC);
		m_Video.SetPicCtrl(pstPic);
	}

	return iRetVal;
}


//UINT CDlgDisp1TabPage::ThreadForPlayVideo(LPVOID pParam)
//{
//	CDlgDisp1TabPage* pDisp1Page = (CDlgDisp1TabPage*)pParam;
//
//	pDisp1Page->m_Video.DispVideo(&(pDisp1Page->m_isWorkingThread), false);
//
//	return 0;
//}

void CDlgDisp1TabPage::OnPlayVideo()
{
	// TODO: 여기에 구현 코드 추가.
	m_Video.CreateThreadForVideo();
	//m_pThread = AfxBeginThread(ThreadForPlayVideo, this);
}

void CDlgDisp1TabPage::ReleaseThread()
{
	m_Video.ReleaseThread();
	//m_isWorkingThread = !bExitThread;
}
