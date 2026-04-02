#pragma once
#include "afxdialogex.h"

#include "CMyVideoCapture.h"

// CDlgDisp1TabPage 대화 상자

class CDlgDisp1TabPage : public CDialog
{
	DECLARE_DYNAMIC(CDlgDisp1TabPage)

public:
	CDlgDisp1TabPage(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgDisp1TabPage();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DISP1_TAB };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	int OnInitProgram(int iPage);

private:
	CMyVideoCapture m_Video;

	// CWinThread* m_pThread;
	// bool m_isWorkingThread;

	// static UINT ThreadForPlayVideo(LPVOID pParam);

public:
	void OnPlayVideo();
	void ReleaseThread();
};
