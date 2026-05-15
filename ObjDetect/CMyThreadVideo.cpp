#include "pch.h"
#include "CMyThreadVideo.h"
#include "CMyVideoCapture.h"
#include <mutex>

CMyThreadVideo::CMyThreadVideo()
{
}

void CMyThreadVideo::SetParams(CMyVideoCapture* pVC, int iThreadIdx)
{
	m_pVC = pVC;					// 커스텀 비디오 캡쳐가 있는 클래스 포인터 저장
	m_iThreadIdx = iThreadIdx;		// 스레드 생성시 인덱스 번호 저장 (0 에서 4 이상의 논리 코어의 반수)
}

void CMyThreadVideo::SetEnableThread(bool bEnable)
{
	// 스레드의 반복문을 유지해 주는 변수 설정
	m_bThreadEnable = bEnable;
}

// 비디오 캡쳐를 읽어오는 스레드
UINT CMyThreadVideo::ThreadForReadVideo(LPVOID pParam)
{
	CMyThreadVideo* pThreadV = (CMyThreadVideo*)pParam;
	CMyVideoCapture* pVC = pThreadV->m_pVC;

	int iNumCPU = pVC->GetNumCPU();					// 논리 스레드의 수를 가지고 온다.
	iNumCPU = iNumCPU < 4 ? 2 : iNumCPU / 2;		// 3이하이면 읽기 스레드는 2로 한다. 4 이상이면 받은 값의 반을 사용한다.

	bool bReadSuc;

	while (pThreadV->m_bThreadEnable) {
		// 뮤택스를 이용하여 특정 프레임의 순서대로 스레드 인덱스의 순서와 맞추어 읽어오게 한다.
		Mat frame;
		bReadSuc = false;

		pVC->m_muReadV.Lock();

		if (pVC->m_iNowReadVideo == pThreadV->m_iThreadIdx) {	// 스레드 인덱스와 현재 읽어올 비디오의 프레임 번호가 같은지 확인한다.
			// 현재 비디오 프레임과 스레드 인덱스가 같다면
			frame = pVC->CpyFrame();
			bReadSuc = true;
		}

		// 뮤택스를 빠져나온다.
		pVC->m_muReadV.Unlock();

		if (bReadSuc && pVC->WorkFrameToQueue(frame, pThreadV->m_iThreadIdx)) {	// Mat을 저장해 주는 Queue에 순번과 VideoCapute에서 받은 Mat 데이터를 저장한다.
			pVC->m_iNowReadVideo++;							// 순번을 올린다.
			pVC->m_iNowReadVideo %= iNumCPU;				// 계산된 값으로 잉여연산을 한다.
			bReadSuc = true;
		}
		else {
			bReadSuc = false;
		}
	}

	return 0;
}

// 비디오 재생(화면 표시) 스레드
UINT CMyThreadVideo::ThreadForDrawVideo(LPVOID pParam)
{
	CMyThreadVideo* pThreadV = (CMyThreadVideo*)pParam;
	CMyVideoCapture* pVC = pThreadV->m_pVC;

	int iSleep = pVC->GetDelay();

	bool bDispSuc;

	ULONGLONG llTimeStart, llTimeEnd;
	LONGLONG llGap;

	while (pThreadV->m_bThreadEnable) {
		bDispSuc = false;

		llTimeStart = GetTickCount64();
		// 비디오 재생용 뮤택스 접근
		pVC->m_muDrawV.Lock();

		if (pVC->DispQueueData(pThreadV->m_iThreadIdx, true)) {	// Queue에 있는 Mat 데이터를 홀짝 순서로 2개의 스레드를 사용하여 표시한다.
			pVC->m_iNowDrawVideo++;				// 현재 그리기를 한후 다음 페이지의 데이터를 출력하기 위해 인덱스를 증가시킨다.
			pVC->m_iNowDrawVideo %= 2;			// 화면에 그리기는 0, 1 로 홀짝 스레드

			bDispSuc = true;
		}

		pVC->m_muDrawV.Unlock();
		
		// 2. 시간차 계산 및 지연
		if (bDispSuc) {
			llTimeEnd = GetTickCount64();
			llGap = llTimeEnd - llTimeStart;
			int iSleepTime = iSleep - (int)llGap;

			if (iSleepTime > 0) {
				Sleep(iSleepTime);
			}
		}
		else {
			// 내 차례가 아니거나 데이터가 없으면 짧게 쉬고 다시 확인
			Sleep(5);
		}
	}

	return 0;
}
