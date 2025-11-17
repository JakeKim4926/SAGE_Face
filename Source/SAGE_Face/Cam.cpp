#include "pch.h"
#include "Cam.h"

Cam::Cam() {
}

Cam::~Cam() {
	Close();
}

bool Cam::Open(int index, int width, int height, double fps) {
	if (m_vcCapture.isOpened())
		m_vcCapture.release();

	if (!m_vcCapture.open(index)) {
		AfxMessageBox(_T("failed to connect"));
		return false;
	}

	CString message;

	if (width < MIN_WIDTH || height < MIN_HEIGHT || width > MAX_WIDTH || height > MAX_HEIGHT) {
		message.Format(_T("이미지(영상)의 크기는 (%d,%d) ~ (%d,%d)으로 입력해주세요"),
			MIN_WIDTH,
			MIN_HEIGHT,
			MAX_WIDTH,
			MAX_HEIGHT
		);

		AfxMessageBox(message);
		return false;
	}

	if (fps < MIN_FRAME || fps > MAX_FRAME) {
		message.Format(_T("프레임은 %d ~ %d 값 사이로 입력해주세요"),
			MIN_FRAME,
			MAX_FRAME
		);

		AfxMessageBox(message);
		return false;
	}

	return m_vcCapture.isOpened();
}

void Cam::Close() {
	if (m_vcCapture.isOpened())
		m_vcCapture.release();
}

bool Cam::IsOpened() const {
	return m_vcCapture.isOpened();
}

bool Cam::GrabFrame(cv::Mat& frame) {
	if (!m_vcCapture.isOpened())
		return false;

	return m_vcCapture.read(frame);
}

bool Cam::showVideo() {
	cv::Mat frame;
	while (m_vcCapture.isOpened()) {
		if (!GrabFrame(frame)) {
			AfxMessageBox(_T("failed to grab frame"));
			break;
		}

		cv::imshow("video test", frame);

		int key = cv::waitKey(1);
		if (key == CV_ESC) {
			AfxMessageBox(_T("stop video")); 
			break;
		}
	}

	return true;
}
