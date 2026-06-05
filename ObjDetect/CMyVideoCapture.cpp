#include "pch.h"
#include "CMyVideoCapture.h"
#include "CMyMat.h"
#include "CMyThreadVideo.h"

CMyVideoCapture::CMyVideoCapture() :
	m_bQRScan(false),
	m_qwStartTime(0),
	m_bTimeOver(false),
	m_bHogSkipFrame(false),
	m_bShowFPS(true),
	m_qwLastRenderTime(0),
	m_dRealRenderFPS(0.0),
	m_bVideoProcessingEnable(true),
	m_bPause(true),
	m_qwLastFPSTime(0),
	m_dFinalFPS(0)

{
	// CPU 논리 스레드 수를 반환
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);

	m_iNumCPU = sysinfo.dwNumberOfProcessors;

	ReleaseThread();
	m_qwLastFPSTime = ::GetTickCount64();
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
		m_dDelay = 1000.0 / m_dFPS;
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
	m_dDelay = 1000 / m_dFPS;

	return bRetVal;
}

void CMyVideoCapture::SetPicCtrl(COpenGLControl* pstPic)
{
	m_pWndPicGL = pstPic;
	m_pWndPicGL->SetMuxDraw(&m_drawMtx);
}

bool CMyVideoCapture::CreateThreadForVideo()
{
	bool bRetVal = false;
	int iHalfThread;
	int i;

	ReleaseThread();

	m_bPause = true;

	m_bVideoProcessingEnable = true;

	// 논리 코어 4개 이상
	iHalfThread = GetReadThreadCnt();

	// VideoCapture의 읽기 다중 스레드 생성
	m_vThReadV.reserve(iHalfThread);

	// 모션 캡쳐 일때 활성화
	if (!m_bQRScan) {
		m_hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
	}

	// VideoCapture의 읽기 다중 스레드를 활성화
	for (i = 0; i < iHalfThread; i++) {
		m_vThReadV.push_back(std::make_unique<CMyThreadVideo>());

		m_vThReadV[i]->SetParams(this, i);
		m_vThReadV[i]->SetEnableThread(true);

		// 스레드 시작 및 포인터 저장
		CWinThread* pThread = AfxBeginThread(m_vThReadV[i]->ThreadForReadVideo, m_vThReadV[i].get());

		if (pThread) {
			// CWinThread 객체에서 실제 OS 핸들을 추출하여 저장
			m_vThReadV[i]->m_hThread = pThread->m_hThread;
		}
	}

	// Queue에 저장된 Mat 데이터를 출력시 2개의 스레드를 사용하여 번갈아 출력한다.
	// VideoCapture의 읽기 다중 스레드 생성
	m_vThDrawV.reserve(2);
	for (i = 0; i < 2; i++) {
		m_vThDrawV.push_back(std::make_unique<CMyThreadVideo>());

		m_vThDrawV[i]->SetParams(this, i);
		m_vThDrawV[i]->SetEnableThread(true);
		AfxBeginThread(m_vThDrawV[i]->ThreadForDrawVideo, m_vThDrawV[i].get());
	}

	bRetVal = true;

	return bRetVal;
}

