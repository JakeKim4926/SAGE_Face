#pragma once
#include "Cam.h"

#include <opencv2/opencv.hpp>

class CamGrabber {
public:
    CamGrabber(Cam& cam);
    ~CamGrabber();

    bool Start();
    void Stop();

    bool GetLatestFrame(cv::Mat& dst);

private:
    static UINT ThreadProc(LPVOID pParam);
    UINT Run();

private:
    Cam& m_cam;
    CWinThread*      m_pThread = nullptr;
    BOOL             m_bRun = FALSE;

    cv::Mat          m_frame;
    CCriticalSection m_csFrame;
};