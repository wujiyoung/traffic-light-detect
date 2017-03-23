#include "opencv2/opencv.hpp"  
#include "opencv2/imgproc.hpp"
// #include <windows.h>
#include <iostream>

using namespace std;
using namespace cv;

int min_size = 8;
float ratio = 1.2;

bool isOnline = false;

enum LightType{CIRCLE, FORWARD, LEFT, RIGHT};

struct light{
	Point2f pos, sz;
	LightType lt;
	int co;
	float rat;
};

// Function header
vector<light> findLight(Mat&, Mat, int);
int validLight(vector<light> &, vector<light> &);

int main()
{
	int redCount = 0;
	int greenCount = 0;

	Mat frame;
	Mat img;
	Mat imgYCrCb;
	Mat imgGreen;
	Mat imgRed;

	// Parameters of brightness
	double a = 0.3; // gain, modify this only.
	double b = (1 - a) * 125; // bias

	VideoCapture capture;
	if(isOnline){
		capture = VideoCapture(0);
		if (!capture.isOpened())
		{
			cout << "Start device failed!\n" << endl;
			return -1;
		}

	}

	char imgpath[50];
	int iii = 1;

	namedWindow("Origin", 0);

	// ÖðÖ¡´¦Àí
	while (1)
	{
		if(isOnline){
			capture >> frame;
		}
		else {
			// sprintf(imgpath, "data/day/daySequence1--%.5d.png", iii);
			// sprintf(imgpath, "data/night/nightSequence1--%.5d.png", iii);
			sprintf(imgpath, "data/TrafficLight8-19/%d.png", iii);
			frame = imread(imgpath);
			iii++;
		}

		if(!frame.data){
			cout << "fail to read image!" << endl;
			return 0;
		}
		// Adjust the brightness
		frame.convertTo(img, img.type(), a, b);

		// Convert to YCrCb color space
		cvtColor(img, imgYCrCb, CV_BGR2YCrCb);

		imgRed.create(imgYCrCb.rows, imgYCrCb.cols, CV_8UC1);
		imgGreen.create(imgYCrCb.rows, imgYCrCb.cols, CV_8UC1);

		// Split three components of YCrCb
		vector<Mat> planes;
		split(imgYCrCb, planes);
		// Traversing to split the color of RED and GREEN according to the Cr component
		MatIterator_<uchar> it_Cr = planes[1].begin<uchar>(),
			it_Cr_end = planes[1].end<uchar>();
		MatIterator_<uchar> it_Red = imgRed.begin<uchar>();
		MatIterator_<uchar> it_Green = imgGreen.begin<uchar>();

		for (; it_Cr != it_Cr_end; ++it_Cr, ++it_Red, ++it_Green)
		{
			// RED, 145<Cr<470 
			if (*it_Cr > 145 && *it_Cr < 470)
				*it_Red = 255;
			else
				*it_Red = 0;

			// GREEN£¬95<Cr<110
			if (*it_Cr > 95 && *it_Cr < 110)
				*it_Green = 255;
			else
				*it_Green = 0;
		}

		vector<light> redlgs = findLight(frame, imgRed, 0);
		vector<light> greenlgs = findLight(frame, imgGreen, 1);

		imshow("Origin", frame);

		namedWindow("Green", 0);
		namedWindow("Red", 0);
		imshow("Red", imgRed);
		imshow("Green", imgGreen);

		// Handle with the keyboard input
		char c = waitKey(50);
		if (c == 27)
			break;
	}

	if(isOnline)
		capture.release();

	return 0;
}

/** @function findLight */
vector<light> findLight(Mat &srcImg, Mat grayImg, int color)
{
  Mat threshold_output;
  vector<vector<Point> > contours;
  vector<Vec4i> hierarchy;

  vector<light> lgs;

  /// Detect edges using Threshold
  threshold( grayImg, threshold_output, 100, 255, THRESH_BINARY );
  /// Find contours
  findContours( threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

  /// Approximate contours to polygons + get bounding rects and circles
  vector<vector<Point> > contours_poly( contours.size() );
  vector<Rect> boundRect;
  vector<Point2f>center;
  vector<float>radius;
  vector<LightType> lts;
  vector<float> rats;

  for( int i = 0; i < contours.size(); i++ ){ 
		approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
		Rect rec = boundingRect( Mat(contours_poly[i]) );
		if(rec.width > min_size && rec.height > min_size && (float)rec.width / (float)rec.height < ratio && (float)rec.width / (float)rec.height > 1 / ratio){
	 		int recw = rec.width; 
	 		int rech = rec.height;
	 		float count1 = countNonZero(grayImg(rec));
	 		float countu = countNonZero(grayImg(rec)(Rect(0, 0, recw, rech / 2)));
	 		float countd = countNonZero(grayImg(rec)(Rect(0, rech / 2, recw, rech / 2)));
	 		float countl = countNonZero(grayImg(rec)(Rect(0, 0, recw / 2, rech)));
	 		float countr = countNonZero(grayImg(rec)(Rect(recw / 2, 0, recw / 2, rech)));
	 		float ud = countu / countd;
	 		float du = countd / countu;
	 		float lr = countl / countr;
	 		float rl = countr / countl;
	 		float sz = recw * rech;
	 		bool isLight = false;
	 		if(count1 / sz > 0.75 && count1 / sz < 0.9){
	 			lts.push_back(CIRCLE);
	 			isLight = true;
	 			rats.push_back(count1 / sz);
	 		}
	 		else if(count1 / sz < 0.6){
	 			isLight = true;
	 			if(ud > 1.3){
	 				lts.push_back(FORWARD);
	 				rats.push_back(ud);
	 			}
	 			else if(lr > 1.5 && (ud < 1.2 || du < 1.2)){
	 				lts.push_back(LEFT);
	 				rats.push_back(lr);
	 			}
	 			else if(rl > 1.5 && (ud < 1.2 || du < 1.2)){
	 				lts.push_back(RIGHT);
	 				rats.push_back(rl);
	 			}
	 			else {
	 				isLight = false;
	 			}
	 		}

	 		if(isLight){
	 			boundRect.push_back(rec);
	   		Point2f cen;
	   		float r;
	   		minEnclosingCircle( (Mat)contours_poly[i], cen, r);
	   		center.push_back(cen);
	   		radius.push_back(r);
	 		}
	 }
	}

  if(color == 0){
  	cout << "red: " << boundRect.size() << " ";
  }
  else {
  	cout << "green: " << boundRect.size() << endl;	
  }

  /// Draw polygonal contour + bonding rects + circles
  for( int i = 0; i< boundRect.size(); i++ )
	{
		light lg;
		lg.pos = Point2f(boundRect[i].x, boundRect[i].y);
		lg.sz = Point2f(boundRect[i].width, boundRect[i].height);
		lg.lt = lts[i];
		lg.co = color;
		lg.rat = rats[i];
		lgs.push_back(lg);

		if(color == 0) {
			if(lts[i] == 0){
     		rectangle( srcImg, boundRect[i].tl(), boundRect[i].br(), Scalar(255, 255, 255), 2, 8, 0 );
			}
			else {
				circle(srcImg, center[i], radius[i] * lts[i], Scalar(255, 255, 255));
			}
		}
		else {
			if(lts[i] == 0){
   			rectangle( srcImg, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 0, 255), 2, 8, 0 );
			}
			else {
	 			circle(srcImg, center[i], radius[i] * lts[i], Scalar(0, 0, 255));
			}
		}
	}

  return lgs;
}

int validLight(vector<light> &ltrs, vector<light> &ltgs)
{
	int result = 0;
	return result;
}