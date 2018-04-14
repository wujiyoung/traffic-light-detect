#include "opencv2/opencv.hpp"
#include <iostream>

// parameters for light area extraction
const int thresh_red_min = 145;
const int thresh_red_max = 256;
const int thresh_green_min = 90;
const int thresh_green_max = 110;

// image light compensation
const float gain_green_a = 0.25;
const float gain_red_a = 0.45;

// minimal box size for detected light
const int min_size = 10;
// maximal ratio between width and height of detected box
const float w_vs_h = 1.2;

// thresh value for circle and arrow
const float thresh_circle_min = 0.70;
const float thresh_circle_max = 0.95;
const float thresh_arrow_max = 0.7;
const float thresh_arrow_min = 0.2;
const float thresh_ud = 1.2;
const float thresh_lr = 1.2;

// history frame number to be used
const float history_number = 10;

// show debug result: including some intermediate windows and extra standard output
const bool showDebug = false;

enum LightType{ CIRCLE, FORWARD, LEFT, RIGHT };

struct light{
	cv::Point2f pos, sz;	// light position and box size
	LightType lt;			// light type
	int co = -1;			// light color: 0, red;  1, green
	float rat = 0;			// light cofidence
};

// split rgb image to YCrCb image
void splitImage(cv::Mat img, cv::Mat &imgRed, cv::Mat &imgGreen);
// find potential light box
std::vector<light> findLight(cv::Mat &srcImg, cv::Mat grayImg, int color);
// light box filter
std::vector<light> validLight(std::vector<light> lts);
// gain the light resut of current frame
int genResult(std::vector<light> &ltrs, std::vector<light> &ltgs);
// two methods of generate robust result with former frames
int multiPreidict(int curResult);
int multiPreidict2(int curResult, std::vector<light> ltgs);
// show detected light on original image
cv::Mat showResult(cv::Mat img, int result, std::vector<light> redlgs, std::vector<light> greenlgs);