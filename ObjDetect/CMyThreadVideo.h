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

private:
	CMyVideoCapture* m_pVC;
	int m_iThreadIdx;
	bool m_bThreadEnable;
};
