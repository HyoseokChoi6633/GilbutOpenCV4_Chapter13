#include "pch.h"
#include "CMyVideoCapture.h"
#include "CMyMat.h"
#include "CMyThreadVideo.h"

CMyVideoCapture::CMyVideoCapture() :
	m_bQRScan(false),
	m_arThReadV(nullptr),
	m_arThDrawV(nullptr),
	m_muReadV(false, nullptr),
	m_muDrawV(false, nullptr)
{
	// CPU 논리 스레드 수를 반환
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);

	m_iNumCPU = sysinfo.dwNumberOfProcessors;

	ReleaseThread();
}

CMyVideoCapture::~CMyVideoCapture()
{
	ReleaseThread();
}

bool CMyVideoCapture::OnOpenVideo(int iDeviceID)
{
	bool bRetVal;
	m_cvCap.open(iDeviceID);

	bRetVal = m_cvCap.isOpened();
	if (bRetVal) {
		m_bQRScan = true;

		m_dFPS = m_cvCap.get(CAP_PROP_FPS);
		m_iDelay = cvRound(1000 / m_dFPS);
	}

	return bRetVal;
}

bool CMyVideoCapture::OnOpenVideo(CString strVideoFile)
{
	bool bRetVal;

#ifdef UNICODE
	wstring strImgFile_w = strVideoFile;
	m_strImgFile_a.assign(strImgFile_w.begin(), strImgFile_w.end());
#else
	m_strImgFile_a = strImgFile;;
#endif

	m_cvCap.open(m_strImgFile_a);

	bRetVal = m_cvCap.isOpened();
	m_bQRScan = false;

	m_dFPS = m_cvCap.get(CAP_PROP_FPS);
	m_iDelay = cvRound(1000 / m_dFPS);

	return bRetVal;
}

void CMyVideoCapture::SetPicCtrl(CStatic* pstPic)
{
	m_pstPic = pstPic;
}

bool CMyVideoCapture::CreateThreadForVideo()
{
	bool bRetVal = false;
	int iHalfThread;
	int i;

	ReleaseThread();

	// 논리 코어 4개 이상
	if (m_iNumCPU > 3) {
		iHalfThread = m_iNumCPU / 2;
	}
	// 논리 코어 3개 이하
	else {
		iHalfThread = 2;
	}

	// VideoCapture의 읽기 다중 스레드 생성
	m_arThReadV = new CMyThreadVideo[iHalfThread];

	// 모션 캡쳐 일때 활성화
	if (!m_bQRScan) {
		m_hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
	}

	// VideoCapture의 읽기 다중 스레드를 활성화
	for (i = 0; i < iHalfThread; i++) {
		m_arThReadV[i].SetParams(this, i);
		m_arThReadV[i].SetEnableThread(true);

		AfxBeginThread(m_arThReadV[i].ThreadForReadVideo, &m_arThReadV[i]);
	}

	// Queue에 저장된 Mat 데이터를 출력시 2개의 스레드를 사용하여 번갈아 출력한다.
	m_arThDrawV = new CMyThreadVideo[2];
	for (i = 0; i < 2; i++) {
		m_arThDrawV[i].SetParams(this, i);
		m_arThDrawV[i].SetEnableThread(true);
		AfxBeginThread(m_arThDrawV[i].ThreadForDrawVideo, &m_arThDrawV[i]);
	}

	bRetVal = true;

	return bRetVal;
}

// 단순히 VideoCapture에서 프레임을 얻어온다.
Mat CMyVideoCapture::CpyFrame()
{
	Mat frame;

	m_cvCap >> frame;

	if (frame.empty()) {
		m_bEndPlay = true;
	}

	return frame;
}

