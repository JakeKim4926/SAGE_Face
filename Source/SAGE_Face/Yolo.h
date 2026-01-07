#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <vector>
#include <string>

struct YoloResult {
    cv::Rect box;
    int      classId;
    float    confidence;
};

class Yolo {
public:
    Yolo();

    BOOL    LoadModel(const std::string& onnxPath, BOOL useCuda = FALSE);
    BOOL    IsLoaded() const { return m_bLoaded; }

    void    Detect(const cv::Mat& image, std::vector<YoloResult>& results,float confThreshold = 0.25f, float nmsThreshold = 0.45f);
    void    DrawDetections(const cv::Mat& image, cv::Mat& output, const std::vector<YoloResult>& results);

private:
    cv::dnn::Net m_net;

    bool m_bLoaded;
    int m_nInputW;
    int m_nInputH;
    int m_nNumClasses;

    void ParseDetections(const cv::Mat& out,
        float confThreshold,
        int imgW, int imgH,
        std::vector<cv::Rect>& boxes,
        std::vector<float>& confidences,
        std::vector<int>& classIds);
};