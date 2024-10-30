#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

bool isDrawing = false;
bool initialChannelBar = false;
Point startPoint, endPoint;
Mat tempFrame, roiFrame, frame, Bframe, Gframe, Rframe, selectChannelFrame;
int blueNonZero = 0, greenNonZero = 0, redNonZero = 0;
int channelSelector = 0; //選擇 B:1、G:2、R:3 哪一張圖層
int bins = 1; //切割的分群數

vector<vector<uchar>> DataGrouping() {
    vector<uchar> vecData;
    Mat bgr[3];
    split(selectChannelFrame, bgr);
    Mat Data = bgr[channelSelector];
    vecData.assign(Data.begin<uchar>(), Data.end<uchar>());

    int rangeSize = 256 / (bins == 0 ? 1 : bins);
    vector<vector<uchar>> groupData(bins);

    int index = 0;
    for (uchar val : vecData) {
        int groupIndex = val / rangeSize;
        if (groupIndex >= bins) {
            groupIndex = bins - 1; //確保不會超過最後一組
        }
        groupData[groupIndex].push_back(val);
    }
    return groupData;
}

void updateHistogram(int, void*) {
    bins = bins + 1;
    int histHeight = 300, histWidth = 400;
    Mat histImage(histHeight, histWidth, CV_8SC3, Scalar(255, 255, 255));
    
    // 計算每組數據的最大值
    vector<vector<uchar>> groupData = DataGrouping();
    int maxCount = 0;
    for (auto group : groupData) {
        maxCount = max(maxCount, (int)group.size());
    }

    //設定每組的顏色
    vector<Vec3b> colors = { Vec3b(255, 0, 0), Vec3b(0, 255, 0), Vec3b(0, 0, 255), Vec3b(255, 255, 0) };
    
    //繪製直方圖
    int rangeSize = 256 / (bins == 0 ? 1 : bins);
    for (int i = 0; i < bins; i++) {
        int binHeight = (groupData[i].size() * histHeight) / (maxCount > 0 ? maxCount : 1); //計算每組高度
        rectangle(
            histImage,
            Point(i * (histWidth / bins) + 10, histHeight),
            Point((i + 1) * (histWidth / bins) - 10, histHeight - binHeight),
            colors[i % colors.size()],
            -1
        );
    }
    // 添加x軸標籤
    for (int i = 0; i < bins; i++) {
        int x = (i + 0.5) * (histWidth / bins) - 10;
        int binHeight = (groupData[i].size() * histHeight) / (maxCount > 0 ? maxCount : 1);
        putText(
            histImage,
            to_string(groupData[i].size()),
            Point(x - 15, histHeight - binHeight),
            FONT_HERSHEY_SIMPLEX,
            0.5,
            Scalar(0, 0, 0),
            1
        );

        putText(
            histImage,
            to_string(i * rangeSize) + "_" + to_string((i + 1) * rangeSize - 1),
            Point(x - 15, histHeight - 10),
            FONT_HERSHEY_SIMPLEX,
            0.5,
            Scalar(0, 0, 0),
            1
        );  
    }
    imshow("Histogram", histImage);
}

void onHistogramChange() {
    if (!initialChannelBar) {
        namedWindow("Histogram", WINDOW_NORMAL);
        createTrackbar("Bins", "Histogram", &bins, 16, updateHistogram);
        updateHistogram(0, 0);
    }
    // 設定目前要統計的圖層
    
    setTrackbarPos("Bins", "Histogram", 0);
}

void updateChannel(int, void*) {
    //更新顯示圖層
    Mat bgr[3];
    split(roiFrame, bgr);
    Mat zeroChannel = Mat::zeros(roiFrame.size(), CV_8UC1);
    vector<Mat> channels(3, zeroChannel);
    channels[channelSelector] = bgr[channelSelector];
    merge(channels, selectChannelFrame);
    imshow("select channel frame", selectChannelFrame);
    // 更新histogram視窗
    onHistogramChange();
}

void onChannelChange() {
    // 顯示選擇頻道視窗
    if (!initialChannelBar) {
        namedWindow("select channel frame", WINDOW_NORMAL);
        createTrackbar("Channel", "select channel frame", &channelSelector, 2, updateChannel);
        updateChannel(0, 0);
        initialChannelBar = true;
    }
}

void Split_RGB_Layer(void) {
    Mat bgr[3];
    Mat zeroChannel = Mat::zeros(roiFrame.size(), CV_8UC1);
    split(roiFrame, bgr);

    // Count Non Zero
    blueNonZero = countNonZero(bgr[0]);
    greenNonZero = countNonZero(bgr[1]);
    redNonZero = countNonZero(bgr[2]);

    // Show channel
    vector<Mat> channel;
    // Blue channel
    channel = { bgr[0], zeroChannel, zeroChannel };
    merge(channel, Bframe);

    //Green channel
    channel = { zeroChannel, bgr[1], zeroChannel };
    merge(channel, Gframe);

    //Red channel
    channel = { zeroChannel, zeroChannel, bgr[2] };
    merge(channel, Rframe);


    imshow("Blue Channel", Bframe);
    imshow("Green Channel", Gframe);
    imshow("Red Channel", Rframe);

}

void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN) {
        isDrawing = true;
        startPoint = Point(x, y);
    }
    else if (event == EVENT_MOUSEMOVE && isDrawing) {
        tempFrame = frame.clone();
        rectangle(tempFrame, startPoint, Point(x, y), Scalar(0, 255, 0), 2);
        imshow("Video", tempFrame);
    }
    else if (event == EVENT_LBUTTONUP && isDrawing) {
        isDrawing = false;
        endPoint = Point(x, y);
        rectangle(frame, startPoint, endPoint, Scalar(0, 255, 0), 2);
        imshow("Video", frame);

        Rect roiRect(
            min(startPoint.x, endPoint.x),
            min(startPoint.y, endPoint.y),
            abs(startPoint.x - endPoint.x),
            abs(startPoint.y - endPoint.y));
        roiFrame = frame(roiRect).clone();
        //imshow("ROI", roiFrame);
        Split_RGB_Layer();
        onChannelChange();
    }
}

int main()
{
    string video_path = "D:\\C_project\\MidtermAssignment\\SkyFire.avi";
    VideoCapture cap(video_path);

    if (!cap.isOpened()) {
        cout << "Cannot open video" << endl;
        return -1;
    }
    namedWindow("Video", WINDOW_AUTOSIZE);
    setMouseCallback("Video", mouseCallback);
    while (true) {
        cap >> frame;
        if (!cap.read(frame)) {
            cap.set(CAP_PROP_POS_FRAMES, 0);
            continue;
        }
        putText(frame, "Blue Non-Zero: " + to_string(blueNonZero), Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 0, 0), 1);
        putText(frame, "Green Non-Zero: " + to_string(greenNonZero), Point(10, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 1);
        putText(frame, "Red Non-Zero: " + to_string(redNonZero), Point(10, 70), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 1);


        imshow("Video", frame);

        if ((char)waitKey(50) == 'q') break;

    }

    cap.release();
    destroyAllWindows();
    return 0;
}

