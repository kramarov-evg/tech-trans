#include <ctype.h>

#include <algorithm>
#include <iostream>
#include <opencv2/features2d.hpp>
#include <opencv2/highgui.hpp>

#include "opencv2/imgproc.hpp"
#include "opencv2/video/tracking.hpp"
#include "opencv2/videoio.hpp"

static void help() {
  // print a welcome message, and the OpenCV version
  std::cout << "Single object motion direction estimation with OpenCV"
            << std::endl;
  std::cout << "Args:" << std::endl
            << "\tpath_to_video: Path to video file to work on" << std::endl;
  std::cout << "Hot keys:" << std::endl
            << "\tESC - quit the program\n"
            << std::endl;
}

struct Arrow {
  cv::Point2i start;
  cv::Point2i end;
  double angle;
  double length;
};

bool operator<(const Arrow& a1, const Arrow& a2) {
  return a1.length < a2.length;
}
bool operator>(const Arrow& a1, const Arrow& a2) {
  return a1.length > a2.length;
}

int main(int argc, char** argv) {
  help();
  if (argc != 2) {
    std::cout << "One and only one option must be provided: path to video file"
              << std::endl;
    return 1;
  }
  cv::VideoCapture cap(argv[1]);
  if (!cap.isOpened()) {
    std::cout << "Could not initialize capturing from" << argv[1] << std::endl;
    return 0;
  }
  int frameWidth = cap.get(cv::CAP_PROP_FRAME_WIDTH);
  int frameHeight = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
  int averageArrowStartX = frameWidth / 2;
  int averageArrowStartY = frameHeight / 2;
  double averageArrowLen = std::min(frameWidth, frameHeight) / 4.;
  cv::TermCriteria termcrit(cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS,
                            20, 0.01);
  cv::Size subPixWinSize(10, 10), winSize(31, 31);
  /**
   * MAX_N_FEATURES - Max number of features that will be detected in an image
   * SHIFT_ACCEPTANCE_THRESHOLD - Max shift, that wil be treated as a non-error
   * TOP_N_ARROWS - Number of longest shift vectors, taken into account, when
   *                calculating resulting direction
   * FRAMES_BETWEEN_REINIT - Number of frames between features reinitialization
   */
  const int MAX_N_FEATURES = 800, SHIFT_ACCEPTANCE_THRESHOLD = 15,
            TOP_N_ARROWS = 5, FRAMES_BETWEEN_REINIT = 20;
  cv::namedWindow("Train direction detection", cv::WINDOW_AUTOSIZE);
  cv::Mat gray, prevGray, image;

  // Array of vectors that will store previous and current frames' keypoints
  std::vector<cv::Point2f> points[2];
  int frameCounter = 0;
  std::vector<Arrow> arrows;
  for (;;) {
    // It's needed to reinitialize features once in a while
    if (frameCounter++ % FRAMES_BETWEEN_REINIT == 0) {
      points[0].clear();
      points[1].clear();
    } else {
      cap >> image;
      if (image.empty()) {
        break;
      }
      cvtColor(image, gray, cv::COLOR_BGR2GRAY);

      goodFeaturesToTrack(gray, points[1], MAX_N_FEATURES, 0.001, 1, cv::Mat(),
                          3, 3, true, 0.04);
      cornerSubPix(gray, points[1], subPixWinSize, cv::Size(-1, -1), termcrit);

      // Vector indicating, whether feature from frame1 was found in frame2
      std::vector<uchar> status;
      std::vector<float> err;
      if (!points[0].empty()) {
        arrows.clear();
        status.clear();
        err.clear();
        if (prevGray.empty()) {
          gray.copyTo(prevGray);
        }
        calcOpticalFlowPyrLK(prevGray, gray, points[0], points[1], status, err,
                             winSize, 3, termcrit, 0, 0.001);
      }
      // Calculate shift and for relevant shifts create arrows
      // Shift is considered relevant if its magnitude is below
      // SHIFT_ACCEPTANCE_THRESHOLD
      for (int i = 0; i < status.size(); i++) {
        if (status[i] != 0) {
          int line_thickness;
          line_thickness = 1;
          cv::Point2i start, end;
          start.x = (int)points[0][i].x;
          start.y = (int)points[0][i].y;
          end.x = (int)points[1][i].x;
          end.y = (int)points[1][i].y;
          double angle =
              atan2((double)start.y - end.y, (double)start.x - end.x);
          double magnitude =
              sqrt(pow(start.y - end.y, 2) + pow(start.x - end.x, 2));
          if (magnitude < SHIFT_ACCEPTANCE_THRESHOLD) {
            arrows.push_back(Arrow{start, end, angle, magnitude});
          }
        }
      }
      std::sort(arrows.begin(), arrows.end(), std::greater<Arrow>());
      arrows.resize(TOP_N_ARROWS);
      Arrow averageAr{cv::Point2i(averageArrowStartX, averageArrowStartY),
                      cv::Point2i(0, 0), .0, averageArrowLen};
      for (Arrow ar : arrows) {
        averageAr.angle += ar.angle;
      }
      averageAr.angle /= arrows.size();
      averageAr.end.x =
          (int)(averageAr.start.x - averageAr.length * cos(averageAr.angle));
      averageAr.end.y =
          (int)(averageAr.start.y - averageAr.length * sin(averageAr.angle));
      cv::arrowedLine(image, averageAr.start, averageAr.end,
                      cv::Scalar((0), (255), (0), 0), 3);
      imshow("Train direction detection", image);
      char c = (char)cv::waitKey(10);
      if (c == 27) {
        break;
      }
      std::swap(points[1], points[0]);
      cv::swap(prevGray, gray);
    }
  }
  return 0;
}