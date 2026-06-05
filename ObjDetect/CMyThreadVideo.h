#pragma once

class CMyVideoCapture;

class CMyThreadVideo
{
public:
	CMyThreadVideo();

	void SetParams(CMyVideoCapture* pVC, int iThreadIdx);
	void SetEnableThread(bool bEnable);

	static UINT ThreadForReadVideo(LPVOID pParam);
	static UINT ThreadForDrawVideo(LPVOID pParam);

	HANDLE m_hThread; // 스레드 핸들 저장용 변수 추가

private:
	CMyVideoCapture* m_pVC;
	int m_iThreadIdx;
	bool m_bThreadEnable;
};
