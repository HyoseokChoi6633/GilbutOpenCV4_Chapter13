#include "pch.h"
#include "COpenGLControl.h"

BEGIN_MESSAGE_MAP(COpenGLControl, CStatic)
	ON_WM_PAINT()
	ON_WM_DESTROY() // 해제 루틴도 잊지 마세요!
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

COpenGLControl::COpenGLControl() : m_hDC(nullptr), m_hRC(nullptr), m_nTexID(0), m_nLastW(0), m_nLastH(0), m_ShaderProgram(0), m_VAO(0), m_VBO(0), m_bUseGL(false), m_pDrawMux(nullptr)
{
}

void COpenGLControl::InitGL()
{
	// [핵심] 여기서 딱 한 번만 이 윈도우의 고유 DC를 발급받아 멤버 변수에 박아둡니다.
	m_hDC = ::GetDC(this->GetSafeHwnd());

	if (!m_hDC)
		return;

	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd)); // 0으로 깨끗하게 초기화
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	SetPixelFormat(m_hDC, ChoosePixelFormat(m_hDC, &pfd), &pfd);
	m_hRC = wglCreateContext(m_hDC);

	// 첫 텍스처 이름 생성
	wglMakeCurrent(m_hDC, m_hRC);

	// [중요] GLEW 초기화 (RC 생성 후에 호출해야 함)
	glewInit();

	// 셰이더 정의 및 셰이더 프로그램 생성
	m_ShaderProgram = LoadShaders(".\\ShaderCode\\shader.vert", ".\\ShaderCode\\shader.frag");
	
	// 3. 버퍼 설정(VAO, VBO) - 화면 전체를 채우는 사각형
	// 초기 데이터 (공간 확보용)
	float vertices[] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f
	};

	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));
	glEnableVertexAttribArray(1);

	// 4. 텍스처 준비
	glGenTextures(1, &m_nTexID);
	glBindTexture(GL_TEXTURE_2D, m_nTexID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	wglMakeCurrent(nullptr, nullptr);
}

