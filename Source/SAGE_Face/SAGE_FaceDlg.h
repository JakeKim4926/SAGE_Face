
// SAGE_FaceDlg.h: 헤더 파일
//

#pragma once


// CSAGEFaceDlg 대화 상자
class CSAGEFaceDlg : public CDialogEx
{
// 생성입니다.
public:
	CSAGEFaceDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SAGE_FACE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

// 구현입니다.
protected:
	CWinThread*		 m_pGrabThread = nullptr;
	BOOL			 m_bGrabRun = FALSE;
					 
	CStatic			 m_picCam;
	Cam				 m_cam;
	
	cv::Mat			 m_frame;
	CCriticalSection m_csFrame;

protected:
	static UINT		GrabThreadProc(LPVOID pParam);
	void			DrawMatToPicture(const cv::Mat& mat);
	BOOL			BtnLayout();
	
	afx_msg LRESULT OnUpdateFrame(WPARAM wParam, LPARAM lParam);
	afx_msg void	OnBnClickedCamButton();

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
public:
	
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
