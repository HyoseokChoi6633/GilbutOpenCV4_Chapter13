#pragma once
#include "afxdialogex.h"


// CDlgDisp3TabPage 대화 상자

#include "CMyMat.h"

class CDlgDisp3TabPage : public CDialog
{
	DECLARE_DYNAMIC(CDlgDisp3TabPage)

public:
	CDlgDisp3TabPage(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDlgDisp3TabPage();

	int OnInitProgram(int iPage);

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DISP3_TAB };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

private:
	CMyMat m_Src;
	CMyMat m_Mat1;
	CMyMat m_Mat2;
public:
	afx_msg void OnPaint();
};