void COpenGLControl::Render(const cv::Mat& img, bool fRatio)
{
	if (!m_bUseGL) {
		return;
	}

	if (img.empty() || !m_hRC) {
		return;
	}

	//// 2. [심폐소생술] 현재 스레드에 묶인 컨텍스트가 진짜 정상인지 체크합니다.
	//HGLRC hActiveRC = wglGetCurrentContext();

	//// 만약 묶여있지 않거나, 기존 m_hRC와 일치하지 않는다면 (첫 진입 시 발생하는 문제)
	//if (hActiveRC != m_hRC)
	//{
	//	// [수정] 실패하더라도 쉽게 포기하지 않고 한 번 더 컨텍스트를 확실히 리셋합니다.
	//	wglMakeCurrent(nullptr, nullptr);

	//	// 강제로 기존 끊어졌던 연결을 현재 활성화된 화면(hCurrentDC)에 다시 붙입니다.
	//	if (!wglMakeCurrent(m_hDC, m_hRC)) {			
	//		wglMakeCurrent(nullptr, nullptr);
	//		return;
	//	}
	//}

	wglMakeCurrent(m_hDC, m_hRC);
		
	// 1. 비율 유지 좌표 계산
	float vertices[] = {
		// 위치(x, y)		// 텍스처(u, v)
		1.0f, 1.0f,		1.0f, 1.0f,		// 우상
		1.0f, -1.0f,		1.0f, 0.0f,		// 우하
		-1.0f, -1.0f,		0.0f, 0.0f,		// 좌하
		-1.0f, 1.0f,		0.0f, 1.0f		// 좌상
	};

	if (fRatio) {
		CRect rt;
		GetClientRect(&rt);
		float winW = (float)rt.Width();
		float winH = (float)rt.Height();
		float imgW = (float)img.cols;
		float imgH = (float)img.rows;

		float winAspect = winW / winH;
		float imgAspect = imgW / imgH;

		float scaleX = 1.0f;
		float scaleY = 1.0f;

		if (imgAspect > winAspect) {
			// 이미지가 화면보다 더 넓은 경우 (상하에 여백)
			scaleY = winAspect / imgAspect;
		}
		else {
			// 이미지가 화면보다 더 긴 경우 (좌우에 여백)
			scaleX = imgAspect / winAspect;
		}

		// 계산된 scale을 좌표에 적용
		vertices[0] = scaleX; vertices[1] = scaleY;
		vertices[4] = scaleX; vertices[5] = -scaleY;
		vertices[8] = -scaleX; vertices[9] = -scaleY;
		vertices[12] = -scaleX; vertices[13] = scaleY;
	}

	// 2. VBO 데이터 업데이트 (매 프레임 좌표가 바뀔 수 있으므로)
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

	// 3. 텍스처 업데이트
	glBindTexture(GL_TEXTURE_2D, m_nTexID);

	// 채널에 따른 포맷 및 Swizzle 설정 (공통 로직)
	GLenum format;
	if (img.channels() == 1) {
		format = GL_RED;
		GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	}
	else {
		format = GL_BGR;
		GLint swizzleMask[] = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	}

	// 픽셀 정렬을 1바이트 단위로 하겠다고 설정 (밀림 현상 방지)
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// 크기 변경 여부에 따른 업데이트
	if (m_nLastW != img.cols || m_nLastH != img.rows) {
		// 메모리 재할당
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.cols, img.rows, 0, format, GL_UNSIGNED_BYTE, img.data);

		m_nLastW = img.cols;
		m_nLastH = img.rows;
	}
	else {
		// 데이터만 교체 (빠름)
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.cols, img.rows, format, GL_UNSIGNED_BYTE, img.data);
	}

	// 4. 최종 그리기
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);		// 배경은 검은색 (여백 부분)
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(m_ShaderProgram);
	glBindVertexArray(m_VAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	SwapBuffers(m_hDC);

	wglMakeCurrent(nullptr, nullptr);
}

void COpenGLControl::CleanupGL()
{
	// 1. OpenGL 컨텍스트 활성화
	// 자원을 삭제하려면 해당 RC가 현재 스레드에 연결되어 있어야 합니다.
	if (m_hRC)
	{
		wglMakeCurrent(m_hDC, m_hRC);

		// 2. GPU 자원 해제 (VBO, VAO, Shader, Texture)
		if (m_nTexID > 0)     glDeleteTextures(1, &m_nTexID);
		if (m_VBO > 0)        glDeleteBuffers(1, &m_VBO);
		if (m_VAO > 0)        glDeleteVertexArrays(1, &m_VAO);
		if (m_ShaderProgram > 0) glDeleteProgram(m_ShaderProgram);

		// 3. 렌더링 컨텍스트(RC) 삭제
		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(m_hRC);
		m_hRC = nullptr;
	}

	if (m_hDC) {
		::ReleaseDC(this->GetSafeHwnd(), m_hDC);
		m_hDC = nullptr;
	}
}

void COpenGLControl::SetUseGL(bool bUseGL)
{
	if (m_hRC == nullptr) {
		InitGL();
	}

	m_bUseGL = bUseGL;
}

bool COpenGLControl::GetUseGL() const
{
	return m_bUseGL;
}

void COpenGLControl::MuxDraw(bool bLock)
{
	if (!m_pDrawMux) {
		return;
	}

	if (bLock) {
		m_pDrawMux->Lock();
	}
	else {
		m_pDrawMux->Unlock();
	}
}

void COpenGLControl::SetMuxDraw(CMutex* pDrawMux)
{
	m_pDrawMux = pDrawMux;
}

