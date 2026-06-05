#include "pch.h"
#include "CMyThreadVideo.h"
#include "CMyVideoCapture.h"
#include <mutex>

CMyThreadVideo::CMyThreadVideo() :
	m_hThread(nullptr)
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

	static int iResetCnt = 0;

	while (pThreadV->m_bThreadEnable && pVC->m_bVideoProcessingEnable) {
		Mat frame = pVC->CpyFrame();
		
		if (frame.empty()) {
			Sleep(1); // 프레임 획득 실패 시 아주 잠시 대기

			continue;
		}

		// 2. STL 표준 동기화 구역 진입
		// WorkFrameToQueue 내부에서 다시 락을 잡으면 데드락이 발생하므로, 
		// 여기서는 락을 잡지 않고 바로 작업을 던집니다.
		// WorkFrameToQueue 내부의 wait/notify 구조가 모든 순서를 책임집니다!

		if (pVC->WorkFrameToQueue(frame, pThreadV->m_iThreadIdx)) {
			// WorkFrameToQueue 내부에서 이미 순서 인덱스 증가 및 
			// notify_all()까지 완벽하게 처리되므로 여기서는 아무것도 할 필요가 없습니다.
		}
		else {
			// 작업 실패 혹은 종료 신호 수신 시 아주 짧게 쉬어줍니다.
			Sleep(1);
		}
	}

	return 0;
}

// 비디오 재생(화면 표시) 스레드
UINT CMyThreadVideo::ThreadForDrawVideo(LPVOID pParam)
{
	CMyThreadVideo* pThreadV = (CMyThreadVideo*)pParam;
	CMyVideoCapture* pVC = pThreadV->m_pVC;

	// [정석 교정 1] 비디오의 원래 1프레임당 골든타임을 온전하게 가져옵니다.
	// 예: 10 FPS -> 100ms, 30 FPS -> 33.33ms
	double dSleep = pVC->GetDelay();

	bool bDispSuc;

	// [교정] 락 대기 시간에 오염되지 않는 스레드 고유의 기준점 변수
	ULONGLONG qwLastDrawTime = ::GetTickCount64();

	while (pThreadV->m_bThreadEnable && pVC->m_bVideoProcessingEnable) {
		bDispSuc = false;

		// 1. STL 표준 락(unique_lock) 획득
		std::unique_lock<std::mutex> drawLock(pVC->m_drawMtx);

		// 2. 조건 변수를 이용한 대기 (CPU 점유율 0% 구간)
		pVC->m_cvDrawOrder.wait(drawLock, [pThreadV, pVC] {
			// 1. 종료 신호 처리
			if (!pVC->m_bVideoProcessingEnable || !pThreadV->m_bThreadEnable) return true;

			// 2. [수정] Pause 상태면 false 리턴하여 대기
			if (pVC->m_bPause) return false;

			// 3. 데이터가 있을 때만 인덱스 검사
			if (pVC->GetVideoEmpty()) return false;

			// 변수에 의존하지 말고, 데이터의 인덱스만 확인합니다.
			return (pVC->GetVideoFrontPtr()->iIdx % 2 == pThreadV->m_iThreadIdx);
			});

		// 대기 해제 후 종료 조건 체크
		if (!pThreadV->m_bThreadEnable || !pVC->m_bVideoProcessingEnable) {
			break;
		}

		// wait 이후에 루프 내부에서 체크
		if (pVC->m_bPause) {
			continue; // 정지 상태면 아래 그리기 로직 건너뛰고 다시 wait로 진입
		}

		// 3. 화면 그리기 수행
		if (pVC->DispQueueData(pThreadV->m_iThreadIdx, true)) {	// Queue에 있는 Mat 데이터를 홀짝 순서로 2개의 스레드를 사용하여 표시한다.

			bDispSuc = true;

			// [중요] 소비자(Draw)가 큐를 비웠으니 생산자(Read)에게 신호를 줍니다.
			// 큐가 가득 찼던 상황에서 읽기 스레드가 대기 중이었을 수 있으므로 필수!
			pVC->m_cvReadOrder.notify_all();
		}
		else {
			// 4. 락 해제 (그리기가 끝나면 즉시 풀어주어 생산자가 큐에 접근하게 함)
			drawLock.unlock();

			pVC->m_cvReadOrder.notify_all();

			continue;
		}

		// 4. 락 해제 (그리기가 끝나면 즉시 풀어주어 생산자가 큐에 접근하게 함)
		drawLock.unlock();
		
		// 2. 오차 없는 정밀 지연 제어
		if (bDispSuc) {
			ULONGLONG qwCurrentTime = ::GetTickCount64();
			// 1. 이미 기준시간보다 늦었다면, 기준시간을 현재 시간으로 강제 동기화 (오차 누적 방지)
			if (qwCurrentTime > qwLastDrawTime + (LONGLONG)dSleep) {
				qwLastDrawTime = qwCurrentTime;
			}

			// 2. 다음번 타겟 시간 계산
			qwLastDrawTime += (LONGLONG)dSleep;

			// 3. 남은 시간 계산
			LONGLONG iSleepTime = (LONGLONG)qwLastDrawTime - (LONGLONG)qwCurrentTime;

			// 4. Sleep 처리
			if (iSleepTime > 0) {
				Sleep((DWORD)iSleepTime);
			}

			// 내가 큐에서 하나를 소비했으므로, 
			// 혹시라도 데이터를 기다리던 다른 Draw 스레드나 
			// 큐의 상태를 확인해야 할 Read 스레드를 모두 깨워줍니다.
			pVC->m_cvDrawOrder.notify_all();
		}
	}

	return 0;
}