// frame에 사람 객체에 사각형을 그리거나 qr code일때는 url을 frame에 표시한다.
bool CMyVideoCapture::WorkFrameToQueue(Mat& frame, int iReadIdx)
{
	if (frame.empty()) {
		return false;
	}

	unique_ptr<STReadMat> smartDispframe = nullptr;
	smartDispframe = make_unique<STReadMat>();
	bool bPushSuc;

	vector<Rect> detected;
	vector<Point> points;
	string strInfo;

	// QR Scan시 사용되는 로직
	if (m_bQRScan) {
		strInfo = m_detector.detectAndDecode(frame, points);

		if (!strInfo.empty()) {
			polylines(frame, points, true, Scalar(0, 0, 255), 2);
			putText(frame, strInfo, Point(10, 30), FONT_HERSHEY_DUPLEX, 1, Scalar(0, 0, 255));
		}
	}
	// 모션 캡쳐시 사용되는 로직
	else {
		m_hog.detectMultiScale(frame, detected);

		for (Rect r : detected) {
			Scalar c = Scalar(rand() % 256, rand() % 256, rand() % 256);
			rectangle(frame, r, c, 3);
		}
	}

	smartDispframe->imgVideo = frame;	// VideoCapture에서 얻은 Mat 저장
	smartDispframe->iIdx = iReadIdx;			// 스레드 인덱스 저장

	bPushSuc = false;

	// Queue에 순서대로 작업한 이미지를 넣기 위해 반복문을 이용하여 순서대로 넣는다.
	while (!bPushSuc) {
		m_muDrawV.Lock();

		if (m_iNowReadVideo == smartDispframe->iIdx) {
			// Queue에 저장
			m_qVideo.push(move(smartDispframe));
			bPushSuc = true;

		}
		else {
			Sleep(1);
		}

		m_muDrawV.Unlock();
	}

	return true;
}

// Queue에 저장된 Mat 데이터를 출력
bool CMyVideoCapture::DispQueueData(int iDrawIdx, bool fRatio)
{
	bool bRetVal = true;

	// Queue에 저장된 데이터 출력
	if (!m_qVideo.empty()) {
		unique_ptr<STReadMat>& refSmartDispframe = m_qVideo.front();
		if (refSmartDispframe->iIdx % 2 == iDrawIdx) {
			refSmartDispframe->imgVideo.DispMat(m_pstPic, fRatio);
			m_qVideo.pop();
		}
		else {
			bRetVal = false;
		}
	}
	else {
		bRetVal = false;
	}

	if (bRetVal && waitKey(1) == 27) {
		bRetVal = false;
	}

	// 반복 재생을 위해
	if (!bRetVal && m_bEndPlay && m_qVideo.empty()) {
		m_iNowDrawVideo = 0;
		m_iNowReadVideo = 0;
		m_cvCap.open(m_strImgFile_a);
		m_bEndPlay = false;
	}

	return bRetVal;
}

// 논리 스레드 수 반환
int CMyVideoCapture::GetNumCPU()
{
	return m_iNumCPU;
}

int CMyVideoCapture::GetDelay()
{
	return m_iDelay;
}

// 현재 생생된 스레드를 종료하고 동적 생성된 커스텀 스레드 클래스를 해재 함.
void CMyVideoCapture::ReleaseThread()
{
	int iHalfThread;
	int i;

	if (m_iNumCPU > 3) {
		iHalfThread = m_iNumCPU / 2;
	}
	else {
		iHalfThread = 2;
	}

	if (m_arThReadV) {
		for (i = 0; i < iHalfThread; i++) {
			m_arThReadV[i].SetEnableThread(false);
		}
		Sleep(200);
		delete[] m_arThReadV;
		m_arThReadV = nullptr;
	}

	if (m_arThDrawV) {
		for (i = 0; i < 2; i++) {
			m_arThDrawV[i].SetEnableThread(false);
		}
		Sleep(200);
		delete[] m_arThDrawV;
		m_arThDrawV = nullptr;
	}

	m_iNowReadVideo = 0;
	m_iNowDrawVideo = 0;

	m_bEndPlay = false;
}
