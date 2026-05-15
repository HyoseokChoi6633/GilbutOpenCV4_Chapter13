#pragma once
#include <afxwin.h>
#include <GL/glew.h>
#include <opencv2/opencv.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <vector>

class COpenGLControl :
    public CStatic
{
private:
    HDC m_hDC;
    HGLRC m_hRC;
    GLuint m_nTexID;
    int m_nLastW, m_nLastH; // 이미지 크기 변경 감지용
    GLuint m_ShaderProgram;
    GLuint m_VAO, m_VBO;
    bool m_bUseGL;

public:
    COpenGLControl();

    // 1. 초기화: OnInitDialog 등에서 한 번만 호출
    void InitGL();

    // 2. 통합 출력 함수: 이미지 한 장이나 동영상 프레임이나 모두 이 함수로 통합
    void Render(const cv::Mat& img, bool fRatio = true);

    // 3. 해제: OnDestroy에서 호출
    void CleanupGL();

    void SetUseGL(bool bUseGL);
    bool GetUseGL() const;

protected:
    std::string ReadShaderFile(const char* pFilePath);
    GLuint LoadShaders(const char* pVertPath, const char* pFragPath);

    DECLARE_MESSAGE_MAP()
    afx_msg void OnPaint(); // 창이 가려졌다 나타날 때를 대비
    afx_msg void OnDestroy();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