std::string COpenGLControl::ReadShaderFile(const char* pFilePath)
{
	std::string content;
	std::ifstream fileStream(pFilePath, std::ios::in);

	if (!fileStream.is_open()) {
		OutputDebugString(_T("파일을 찾을 수 없습니다.\n"));
		return "";
	}

	std::stringstream sstr;
	sstr << fileStream.rdbuf();
	content = sstr.str();
	fileStream.close();

	return content;
}

GLuint COpenGLControl::LoadShaders(const char* pVertPath, const char* pFragPath)
{
	// 1. 파일 읽기
	std::string vertCode = ReadShaderFile(pVertPath);
	std::string fragCode = ReadShaderFile(pFragPath);

	if (vertCode.empty() || fragCode.empty()) {
		return 0;
	}

	const char* pVSource = vertCode.c_str();
	const char* pFSource = fragCode.c_str();

	GLint success;
	char infoLog[512] = { 0, };

	// 2. Vertex Shader 컴파일
	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &pVSource, nullptr);
	glCompileShader(vertex);

	// 컴파일 에러 체크
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex, 512, nullptr, infoLog);
		OutputDebugStringA(infoLog);
	}

	// 3. Fragment Shader 컴파일
	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &pFSource, nullptr);
	glCompileShader(fragment);

	// 컴파일 에러 체크
	if (!success) {
		glGetShaderInfoLog(fragment, 512, nullptr, infoLog);
		OutputDebugStringA(infoLog);
	}

	// 4. Shader Program 링크
	GLuint program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);

	// 링크 에러 체크
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, 512, nullptr, infoLog);
		OutputDebugStringA(infoLog);
	}

	// 정리 (링크 완료 후 개별 셰이더는 삭제 가능
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return program;
}

void COpenGLControl::OnPaint()
{
	// MFC의 표준 CPaint를 생성하여 내부적으로 BeginPaint/EndPaint가 호출되게 함
	CPaintDC dc(this);

	if (m_bUseGL) {

		// 이미지가 한 번이라도 로드된 적이 있고, RC가 초기화 되었다면 다시 그림
		if (m_hRC && m_nTexID > 0 && m_nLastW > 0) {
			wglMakeCurrent(dc.GetSafeHdc(), m_hRC);

			// 이전 Render에서 계산된 좌표가 VBO에 남아 있었으므로
			// 별도의 계산 없이 Clear와 Draw만 수행합니다.
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(m_ShaderProgram);
			glBindVertexArray(m_VAO);
			glBindTexture(GL_TEXTURE_2D, m_nTexID);

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			SwapBuffers(dc.GetSafeHdc());
			wglMakeCurrent(nullptr, nullptr);
		}
		else {
			// 출력할 이미지가 없을 때는 그냥 검은색으로 배경만 채움
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			SwapBuffers(dc.GetSafeHdc());
		}
	}
	else {
		// GDI를 이용해 외곽선 그리기
		CRect rt;
		GetClientRect(&rt);

		// 외곽선 그리기 (밝은 회색 혹은 점선)
		CPen pen(PS_SOLID, 1, RGB(100, 100, 100)); // 실선, 두께 1, 회색
		CPen* pOldPen = dc.SelectObject(&pen);

		// 사각형 테두리 그리기
		dc.MoveTo(rt.left, rt.top);
		dc.LineTo(rt.right - 1, rt.top);
		dc.LineTo(rt.right - 1, rt.bottom - 1);
		dc.LineTo(rt.left, rt.bottom - 1);
		dc.LineTo(rt.left, rt.top);

		dc.SelectObject(pOldPen);
	}
}

// .cpp 파일 메시지 맵에 ON_WM_ERASEBKGND() 추가 후 구현
BOOL COpenGLControl::OnEraseBkgnd(CDC* pDC)
{
	return TRUE; // 아무것도 하지 않음 (깜빡임 방지)
}

void COpenGLControl::OnDestroy()
{
	CleanupGL();

	CStatic::OnDestroy();
}