// 단순히 VideoCapture에서 프레임을 얻어온다.
Mat CMyVideoCapture::CpyFrame()
{
	Mat frame;

	// 캡처 전용 뮤텍스를 여기서 확실하게 잡습니다.
	// m_readMtx는 '순서 관리용'이고, m_captureMtx는 'FFmpeg 충돌 방지용'으로 분리하는 게 좋습니다.
	std::lock_guard<std::mutex> lock(m_captureMtx);

	if (m_cvCap.isOpened()) {
		m_cvCap >> frame;
	}

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

	// [플래그 선언] 이번 턴에 HOG를 진짜 스킵했는지 기억하는 로직 변수
	bool bHogSkipped = false;

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
		if (!m_bHogSkipFrame) {
			m_bTimeOver.exchange(false);
		}

		if (m_bTimeOver.exchange(false) == true) {
			// 직전 스레드에서 지연이 발생했다고 알렸으므로, 
			// 이번 스레드는 무거운 HOG를 스킵하고 즉시 하단 큐 대기열로 진행합니다! (지연 0ms)
			bHogSkipped = true;
		}
		else {
			// 1. [핵심 교정] HOG 연산을 시작하기 직전의 현재 시간을 찍습니다.
			ULONGLONG qwStartHOG = ::GetTickCount64();

			// if (iReadIdx % 2 == 0) {
			// m_hog.detectMultiScale(frame, detected);
			m_hog.detectMultiScale(frame, detected, 0, Size(8, 8), Size(32, 32), 1.1, 2);
			// }

			// 2. [핵심 교정] HOG 연산에 순수하게 걸린 시간(소요 시간)을 계산합니다.
			ULONGLONG qwHOGDuration = ::GetTickCount64() - qwStartHOG;

			// 3. 1프레임당 허용 시간 계산 (예: 30fps -> 33.33ms)
			double fFrameDelayMs = GetDelay();

			// 4. 순수 연산 시간이 1프레임 골든타임을 초과했는지 직관적으로 비교합니다.
			// 만약 HOG 연산에 50ms가 걸렸다면, 다음 스레드가 들어올 때 스킵하라고 플래그를 켭니다.
			if ((double)qwHOGDuration > fFrameDelayMs) {
				m_bTimeOver = true;
			}
		}	
		// 여기서 바로 그리지 않고, 순서가 되었을 때 트래킹 후 그립니다.
		//for (Rect r : detected) {
		//	Scalar c = Scalar(rand() % 256, rand() % 256, rand() % 256);
		//	rectangle(frame, r, c, 3);
		//}
	}

	

	// -----------------------------------------------------------------
	// [순차 구간 - STL 표준화 및 외부 매개변수 제거 완료]
	// -----------------------------------------------------------------
	// 1. Read(생산자) 전용 표준 뮤텍스를 unique_lock으로 안전하게 획득합니다.
	std::unique_lock<std::mutex> readLock(m_readMtx);

	// 2. 내 읽기 차례(m_iNowReadVideo == iReadIdx)가 아니거나, 
	//    종료 신호(m_bVideoProcessingEnable == false)가 떨어지지 않는 한 CPU를 0%로 만들고 완전히 잠듭니다.
	//    기존의 번거롭던 while 루프와 Sleep(5)가 이 한 줄로 완전히 청소됩니다.
	m_cvReadOrder.wait(readLock, [this, iReadIdx] {
		// 1. 종료 신호가 들어왔거나 스레드가 중지되어야 한다면 무조건 통과(true)
		if (!m_bVideoProcessingEnable) return true;

		// 2. Pause 상태라면 무조건 대기(false)
		if (m_bPause) return false;

		// 3. 비디오가 열려 있지 않다면 대기(false)
		if (!m_cvCap.isOpened()) return false;

		// 4. 큐가 가득 찼다면 무조건 대기 (제어권 방어)
		if (m_qVideo.size() > 5) return false;

		// 5. 내 차례가 되었을 때만 통과(true) 
		return (m_iNowReadVideo == iReadIdx);
		});

	// 3. 잠에서 깨어난 이유가 프로그램 종료(Shutdown) 상황이라면 안전하게 바로 탈출합니다.
	if (!m_bVideoProcessingEnable) {
		return false;
	}

	// --- [순서가 보장된 임계 구역 내부 진입] 트래킹 및 그리기 진행 ---
	if (!m_bQRScan) {
		if (bHogSkipped) {
			// 다른 스레드가 가장 최근에 성공해서 저장해둔 최신 좌표를 그대로 복사해옵니다!
			detected = m_vecLastDetected;

			string label = "Hog Skip Frame Adapted";
			cv::Scalar redColor(0, 0, 255);
			Point ptPrt = Point(10, 20);
			putText(frame, label, ptPrt, FONT_HERSHEY_SIMPLEX, 0.6, redColor, 2);
		}
		else {
			// HOG 연산에 성공한 스레드라면, 내 결과를 공용 변수에 최신화(백업)해둡니다.
			m_vecLastDetected = detected;
		}

		// --- 트래킹 및 ID 사각형 그리기 로직 ---
		if (!detected.empty()) {
			vector<int> assignedIDs(detected.size());

			// 1. ID 부여 (순차성 보장된 구간이라 안전함)
			AssignIDs(detected, assignedIDs);

			// 2. 부여된 ID로 그리기
			for (size_t i = 0; i < detected.size(); i++) {
				int id = assignedIDs[i];
				Scalar c = GetColorForID(id);
				rectangle(frame, detected[i], c, 3);

				string label = "ID: " + to_string(id);
				putText(frame, label, detected[i].tl(), FONT_HERSHEY_SIMPLEX, 0.6, c, 2);
			}
		}
	}

	// 데이터 패키징 후 안전하게 큐에 Push
	smartDispframe->imgVideo = frame;
	smartDispframe->iIdx = iReadIdx;
	m_qVideo.push(move(smartDispframe));

	// 다음 읽기 순번으로 인덱스 토스 (0 -> 1 -> 2 -> 3 -> 0...)
	m_iNowReadVideo = (m_iNowReadVideo + 1) % GetReadThreadCnt();

	// 4. 생산자 작업을 마치고 락을 해제합니다.
	readLock.unlock();

	// 5. [징검다리 신호 송신]
	// 내 바로 뒤에서 순서를 기다리며 잠들어 있는 다음 'Read' 스레드들을 깨우고,
	m_cvReadOrder.notify_all();
	// 동시에 큐에 데이터가 들어오길 애타게 기다리던 화면 표시('Draw') 스레드도 함께 깨워줍니다.
	m_cvDrawOrder.notify_all();

	return true;
}

