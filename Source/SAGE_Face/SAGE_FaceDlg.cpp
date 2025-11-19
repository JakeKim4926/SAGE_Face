
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

	BtnLayout();

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

	if (pBtnOpen && ::IsWindow(pBtnOpen->GetSafeHwnd())) {
		pBtnOpen->MoveWindow(xStart, y, btnWidth, btnHeight);
	}

	if (pBtnClose && ::IsWindow(pBtnClose->GetSafeHwnd())) {
		pBtnClose->MoveWindow(xStart + btnWidth + btnGap, y, btnWidth, btnHeight);
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

	if (m_pGrabThread) {
		WaitForSingleObject(m_pGrabThread->m_hThread, 1000);
		m_pGrabThread = nullptr;
		m_cam.Close();
	}

	CDialog::OnDestroy();
}

void CSAGEFaceDlg::OnSize(UINT nType, int cx, int cy) {
	CDialogEx::OnSize(nType, cx, cy);
}
