
// ObjDetectDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "ObjDetect.h"
#include "ObjDetectDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CObjDetectDlg 대화 상자



CObjDetectDlg::CObjDetectDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_OBJDETECT_DIALOG, pParent)
	, m_eDisplayMode(MODE_GDI)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CObjDetectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	m_objTab.DoDataExchange(pDX, IDC_TAB_MAIN);
}

BEGIN_MESSAGE_MAP(CObjDetectDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_MAIN, &CObjDetectDlg::OnTcnSelchangeTabMain)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_RADIO_DISP_GDI, &CObjDetectDlg::OnClickedRadioDispGdi)
	ON_BN_CLICKED(IDC_RADIO_DISP_OPENGL, &CObjDetectDlg::OnBnClickedRadioDispOpengl)
	ON_BN_CLICKED(IDC_CHK_HOG_SKIP_FRAME, &CObjDetectDlg::OnClickedChkSkipFrame)
	ON_BN_CLICKED(IDC_CHK_SHOW_FPS, &CObjDetectDlg::OnClickedChkShowFps)
END_MESSAGE_MAP()


// CObjDetectDlg 메시지 처리기

BOOL CObjDetectDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	if (m_objTab.OnInitCtl() == -1) {
		AfxMessageBox(_T("초기화에 실패 하였습니다."));
		PostQuitMessage(WM_QUIT);
		return FALSE;
	}

	// [방법 2-A] CheckRadioButton 사용 (그룹 내에서 하나만 콕 집어 켤 때 편리)
	// 인자: (그룹의 시작 ID, 그룹의 끝 ID, 체크할 버튼 ID)
	CheckRadioButton(IDC_RADIO_DISP_GDI, IDC_RADIO_DISP_OPENGL, IDC_RADIO_DISP_OPENGL);
	m_eDisplayMode = MODE_OPENGL;

	m_objTab.OnDispTypeChange(m_eDisplayMode);

	CButton* pCheck = (CButton*)GetDlgItem(IDC_CHK_HOG_SKIP_FRAME);
	if (pCheck) {
		pCheck->SetCheck(BST_CHECKED);

		bool bChecked = (pCheck->GetCheck() == BST_CHECKED);
		m_objTab.SetHogSkipFrame(bChecked);
	}

	pCheck = (CButton*)GetDlgItem(IDC_CHK_SHOW_FPS);
	if (pCheck) {
		pCheck->SetCheck(BST_CHECKED);

		bool bChecked = (pCheck->GetCheck() == BST_CHECKED);
		m_objTab.SetShowFPS(bChecked);
	}

	// OS의 타이머 정밀도를 1ms 단위로 극대화합니다.
	::timeBeginPeriod(1);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CObjDetectDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CObjDetectDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CObjDetectDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CObjDetectDlg::OnTcnSelchangeTabMain(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	switch (pNMHDR->idFrom) {
	case IDC_TAB_MAIN:
		m_objTab.OnDispTypeChange(m_eDisplayMode);
		m_objTab.OnTcnSelchange(pNMHDR, pResult);
		break;
	default:
		break;
	}

	*pResult = 0;
}


void CObjDetectDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 2024-0616_1419 프로그램 종료시 스레드 종료 코드 추가
	// 동영상과 카메라 스레드 종료 코드
	m_objTab.OnThreadDestroy();

	// 사용이 끝나면 반드시 원래 OS 정밀도(15.6ms)로 복원해 주어야 합니다.
	::timeEndPeriod(1);
}

void CObjDetectDlg::OnClickedRadioDispGdi()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (m_eDisplayMode != MODE_GDI) {
		m_eDisplayMode = MODE_GDI;

		m_objTab.OnDispTypeChange(m_eDisplayMode);
		Invalidate(FALSE);		// 화면 갱신 요청
	}
}

void CObjDetectDlg::OnBnClickedRadioDispOpengl()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (m_eDisplayMode != MODE_OPENGL) {
		m_eDisplayMode = MODE_OPENGL;

		m_objTab.OnDispTypeChange(m_eDisplayMode);
		Invalidate(FALSE);		// 화면 갱신 요청
	}
}

void CObjDetectDlg::OnClickedChkSkipFrame()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	// 1. 체크박스 컨트롤의 현재 체크 상태를 확인합니다.
	CButton* pCheck = (CButton*)GetDlgItem(IDC_CHK_HOG_SKIP_FRAME);
	if (pCheck == nullptr) {
		return;
	}

	bool bChecked = (pCheck->GetCheck() == BST_CHECKED);

	// 2. 비디오 캡처 객체의 플래그를 실시간으로 업데이트합니다.
	m_objTab.SetHogSkipFrame(bChecked);
}

void CObjDetectDlg::OnClickedChkShowFps()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	// 1. 체크박스 컨트롤의 현재 체크 상태를 확인합니다.
	CButton* pCheck = (CButton*)GetDlgItem(IDC_CHK_SHOW_FPS);
	if (pCheck == nullptr) {
		return;
	}

	bool bChecked = (pCheck->GetCheck() == BST_CHECKED);

	m_objTab.SetShowFPS(bChecked);
}
