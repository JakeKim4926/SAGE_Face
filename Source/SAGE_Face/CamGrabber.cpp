#include "pch.h"
#include "CamGrabber.h"

CamGrabber::CamGrabber(Cam& cam)
    : m_cam(cam) {
}

CamGrabber::~CamGrabber() {
    Stop();
}

bool CamGrabber::Start() {
    if (m_pThread) return false;
    m_bRun = TRUE;
    m_pThread = AfxBeginThread(ThreadProc, this);
    return (m_pThread != nullptr);
}

void CamGrabber::Stop() {
    m_bRun = FALSE;
    if (m_pThread) {
        WaitForSingleObject(m_pThread->m_hThread, 1000);
        m_pThread = nullptr;
    }
}

UINT CamGrabber::ThreadProc(LPVOID pParam) {
    return reinterpret_cast<CamGrabber*>(pParam)->Run();
}

UINT CamGrabber::Run() {
    cv::Mat local;
    while (m_bRun) {
        if (!m_cam.GrabFrame(local))
            break;
        if (local.empty())
            continue;

        {
            CSingleLock lock(&m_csFrame, TRUE);
            local.copyTo(m_frame);
        }

        Sleep(1);
    }
    return 0;
}

bool CamGrabber::GetLatestFrame(cv::Mat& dst) {
    CSingleLock lock(&m_csFrame, TRUE);
    if (m_frame.empty())
        return false;

    m_frame.copyTo(dst);
    return true;
}