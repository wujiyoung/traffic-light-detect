#include "trafficLight.h"

using namespace std;
using namespace cv;

extern std::deque<int> history_results;

int main()
{
	int isOnline = 1;	//0 image; 1 video; 2 camera
	bool save = false;

	Mat frame, frameRoi;
	Mat img;
	Mat imgGreen;
	Mat imgRed;

	// Parameters of brightness
	double a = 0.4; // gain, modify this only.
	double b = (1 - a) * 125; // bias

	VideoCapture capture;
	if(isOnline == 2){
		capture = VideoCapture(0);
		if (!capture.isOpened())
		{
			cout << "Start device failed!\n" << endl;
			system("pause");
			return -1;
		}
	}
	else if (isOnline == 1){
		capture = VideoCapture("sample.avi");
		if (!capture.isOpened())
		{
			cout << "fail to open video file!\n" << endl;
			system("pause");
			return -1;
		}
	}

	VideoWriter wr;

	char imgpath[50];

	int iii = 1;

	Mat srcImage;

	namedWindow("Origin", 0);

	while (1)
	{
		if(isOnline > 0){
			capture >> frame;
		}
		else {
			sprintf(imgpath, "TrafficLight8-19/%d.png", iii);
			frame = imread(imgpath);
			iii++;
		}

		if(!frame.data){
			cout << "fail to read image!" << endl;
			break;
		}

		frame.copyTo(srcImage);

		// Adjust the brightness
		// frame.convertTo(img, img.type(), a, b);
		// frame(Rect(0, 0, frame.cols, frame.rows / 2)).copyTo(frameRoi);
		// frameRoi.convertTo(img, img.type(), a, b);
		// frame.convertTo(img, img.type(), a, b);
		frame.copyTo(img);

		imgRed.create(img.rows, img.cols, CV_8UC1);
		imgGreen.create(img.rows, img.cols, CV_8UC1);

		splitImage(img, imgRed, imgGreen);

		vector<light> redlgs = findLight(frame, imgRed, 0);
		vector<light> greenlgs = findLight(frame, imgGreen, 1);

		redlgs = validLight(redlgs);
		greenlgs = validLight(greenlgs);

		int result = genResult(redlgs, greenlgs);

		//if (history_results.size() < history_number){
		//	history_results.push_back(result);
		//	continue;
		//}
		result = multiPreidict2(result, greenlgs);

		img = showResult(frame, result, redlgs, greenlgs);
		imshow("Origin", img);

		if (showDebug){
			namedWindow("Green", 0);
			namedWindow("Red", 0);
			imshow("Red", imgRed);
			imshow("Green", imgGreen);
		}

		if (save)
			wr << srcImage;

		// Handle with the keyboard input
		char c;
		if (showDebug)
			c = waitKey(0);
		else
			c = waitKey(1);
		if (c == 27)
			break;
		else if (c == 's'){
			save = !save;
			if (!wr.isOpened())
				wr = VideoWriter("result.avi", CV_CAP_PROP_FOURCC, 25, frame.size());
			if (save)
				cout << "start record!" << endl;
			else
				cout << "pause record!" << endl;
		}
	}

	if(isOnline > 0)
		capture.release();
	if (wr.isOpened())
		wr.release();
	destroyAllWindows();

	//system("pause");

	return 0;
}