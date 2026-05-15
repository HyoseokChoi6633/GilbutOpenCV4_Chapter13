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

void CMyVideoCapture::SetPicCtrl(COpenGLControl* pstPic)
{
	m_pWndPicGL = pstPic;
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

	auto smartDispframe = make_unique<STReadMat>();

	vector<Rect> detected;
	
	string strInfo;

	// QR Scan시 사용되는 로직
	if (m_bQRScan) {
		vector<Point> points;
		strInfo = m_detector.detectAndDecode(frame, points);

		if (!strInfo.empty()) {
			polylines(frame, points, true, Scalar(0, 0, 255), 2);
			putText(frame, strInfo, Point(10, 30), FONT_HERSHEY_DUPLEX, 1, Scalar(0, 0, 255));
		}
	}
	// 모션 캡쳐시 사용되는 로직
	else {
		// if (iReadIdx % 2 == 0) {
			m_hog.detectMultiScale(frame, detected, 0, Size(8, 8), Size(32, 32), 1.1, 2);
		// }

		// 여기서 바로 그리지 않고, 순서가 되었을 때 트래킹 후 그립니다.
		//for (Rect r : detected) {
		//	Scalar c = Scalar(rand() % 256, rand() % 256, rand() % 256);
		//	rectangle(frame, r, c, 3);
		//}
	}

	// [순차 구간] 순서대로 Queue에 넣기
	bool bPushSuc = false;

	// smartDispframe->imgVideo = frame;	// VideoCapture에서 얻은 Mat 저장
	// smartDispframe->iIdx = iReadIdx;			// 스레드 인덱스 저장

	// Queue에 순서대로 작업한 이미지를 넣기 위해 반복문을 이용하여 순서대로 넣는다.
	while (!bPushSuc) {
		m_muDrawV.Lock();

		if (m_iNowReadVideo == iReadIdx) {
			// --- 트래킹 로직 삽입 구간 ---
			if (!m_bQRScan && !detected.empty()) {
				vector<int> assignedIDs(detected.size());

				// 1. ID 부여 (순차성 보장된 구간이라 안전함)
				AssignIDs(detected, assignedIDs);

				// 2. 부여된 ID로 그리기
				for (size_t i = 0; i < detected.size(); i++) {
					int id = assignedIDs[i];
					// ID에 기반한 고정 색상 함수 사용 (rand() 대신)
					Scalar c = GetColorForID(id);
					rectangle(frame, detected[i], c, 3);

					string label = "ID: " + to_string(id);
					putText(frame, label, detected[i].tl(), FONT_HERSHEY_SIMPLEX, 0.6, c, 2);
				}
			}
			// -------------------------
			
			smartDispframe->imgVideo = frame;
			smartDispframe->iIdx = iReadIdx;

			// Queue에 저장
			m_qVideo.push(move(smartDispframe));

			// 다음 프레임 번호로 갱신 (0->1->2->3->0...)
			m_iNowReadVideo = (m_iNowReadVideo + 1) % m_iNumCPU;
			bPushSuc = true;
			m_muDrawV.Unlock();
		}
		else {
			// 여기서 대기 시 점유율 발생. Condition Variable 됩 추천
			m_muDrawV.Unlock();
			Sleep(5);
		}
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
			refSmartDispframe->imgVideo.DispMat(m_pWndPicGL, fRatio);
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
		ResetTracker();
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

cv::Scalar CMyVideoCapture::GetColorForID(int id)
{
	// ID를 시드로 사용하여 결정론적(Deterministic) 랜덤 색상 생성
	// 0~255 사이에서 너무 어둡지 않게 범위를 조절하면 좋습니다.
	int r = (id * 123) % 255;
	int g = (id * 456) % 255;
	int b = (id * 789) % 255;

	return cv::Scalar(b, g, r);		// OpenCV는 BGR 순서
}

float CMyVideoCapture::GetIoU(Rect a, Rect b)
{
	int interArea = (a & b).area(); // 교집합 면적
	int unionArea = a.area() + b.area() - interArea; // 합집합 면적
	if (unionArea <= 0) return 0;
	return (float)interArea / unionArea;
}

void CMyVideoCapture::AssignIDs(vector<Rect>& detected, vector<int>& assignedIDs)
{
	vector<bool> matchedNew(detected.size(), false);

	// 기존에 추적하던 객체들 중에서 매칭 시도
	for (auto& oldObj : m_trackedList) {
		float bestIoU = 0.3f; // 최소 30%는 겹쳐야 같은 사람으로 인정
		int bestIdx = -1;

		for (int i = 0; i < detected.size(); i++) {
			if (matchedNew[i]) continue; // 이미 매칭된 박스는 패스

			float iou = GetIoU(oldObj.rect, detected[i]);
			if (iou > bestIoU) {
				bestIoU = iou;
				bestIdx = i;
			}
		}

		if (bestIdx != -1) {
			oldObj.rect = detected[bestIdx]; // 위치 업데이트
			oldObj.missingFrames = 0;
			matchedNew[bestIdx] = true;
			assignedIDs[bestIdx] = oldObj.id;
		}
		else {
			oldObj.missingFrames++; // 이번 프레임에선 안 보임
		}
	}

	// 매칭되지 않은 새로운 박스들에 새 ID 부여
	for (int i = 0; i < detected.size(); i++) {
		if (!matchedNew[i]) {
			TrackedObj newObj;
			newObj.id = m_nextID++;
			newObj.rect = detected[i];
			m_trackedList.push_back(newObj);
			assignedIDs[i] = newObj.id;
		}
	}

	// 10프레임 이상 사라진 객체는 리스트에서 삭제
	m_trackedList.erase(std::remove_if(m_trackedList.begin(), m_trackedList.end(),
		[](const TrackedObj& o) { return o.missingFrames > 10; }), m_trackedList.end());
}

void CMyVideoCapture::ResetTracker()
{
	m_muDrawV.Lock(); // 데이터 접근 중일 수 있으므로 락 권장

	// 1. ID 번호 초기화
	m_nextID = 1;

	// 2. 추적 중인 리스트 메모리까지 완전히 해제
	vector<TrackedObj>().swap(m_trackedList);

	m_muDrawV.Unlock();
}
