#include <ctype.h>

#include <algorithm>
#include <iostream>
#include <opencv2/features2d.hpp>
#include <opencv2/highgui.hpp>

#include "opencv2/imgproc.hpp"
#include "opencv2/video/tracking.hpp"
#include "opencv2/videoio.hpp"

#define SHIFT_THRESHOLD 15
#define TOP_N_ARROWS 7

static void help() {
  // print a welcome message, and the OpenCV version
  std::cout << "\nThis is a demo of Lukas-Kanade optical flow lkdemo(),\n"
               "Using OpenCV version "
            << CV_VERSION << std::endl;
  std::cout
      << "\nIt uses camera by default, but you can provide a path to video as "
         "an argument.\n";
  std::cout << "\nHot keys: \n"
               "\tESC - quit the program\n"
               "\tr - auto-initialize tracking\n"
               "\tc - delete all the points\n"
               "\tn - switch the \"night\" mode on/off\n"
               "To add/remove a feature point click it\n"
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
  if (argc != 2) {
    std::cout << "One and only one option must be provided: path to video file"
              << std::endl;
    return 1;
  }
  cv::VideoCapture cap(argv[1]);
  cv::TermCriteria termcrit(cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS,
                            20, 0.01);
  cv::Size subPixWinSize(3, 3), winSize(5, 5);
  const int MAX_COUNT = 800;
  bool needToInit = false;
  bool nightMode = false;
  help();
  if (!cap.isOpened()) {
    std::cout << "Could not initialize capturing..." << std::endl;
    return 0;
  }
  cv::namedWindow("LK Demo", 1);
  cv::Mat gray, prevGray, image, frame, descriptors;
  std::vector<cv::Point2f> points[2];
  std::vector<cv::KeyPoint> keypoints;
  int counter = 0;
  std::vector<Arrow> arrows;
  for (;;) {
    if (counter++ % 20 == 0) {
      points[0].clear();
      points[1].clear();
    } else {
      cap >> frame;
      if (frame.empty()) break;
      frame.copyTo(image);
      cvtColor(image, gray, cv::COLOR_BGR2GRAY);

      goodFeaturesToTrack(gray, points[1], MAX_COUNT, 0.001, 1, cv::Mat(), 3, 3,
                          true, 0.04);
      cornerSubPix(gray, points[1], subPixWinSize, cv::Size(-1, -1), termcrit);
      std::vector<uchar> status;
      std::vector<float> err;
      if (!points[0].empty()) {
        arrows.clear();
        status.clear();
        err.clear();
        if (prevGray.empty()) gray.copyTo(prevGray);
        calcOpticalFlowPyrLK(prevGray, gray, points[0], points[1], status, err,
                             winSize, 3, termcrit, 0, 0.001);
      }
      // Calculate shift and for relevant shifts create arrows
      // Shift is considered relevant if its magnitude is below SHIFT_THRESHOLD
      for (int i = 0; i < status.size(); i++) {
        if (status[i] != 0) {
          int line_thickness;
          line_thickness = 1;
          cv::Point2i p, q;
          p.x = (int)points[0][i].x;
          p.y = (int)points[0][i].y;
          q.x = (int)points[1][i].x;
          q.y = (int)points[1][i].y;
          double angle = atan2((double)p.y - q.y, (double)p.x - q.x);
          double hypotenuse = sqrt(pow(p.y - q.y, 2) + pow(p.x - q.x, 2));
          if (hypotenuse < SHIFT_THRESHOLD) {
            arrows.push_back(Arrow{p, q, angle, hypotenuse});
          }
        }
      }
      std::sort(arrows.begin(), arrows.end(), std::greater<Arrow>());
      arrows.resize(5);
      Arrow averageAr{cv::Point2i(640, 360), cv::Point2i(640, 360), .0, 50.};
      for (Arrow ar : arrows) {
        averageAr.angle += ar.angle;
      }
      averageAr.angle /= arrows.size();
      averageAr.end.x = (int)(averageAr.start.x -
                              3 * averageAr.length * cos(averageAr.angle));
      averageAr.end.y = (int)(averageAr.start.y -
                              3 * averageAr.length * sin(averageAr.angle));
      cv::arrowedLine(image, averageAr.start, averageAr.end,
                      cv::Scalar((0), (255), (0), 0), 3);
      needToInit = true;
      imshow("LK Demo", image);
      char c = (char)cv::waitKey(10);
      if (c == 27) break;
      std::swap(points[1], points[0]);
      cv::swap(prevGray, gray);
    }
  }
  return 0;
}