// Queue에 저장된 Mat 데이터를 출력
bool CMyVideoCapture::DispQueueData(int iDrawIdx, bool fRatio)
{
	bool bRetVal = true;

	// Queue에 저장된 데이터 출력
	if (!m_qVideo.empty()) {
		unique_ptr<STReadMat>& refSmartDispframe = m_qVideo.front();

		// 1. 내 차례일 때만 그리기
		if (refSmartDispframe->iIdx % 2 == iDrawIdx) {
			if (m_bShowFPS) {
				// [이동 평균 방식] 카운트 누적 없이 시간 차이로 계산
				ULONGLONG qwCurrentTime = ::GetTickCount64();
				double dFrameTime = (double)(qwCurrentTime - m_qwLastFPSTime) / 1000.0;

				// 10ms보다 짧은 시간 내에 너무 빨리 호출되는 경우 무시 (너무 높은 FPS 방지)
				if (dFrameTime > 0.01) {
					double dCurrentFPS = 1.0 / dFrameTime;
					// 이전 FPS와 현재 FPS를 섞어서 부드럽게 만듦 (0.1 가중치)
					m_dFinalFPS = (m_dFinalFPS * 0.9) + (dCurrentFPS * 0.1);
					m_qwLastFPSTime = qwCurrentTime;
				}

				// 프로그램 UI 디자인에 맞춰 텍스트 포맷팅 및 출력
				std::string strFps = cv::format("Render FPS: %.1f", m_dFinalFPS);

				cv::putText(refSmartDispframe->imgVideo.GetMat(), strFps, cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
			}
			refSmartDispframe->imgVideo.DispMat(m_pWndPicGL, fRatio);
			m_qVideo.pop();
		}
		else {
			// m_qVideo.pop();
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
	if (m_bEndPlay && m_qVideo.empty()) {
		TRACE(_T("반복 재생: 첫 프레임으로 이동 시도...\n"));

		// 1. 큐를 완전히 비웁니다! (이게 없으면 이전 루프의 데이터가 남아있습니다)
		std::queue<unique_ptr<STReadMat>> empty;
		std::swap(m_qVideo, empty);

		ResetTracker();

		// open() 대신 set()을 사용하여 0번 프레임으로 이동
		// CAP_PROP_POS_FRAMES: 프레임 인덱스 기반 위치 설정
		if (m_cvCap.isOpened()) {
			if (m_cvCap.set(cv::CAP_PROP_POS_FRAMES, 0)) {

				m_bEndPlay = false;

				m_iNowReadVideo = 0;

				// 모든 스레드 깨우기
				m_cvReadOrder.notify_all();

				TRACE(_T("반복 재생: 성공적으로 0번 프레임으로 이동 완료!\n"));
			}
			else {
				TRACE(_T("반복 재생: set() 실패 (일부 코덱은 지원하지 않을 수 있음)\n"));
			}
		}
	}

	return bRetVal;
}

// 논리 스레드 수 반환
int CMyVideoCapture::GetNumCPU()
{
	return m_iNumCPU;
}

double CMyVideoCapture::GetDelay()
{
	return m_dDelay;
}

// 현재 생생된 스레드를 종료하고 동적 생성된 커스텀 스레드 클래스를 해재 함.
void CMyVideoCapture::ReleaseThread()
{
	m_bPause = false;                 // 대기 중인 스레드를 깨우기 위해 해제

	// 0. 종료 신호 전송 (가장 먼저)
	m_bVideoProcessingEnable = false; // 스레드 루프 탈출 조건

	// 1. 모든 스레드가 wait에서 깨어나도록 조건 변수 호출 (중요!)
	m_cvReadOrder.notify_all();
	m_cvDrawOrder.notify_all();

	if (!m_vThReadV.empty()) {
		for (auto& pThread : m_vThReadV) {
			if (pThread) pThread->SetEnableThread(false);
		}
	}

	// 2. 스레드 종료 대기 (메모리 해제 전 필수!)
	for (auto& pThread : m_vThReadV) {
		if (pThread && pThread->m_hThread) {
			::WaitForSingleObject(pThread->m_hThread, INFINITE);
			pThread->m_hThread = nullptr;
		}
	}

	if (!m_vThDrawV.empty()) {
		for (auto& pThread : m_vThDrawV) {
			if (pThread) pThread->SetEnableThread(false);
		}
	}

	// 2. 스레드 종료 대기 (메모리 해제 전 필수!)
	for (auto& pThread : m_vThDrawV) {
		if (pThread && pThread->m_hThread) {
			::WaitForSingleObject(pThread->m_hThread, INFINITE);
			pThread->m_hThread = nullptr;
		}
	}

	m_iNowReadVideo = 0;

	m_bEndPlay = false;

	// 4. 스마트 포인터 배열(벡터) 비우기
	// 벡터를 clear하면 내부의 unique_ptr들이 자동으로 delete를 호출합니다.
	m_vThReadV.clear();
	m_vThDrawV.clear();
}

void CMyVideoCapture::SetHogSkipFrame(bool bHogSkipFrame)
{
	m_bHogSkipFrame = bHogSkipFrame;
}

void CMyVideoCapture::SetShowFPS(bool bShowFPS) {
	m_bShowFPS = bShowFPS;
}

bool CMyVideoCapture::GetVideoEmpty()
{
	return m_qVideo.empty();
}

LPSTReadMat CMyVideoCapture::GetVideoFrontPtr()
{
	return m_qVideo.front().get();
}

int CMyVideoCapture::GetReadThreadCnt()
{
	int iReadThreadCnt = m_iNumCPU / 2;

	if (iReadThreadCnt < 2) {
		iReadThreadCnt = 2;
	}

	return iReadThreadCnt;
}

void CMyVideoCapture::SetMuxNotify()
{
	// 1. 객체 자체가 유효한 상태인지, 탭 전환 중 종료된 객체는 아닌지 확인
	// m_bVideoProcessingEnable 플래그를 사용하여 객체가 '살아있는 상태'인지 검사합니다.
	if (!m_bVideoProcessingEnable) return;

	// 2. 조건 변수 호출 전 예외 방지 (try-catch)
	try {
		m_cvReadOrder.notify_all();
		m_cvDrawOrder.notify_all();
	}
	catch (...) {
		TRACE(_T("notify_all 호출 중 예외 발생!\n"));
	}
}

bool CMyVideoCapture::ReloadVideo()
{
	return m_bEndPlay && m_qVideo.empty();
}

void CMyVideoCapture::SetEndPlay(bool bEndPlay)
{
	m_bEndPlay = bEndPlay;
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
	// 1. ID 번호 초기화
	m_nextID = 1;

	// 2. 추적 중인 리스트 메모리까지 완전히 해제
	std::vector<TrackedObj>().swap(m_trackedList);

	// 3. 스킵 프레임용 최신 좌표 백업본도 함께 안전하게 밀어줍니다.
	std::vector<cv::Rect>().swap(m_vecLastDetected);
}
