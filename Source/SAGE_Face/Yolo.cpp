#include "pch.h"
#include "Yolo.h"
#include <algorithm>
#include <cmath>

Yolo::Yolo()
    : m_bLoaded(false)
    , m_nInputW(640)
    , m_nInputH(640)
    , m_nNumClasses(80)
{
    //OutputDebugStringA(cv::getBuildInformation().c_str());
}

BOOL Yolo::LoadModel(const std::string& onnxPath, BOOL useCuda) {
    try {
        m_net = cv::dnn::readNetFromONNX(onnxPath);

        if (useCuda) {
            m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        } else {
            m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }

        m_bLoaded = true;
    } catch (const cv::Exception& e) {
        std::cerr << "YOLO LoadModel error: " << e.what() << std::endl;
        m_bLoaded = false;
    }

    return m_bLoaded;
}

void Yolo::Detect(const cv::Mat& image,
    std::vector<YoloResult>& results,
    float confThreshold,
    float nmsThreshold) {
    try {
        results.clear();

        char buf[128];
        sprintf_s(buf, "img type=%d, ch=%d, size=%dx%d\n",
        image.type(), image.channels(), image.cols, image.rows);
        OutputDebugStringA(buf);

        if (!m_bLoaded || image.empty())
            return;

        cv::Mat bgr;
        if (image.channels() == 4)      cv::cvtColor(image, bgr, cv::COLOR_BGRA2BGR);
        else if (image.channels() == 1) cv::cvtColor(image, bgr, cv::COLOR_GRAY2BGR);
        else if (image.channels() == 3) bgr = image;
        else return;

        if (bgr.depth() != CV_8U) {
            // 필요하면 스케일 조정해야 하지만, 보통 카메라는 8U라 convert만
            bgr.convertTo(bgr, CV_8U);
        }

        // 1. blob 생성
        cv::Mat blob;
        cv::dnn::blobFromImage(
            bgr, blob,
            1.0 / 255.0,
            cv::Size(m_nInputW, m_nInputH),
            cv::Scalar(),
            true,   // BGR->RGB
            false
        );

        m_net.setInput(blob);

        // 2. forward
        std::vector<cv::Mat> outputs;
        std::vector<cv::String> outNames = m_net.getUnconnectedOutLayersNames();
        m_net.forward(outputs, outNames);

        if (outputs.empty()) return;

        cv::Mat out = outputs[0];
        if (out.empty()) return;
        if (!out.isContinuous()) out = out.clone();

        std::vector<cv::Rect> boxes;
        std::vector<float> confidences;
        std::vector<int> classIds;

        ParseDetections(out, confThreshold, bgr.cols, bgr.rows, boxes, confidences, classIds);

        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);

        for (int idx : indices) {
            results.push_back({ boxes[idx], classIds[idx], confidences[idx] });
        }
    } catch (cv::Exception ex) {
        OutputDebugStringA(ex.what());
        return;
    }
}

void Yolo::DrawDetections(const cv::Mat& image,
    cv::Mat& output,
    const std::vector<YoloResult>& results) {
    if (image.empty())
        return;

    output = image.clone();

    for (const auto& r : results) {
        cv::rectangle(output, r.box, cv::Scalar(0, 255, 0), 2);

        char text[64];
        std::snprintf(text, sizeof(text), "ID:%d %.2f", r.classId, r.confidence);

        int baseLine = 0;
        cv::Size labelSize = cv::getTextSize(
            text,
            cv::FONT_HERSHEY_SIMPLEX,
            0.5, 1, &baseLine
        );

        int x = r.box.x;
        int y = std::max(r.box.y - labelSize.height, 0);

        cv::rectangle(
            output,
            cv::Rect(x, y, labelSize.width, labelSize.height + baseLine),
            cv::Scalar(0, 255, 0),
            cv::FILLED
        );

        cv::putText(
            output, text,
            cv::Point(x, y + labelSize.height),
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,
            cv::Scalar(0, 0, 0),
            1
        );
    }
}

