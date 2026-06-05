#pragma once

#include <queue>
#include "CMyMat.h"
#include "COpenGLControl.h"

#include <atomic>

class CMyThreadVideo;

typedef struct tag_STReadMat {
	CMyMat imgVideo;
	int iIdx;
} STReadMat, *LPSTReadMat;

typedef struct tag_TrackedObj {
	int id;
	cv::Rect rect;
	int missingFrames = 0; // 프레임에서 사라졌을 때 카운트
} TrackedObj;

class CMyVideoCapture
{
public:
	CMyVideoCapture();
	~CMyVideoCapture();

	bool OnOpenVideo(int iDeviceID);
	bool OnOpenVideo(CString strVideoFile);

	void SetPicCtrl(COpenGLControl* pstPic);

	bool CreateThreadForVideo();

	int m_iNowReadVideo;
	ULONGLONG m_qwStartTime;

	Mat CpyFrame();
	bool WorkFrameToQueue(Mat& frame, int iReadIdx);
	bool DispQueueData(int iDrawIdx, bool fRatio);

	int GetNumCPU();
	double GetDelay();

	void ReleaseThread();

	void SetHogSkipFrame(bool bSkipFrame);
	void SetShowFPS(bool bShowFPS);

	bool GetVideoEmpty();
	LPSTReadMat GetVideoFrontPtr();
	int GetReadThreadCnt();

	// 1. Read(생산자) 스레드 그룹 간의 순서 정렬용 세트 (기존 m_muReadV 대체)
	std::mutex              m_readMtx;
	std::condition_variable m_cvReadOrder;

	// 2. Draw(화면 표시) 스레드 그룹 간의 순서 정렬용 세트 (기존 m_muDrawV 대체)
	std::mutex              m_drawMtx;
	std::condition_variable m_cvDrawOrder;

	std::mutex m_captureMtx; // 캡처 전용 뮤텍스 추가

	atomic<bool> m_bVideoProcessingEnable;

	std::atomic<bool> m_bPause;

	void SetMuxNotify();

	bool ReloadVideo();
	void SetEndPlay(bool bEndPlay);

protected:
	// FPS 계산을 위한 멤버 변수들
	double m_dFinalFPS;          // 최종 FPS 값
	ULONGLONG m_qwLastFPSTime = 0;     // 마지막 측정 기준 시간

private:
	bool m_bQRScan;
	int m_iNumCPU;
	double m_dFPS;
	double m_dDelay;
	std::atomic<bool> m_bEndPlay;

	// 일반 bool 대신 atomic을 사용합니다.
	std::atomic<bool> m_bTimeOver;
	bool m_bHogSkipFrame;
	bool m_bShowFPS;

	// 실시간 FPS 계산을 위한 변수들
	ULONGLONG m_qwLastRenderTime; // 직전 프레임이 출력된 시간
	double    m_dRealRenderFPS;  // 최종 계산된 실시간 FPS

	vector<Rect> m_vecLastDetected;

	string m_strImgFile_a;

	HOGDescriptor m_hog;
	QRCodeDetector m_detector;

	// CStatic* m_pstPic;
	COpenGLControl* m_pWndPicGL;
	VideoCapture m_cvCap;

	queue<unique_ptr<STReadMat>> m_qVideo;

	// 기존 포인터 배열 대신 vector와 unique_ptr 사용
	std::vector<std::unique_ptr<CMyThreadVideo>> m_vThReadV;
	std::vector<std::unique_ptr<CMyThreadVideo>> m_vThDrawV;

	// CMyThreadVideo* m_arThReadV;
	// CMyThreadVideo* m_arThDrawV;

	cv::Scalar GetColorForID(int id);


	// 클래스 멤버 변수
	std::vector<TrackedObj> m_trackedList;
	int m_nextID = 1;

	// 1. 두 사각형의 겹침 정도(0.0 ~ 1.0)를 계산
	float GetIoU(Rect a, Rect b);

	// 2. ID를 할당하는 핵심 함수
	void AssignIDs(vector<Rect>& detected, vector<int>& assignedIDs);

	void ResetTracker();
};
