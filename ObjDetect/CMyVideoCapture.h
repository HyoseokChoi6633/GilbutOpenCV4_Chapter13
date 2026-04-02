#pragma once

#include <queue>
#include "CMyMat.h"

class CMyThreadVideo;

typedef struct tag_STReadMat {
	CMyMat imgVideo;
	int iIdx;
} STReadMat;

class CMyVideoCapture
{
public:
	CMyVideoCapture();
	~CMyVideoCapture();

	bool OnOpenVideo(int iDeviceID);
	bool OnOpenVideo(CString strVideoFile);

	void SetPicCtrl(CStatic* pstPic);

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

	CStatic* m_pstPic;
	VideoCapture m_cvCap;

	queue<unique_ptr<STReadMat>> m_qVideo;

	CMyThreadVideo* m_arThReadV;
	CMyThreadVideo* m_arThDrawV;
};
