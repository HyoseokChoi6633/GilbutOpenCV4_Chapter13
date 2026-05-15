#pragma once

#include <queue>
#include "CMyMat.h"
#include "COpenGLControl.h"

class CMyThreadVideo;

typedef struct tag_STReadMat {
	CMyMat imgVideo;
	int iIdx;
} STReadMat;

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
	int m_iNowDrawVideo;

	Mat CpyFrame();
	bool WorkFrameToQueue(Mat& frame, int iReadIdx);
	bool DispQueueData(int iDrawIdx, bool fRatio);

	int GetNumCPU();
	int GetDelay();

	void ReleaseThread();

	// MFC Mutex를 사용하였다.
	// stl mutex는 7과 11에서 작동하지 않았다.
	// 10에서는 작동을 하였지만...
	CMutex m_muReadV;
	CMutex m_muDrawV;

private:
	bool m_bQRScan;
	int m_iNumCPU;
	double m_dFPS;
	int m_iDelay;
	bool m_bEndPlay;

	string m_strImgFile_a;

	HOGDescriptor m_hog;
	QRCodeDetector m_detector;

	// CStatic* m_pstPic;
	COpenGLControl* m_pWndPicGL;
	VideoCapture m_cvCap;

	queue<unique_ptr<STReadMat>> m_qVideo;

	CMyThreadVideo* m_arThReadV;
	CMyThreadVideo* m_arThDrawV;

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