void Yolo::ParseDetections(const cv::Mat& out,
    float confThreshold,
    int imgW, int imgH,
    std::vector<cv::Rect>& boxes,
    std::vector<float>& confidences,
    std::vector<int>& classIds) {
    boxes.clear();
    confidences.clear();
    classIds.clear();

    if (out.dims == 3) {
        int dim0 = out.size[0];
        int dim1 = out.size[1];
        int dim2 = out.size[2];

        if (dim1 == (4 + m_nNumClasses)) {
            // [1, 84, N] (channels-first)
            int numChannels = dim1;
            int numDet = dim2;

            float* data = (float*)out.data;

            float scaleX = static_cast<float>(imgW) / m_nInputW;
            float scaleY = static_cast<float>(imgH) / m_nInputH;

            for (int i = 0; i < numDet; ++i) {
                float x = data[0 * numDet + i];
                float y = data[1 * numDet + i];
                float w = data[2 * numDet + i];
                float h = data[3 * numDet + i];

                float maxScore = -1.0f;
                int   bestClass = -1;

                for (int c = 0; c < m_nNumClasses; ++c) {
                    float score = data[(4 + c) * numDet + i];
                    if (score > maxScore) {
                        maxScore = score;
                        bestClass = c;
                    }
                }

                if (maxScore < confThreshold)
                    continue;

                float x0 = (x - w / 2.0f) * scaleX;
                float y0 = (y - h / 2.0f) * scaleY;
                float x1 = (x + w / 2.0f) * scaleX;
                float y1 = (y + h / 2.0f) * scaleY;

                int left = std::max(0, (int)std::round(x0));
                int top = std::max(0, (int)std::round(y0));
                int right = std::min(imgW - 1, (int)std::round(x1));
                int bottom = std::min(imgH - 1, (int)std::round(y1));

                int boxW = right - left;
                int boxH = bottom - top;
                if (boxW <= 0 || boxH <= 0)
                    continue;

                boxes.emplace_back(left, top, boxW, boxH);
                confidences.emplace_back(maxScore);
                classIds.emplace_back(bestClass);
            }
        } else if (dim2 == (4 + m_nNumClasses)) {
            // [1, N, 84] (channels-last) 경우
            int numDet = dim1;
            int numChannels = dim2;

            float* data = (float*)out.data;

            float scaleX = static_cast<float>(imgW) / m_nInputW;
            float scaleY = static_cast<float>(imgH) / m_nInputH;

            for (int i = 0; i < numDet; ++i) {
                float x = data[i * numChannels + 0];
                float y = data[i * numChannels + 1];
                float w = data[i * numChannels + 2];
                float h = data[i * numChannels + 3];

                float maxScore = -1.0f;
                int   bestClass = -1;

                for (int c = 0; c < m_nNumClasses; ++c) {
                    float score = data[i * numChannels + 4 + c];
                    if (score > maxScore) {
                        maxScore = score;
                        bestClass = c;
                    }
                }

                if (maxScore < confThreshold)
                    continue;

                float x0 = (x - w / 2.0f) * scaleX;
                float y0 = (y - h / 2.0f) * scaleY;
                float x1 = (x + w / 2.0f) * scaleX;
                float y1 = (y + h / 2.0f) * scaleY;

                int left = std::max(0, (int)std::round(x0));
                int top = std::max(0, (int)std::round(y0));
                int right = std::min(imgW - 1, (int)std::round(x1));
                int bottom = std::min(imgH - 1, (int)std::round(y1));

                int boxW = right - left;
                int boxH = bottom - top;
                if (boxW <= 0 || boxH <= 0)
                    continue;

                boxes.emplace_back(left, top, boxW, boxH);
                confidences.emplace_back(maxScore);
                classIds.emplace_back(bestClass);
            }
        } else {
            std::cerr << "Unexpected YOLO output shape: ["
                << dim0 << "," << dim1 << "," << dim2 << "]" << std::endl;
        }
    } else {
        std::cerr << "Unsupported YOLO output dims: " << out.dims << std::endl;
    }
}