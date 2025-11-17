
// SAGE_FaceDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "SAGE_Face.h"
#include "SAGE_FaceDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx {
public:
	CAboutDlg();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CSAGEFaceDlg 대화 상자



CSAGEFaceDlg::CSAGEFaceDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SAGE_FACE_DIALOG, pParent) {
}

void CSAGEFaceDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_CAM, m_picCam);
}

BEGIN_MESSAGE_MAP(CSAGEFaceDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()

	ON_MESSAGE(WM_UPDATE_FRAME, &CSAGEFaceDlg::OnUpdateFrame)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CSAGEFaceDlg 메시지 처리기


BOOL CSAGEFaceDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	bool bOpen = m_cam.Open(0, CAM_WIDTH, CAM_HEIGHT, CAM_FPS);
	if (!bOpen) {
		AfxMessageBox(_T("Failed to connect cam"));
	} else {
		m_bGrabRun = TRUE;
		m_pGrabThread = AfxBeginThread(GrabThreadProc, this);
	}


	return TRUE;
}

UINT CSAGEFaceDlg::GrabThreadProc(LPVOID pParam) {
	auto* pDlg = reinterpret_cast<CSAGEFaceDlg*>(pParam);
	cv::Mat localFrame;

	while (pDlg->m_bGrabRun) {
		if (!pDlg->m_cam.GrabFrame(localFrame)) {
			break;
		}

		if (localFrame.empty()) {
			continue;
		}

		//pDlg->m_csFrame.Lock();
		//localFrame.copyTo(pDlg->m_frame);
		//pDlg->m_csFrame.Unlock();
		{
			CSingleLock lock(&pDlg->m_csFrame, TRUE);
			localFrame.copyTo(pDlg->m_frame);
		}

		pDlg->PostMessage(WM_UPDATE_FRAME, 0, 0);

		Sleep(1);
	}

	return 0;
}

void CSAGEFaceDlg::DrawMatToPicture(const cv::Mat& mat) {
	if (mat.empty())
		return;

	CRect rect;
	m_picCam.GetClientRect(&rect);
	int dstW = rect.Width();
	int dstH = rect.Height();

	CClientDC dc(&m_picCam);

	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = mat.cols;
	bmi.bmiHeader.biHeight = -mat.rows;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;

	StretchDIBits(
		dc.GetSafeHdc(),
		0, 0, dstW, dstH,
		0, 0, mat.cols, mat.rows,
		mat.data,
		&bmi,
		DIB_RGB_COLORS,
		SRCCOPY
	);
}

LRESULT CSAGEFaceDlg::OnUpdateFrame(WPARAM wParam, LPARAM lParam) {
	cv::Mat frameCopy;

	{
		CSingleLock lock(&m_csFrame, TRUE);
		if (m_frame.empty())
			return 0;
		m_frame.copyTo(frameCopy);
	}

	//int type = frameCopy.type();
	//int depth = type & CV_MAT_DEPTH_MASK;
	//int channels = 1 + (type >> CV_CN_SHIFT);
	//TRACE("frameCopy type=%d, depth=%d, channels=%d, size=%d x %d\n",
	//	type, depth, channels, frameCopy.cols, frameCopy.rows);


	DrawMatToPicture(frameCopy);
	return 0;
}


BOOL CSAGEFaceDlg::PreTranslateMessage(MSG* pMsg) {
	if (pMsg->message == WM_KEYDOWN) {
		bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
		if (ctrlDown) {
			if (pMsg->wParam == 'A') {
				// OPENCV TEST
				cv::Mat temp = cv::imread("sample.bmp");
				cv::imshow("test sample", temp);
			} else if (pMsg->wParam == 'S') {
				// CAM TEST
				Cam cam;
				bool bResult = cam.Open(0, CAM_WIDTH, CAM_HEIGHT, CAM_FPS);

				if (!bResult)
					return TRUE;

				cam.showVideo();
				cam.Close();
			}

			return TRUE;
		}
	}

	// 나머지는 기본 동작
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CSAGEFaceDlg::OnSysCommand(UINT nID, LPARAM lParam) {
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else {
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CSAGEFaceDlg::OnPaint() {
	if (IsIconic()) {
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);

		CRect rect;
		GetClientRect(&rect);

		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

	} else {
		CDialogEx::OnPaint();
	}
}


void CSAGEFaceDlg::OnDestroy() {
	CDialogEx::OnDestroy();
	
	m_bGrabRun = FALSE;

	if (m_pGrabThread) {
		WaitForSingleObject(m_pGrabThread->m_hThread, 1000);
		m_pGrabThread = nullptr;
	}

	CDialog::OnDestroy();
}
