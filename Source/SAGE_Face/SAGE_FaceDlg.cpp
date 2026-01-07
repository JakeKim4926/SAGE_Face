
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


// CSAGEFaceDlg 대화 상자


CSAGEFaceDlg::CSAGEFaceDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SAGE_FACE_DIALOG, pParent) {
}

void CSAGEFaceDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_CAM, m_picCam);
}

BEGIN_MESSAGE_MAP(CSAGEFaceDlg, CDialogEx)
	ON_MESSAGE(WM_UPDATE_FRAME, &CSAGEFaceDlg::OnUpdateFrame)

	ON_BN_CLICKED(IDC_BUTTON_CAM_OPEN, &CSAGEFaceDlg::OnBnClickedCamButton)
	ON_BN_CLICKED(IDC_BUTTON_CAM_CLOSE, &CSAGEFaceDlg::OnBnClickedCamButton)

	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON_DETECT, &CSAGEFaceDlg::OnBnClickedButtonDetect)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CSAGEFaceDlg 메시지 처리기


BOOL CSAGEFaceDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	if (::IsWindow(m_picCam.GetSafeHwnd())) {
		m_picCam.SetWindowPos(
			nullptr,
			0, 0,
			800, 600,
			SWP_NOMOVE | SWP_NOZORDER
		);
	}

	ModifyStyle(WS_THICKFRAME | WS_MAXIMIZEBOX, 0);
	SetTimer(WM_TIMER_UI_MS, TIMER_MS, NULL);

	BtnLayout();
	Initialize();

	DumpOpenCVDll();

	return TRUE;
}

