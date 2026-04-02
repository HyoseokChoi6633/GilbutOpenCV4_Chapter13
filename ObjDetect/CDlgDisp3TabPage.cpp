// CDlgDisp3TabPage.cpp: 구현 파일
//

#include "pch.h"
#include "ObjDetect.h"
#include "afxdialogex.h"
#include "CDlgDisp3TabPage.h"


// CDlgDisp3TabPage 대화 상자

IMPLEMENT_DYNAMIC(CDlgDisp3TabPage, CDialog)

CDlgDisp3TabPage::CDlgDisp3TabPage(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DISP3_TAB, pParent)
{

}

CDlgDisp3TabPage::~CDlgDisp3TabPage()
{
}

int CDlgDisp3TabPage::OnInitProgram(int iPage)
{
	int iRetVal = 0;
	switch (iPage) {
	case 0:
		m_Src.LoadImg(_T("circuit.bmp"), IMREAD_COLOR);
		m_Mat1.LoadImg(_T("crystal.bmp"), IMREAD_COLOR);

		if (m_Src.empty() || m_Mat1.empty()) {
			iRetVal = -1;
			break;
		}

		m_Src += Scalar(50, 50, 50);

		{
			Mat noise(m_Src.GetMat().size(), CV_32SC3);
			randn(noise, 0, 10);
			add(m_Src.GetMat(), noise, m_Src.GetMat(), Mat(), CV_8UC3);

			Mat res, res_norm;
			matchTemplate(m_Src.GetMat(), m_Mat1.GetMat(), res, TM_CCOEFF_NORMED);
			normalize(res, res_norm, 0, 255, NORM_MINMAX, CV_8U);

			double maxv;
			Point maxloc;
			minMaxLoc(res, 0, &maxv, 0, &maxloc);
			CString strMsg;
			strMsg.Format(_T("maxv: %f"), maxv);
			SetDlgItemText(IDC_ST_DISP_MSG, strMsg);

			rectangle(m_Src.GetMat(), Rect(maxloc.x, maxloc.y, m_Mat1.GetMat().cols, m_Mat1.GetMat().rows), Scalar(0, 0, 255), 2);

			m_Src.CreateDIB(true);

			m_Mat2.SetMat(res_norm);
			m_Mat2.CreateDIB(true);

			SetDlgItemText(IDC_ST_TITLE1, _T("Template"));
			SetDlgItemText(IDC_ST_TITLE2, _T("Res_norm"));
		}

		break;
	case 1:
		iRetVal = m_Src.LoadImg(_T("kids.png"), IMREAD_COLOR);

		if (iRetVal == -1) {
			break;
		}

		{
			CascadeClassifier face_classifier("haarcascade_frontalface_default.xml");
			CascadeClassifier eye_classifier("haarcascade_eye.xml");

			if (face_classifier.empty() || eye_classifier.empty()) {
				CString strMsg;
				strMsg.Format(_T("XML load failed!"));
				SetDlgItemText(IDC_ST_DISP_MSG, strMsg);

				break;
			}

			vector<Rect> faces;
			face_classifier.detectMultiScale(m_Src.GetMat(), faces);

			m_Mat1.SetMat(m_Src.GetMat().clone());

			for (Rect rcFace : faces) {
				rectangle(m_Mat1.GetMat(), rcFace, Scalar(255, 0, 255), 2);
			}

			m_Mat2.SetMat(m_Mat1.GetMat().clone());

			for (Rect rcFace : faces) {

				Mat faceROI = m_Mat2.GetMat()(rcFace);
				vector<Rect> eyes;
				eye_classifier.detectMultiScale(faceROI, eyes);

				for (Rect rcEye : eyes) {
					Point center(rcEye.x + rcEye.width / 2, rcEye.y + rcEye.height / 2);
					circle(faceROI, center, rcEye.width / 2, Scalar(255, 0, 0), 2, LINE_AA);
				}
			}

			m_Mat1.CreateDIB(true);
			m_Mat2.CreateDIB(true);

			SetDlgItemText(IDC_ST_DISP_MSG, _T(""));
			SetDlgItemText(IDC_ST_TITLE1, _T("Face"));
			SetDlgItemText(IDC_ST_TITLE2, _T("Face and Eyes"));
		}

		break;
	default:
		iRetVal = -1;
		break;
	}

	return iRetVal;
}

void CDlgDisp3TabPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgDisp3TabPage, CDialog)
	ON_WM_PAINT()
END_MESSAGE_MAP()


// CDlgDisp3TabPage 메시지 처리기


void CDlgDisp3TabPage::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CDialog::OnPaint()을(를) 호출하지 마십시오.
	CStatic* pstPic = (CStatic*)GetDlgItem(IDC_PIC_SRC);
	m_Src.DispDIB(&dc, this, pstPic, true);

	pstPic = (CStatic*)GetDlgItem(IDC_PIC_MAT1);
	m_Mat1.DispDIB(&dc, this, pstPic, true);

	pstPic = (CStatic*)GetDlgItem(IDC_PIC_MAT2);
	m_Mat2.DispDIB(&dc, this, pstPic, true);
}
