#pragma once

#include <opencv2/opencv.hpp>

class Cam
{
public:
    Cam();
    ~Cam();

    bool Open(int index, int width = 0, int height = 0, double fps = 0.0);
    void Close();
    bool IsOpened() const;
    
    bool GrabFrame(cv::Mat& frame);

private:
    cv::VideoCapture m_vcCapture;
};