void CSAGEFaceDlg::Initialize() {
	std::string onnxPath = "jake_yolo.onnx";
	
	m_yolo = new Yolo();
	BOOL bLoaded = m_yolo->LoadModel(onnxPath, FALSE);
	if (!bLoaded || !m_yolo->IsLoaded())
		AfxMessageBox(_T("failed to laod .onnx"));
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


		if (pDlg->m_bDetectMode && pDlg->m_yolo && pDlg->m_yolo->IsLoaded()) {
			std::vector<YoloResult> dets;
			pDlg->m_yolo->Detect(localFrame, dets);

			CString s;
			s.Format(_T("dets = %d\n"), (int)dets.size());
			OutputDebugString(s);

			cv::Mat annotated;
			pDlg->m_yolo->DrawDetections(localFrame, annotated, dets);

			annotated.copyTo(localFrame);
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

	cv::Mat bgra;
	if (mat.channels() == 4) bgra = mat;
	else if (mat.channels() == 3) cv::cvtColor(mat, bgra, cv::COLOR_BGR2BGRA);
	else if (mat.channels() == 1) cv::cvtColor(mat, bgra, cv::COLOR_GRAY2BGRA);
	else return;

	if (!bgra.isContinuous())
		bgra = bgra.clone();

	CRect rect;
	m_picCam.GetClientRect(&rect);
	CClientDC dc(&m_picCam);

	BITMAPINFO bmi{};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = bgra.cols;
	bmi.bmiHeader.biHeight = -bgra.rows;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	StretchDIBits(
		dc.GetSafeHdc(),
		0, 0, rect.Width(), rect.Height(),
		0, 0, bgra.cols, bgra.rows,
		bgra.data,
		&bmi,
		DIB_RGB_COLORS,
		SRCCOPY
	);
}

BOOL CSAGEFaceDlg::BtnLayout() {

	if (!::IsWindow(m_picCam.GetSafeHwnd()))
		return FALSE;

	CRect rcPic;
	m_picCam.GetWindowRect(&rcPic);
	ScreenToClient(&rcPic);

	const int btnWidth = 150;
	const int btnHeight = 40;
	const int btnGap = 50;
	const int marginTop = 50;

	int totalWidth = btnWidth * 2 + btnGap;
	int xStart = rcPic.left + (rcPic.Width() - totalWidth) / 2;
	int y = rcPic.bottom + marginTop;

	CWnd* pBtnOpen = GetDlgItem(IDC_BUTTON_CAM_OPEN);
	CWnd* pBtnClose = GetDlgItem(IDC_BUTTON_CAM_CLOSE);
	CWnd* pBtnDetect = GetDlgItem(IDC_BUTTON_DETECT);

	if (pBtnOpen && ::IsWindow(pBtnOpen->GetSafeHwnd())) {
		pBtnOpen->MoveWindow(xStart, y, btnWidth, btnHeight);
	}

	if (pBtnClose && ::IsWindow(pBtnClose->GetSafeHwnd())) {
		pBtnClose->MoveWindow(xStart + btnWidth + btnGap, y, btnWidth, btnHeight);
	}

	if (pBtnDetect && ::IsWindow(pBtnDetect->GetSafeHwnd())) {
		int xDetect = xStart + (totalWidth - btnWidth) / 2;
		int yDetect = y + btnHeight + 50;
		pBtnDetect->MoveWindow(xDetect, yDetect, btnWidth, btnHeight);
	}

	return TRUE;
}

void CSAGEFaceDlg::OnBnClickedCamButton()
{
	UINT nID = GetCurrentMessage()->wParam;
	switch (nID)
	{
		case IDC_BUTTON_CAM_OPEN:
			if (!m_cam.Open(CAM_INDEX, CAM_WIDTH, CAM_HEIGHT, CAM_FPS)) {
				AfxMessageBox(_T("Failed to connect cam"));
			} else {
				m_bGrabRun = TRUE;
				m_pGrabThread = AfxBeginThread(GrabThreadProc, this);
			}

			break;

		case IDC_BUTTON_CAM_CLOSE:
			m_bGrabRun = FALSE;
			if (m_pGrabThread) {
				WaitForSingleObject(m_pGrabThread->m_hThread, 1000);
				m_pGrabThread = nullptr;
				m_cam.Close();
			}

			Invalidate(TRUE);
			break;
	}
}

LRESULT CSAGEFaceDlg::OnUpdateFrame(WPARAM wParam, LPARAM lParam) {
	cv::Mat frameCopy;

	{
		CSingleLock lock(&m_csFrame, TRUE);
		if (m_frame.empty())
			return 0;
		m_frame.copyTo(frameCopy);
	}

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
			}
			else if (pMsg->wParam == 'S') {
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

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CSAGEFaceDlg::OnSysCommand(UINT nID, LPARAM lParam) {

	CDialogEx::OnSysCommand(nID, lParam);
}

void CSAGEFaceDlg::OnPaint() {
	if (IsIconic()) {
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
	}
	else {
		CDialogEx::OnPaint();
	}
}


void CSAGEFaceDlg::OnDestroy() {
	CDialogEx::OnDestroy();

	m_bGrabRun = FALSE;
	m_bDetectMode = FALSE;
	
	m_cam.Close();

	if (m_pGrabThread) {
		WaitForSingleObject(m_pGrabThread->m_hThread, 1000);
		m_pGrabThread = nullptr;
	}

	KillTimer(WM_TIMER_UI_MS);

	delete m_yolo;

	CDialog::OnDestroy();
}

void CSAGEFaceDlg::OnSize(UINT nType, int cx, int cy) {
	CDialogEx::OnSize(nType, cx, cy);
}

void CSAGEFaceDlg::OnBnClickedButtonDetect() {
	m_bDetectMode = !m_bDetectMode;
}

void CSAGEFaceDlg::OnTimer(UINT_PTR nIDEvent) {
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nIDEvent == WM_TIMER_UI_MS) {
		if(m_bDetectMode)		GetDlgItem(IDC_BUTTON_DETECT)->SetWindowText(_T("DETECT OFF"));
		else					GetDlgItem(IDC_BUTTON_DETECT)->SetWindowText(_T("DETECT ON"));
	}

	CDialogEx::OnTimer(nIDEvent);
}
