#include "trafficLight.h"
#include "opencv2/imgproc/imgproc.hpp"

using namespace std;
using namespace cv;

int lastResult = 0;
deque<int> history_results;

int countl = history_number, countf = history_number, countr = history_number;
int lastl = 0, lastf = 0, lastr = 0;

// Split three components of YCrCb
void splitImage(Mat img, Mat &imgRed, Mat &imgGreen){

	Mat imgYCrCb;

	// Convert to YCrCb color space
	cvtColor(img, imgYCrCb, CV_BGR2YCrCb);

	vector<Mat> planes;
	split(imgYCrCb, planes);
	// Traversing to split the color of RED and GREEN according to the Cr component
	MatIterator_<uchar> it_Cr = planes[1].begin<uchar>(),
		it_Cr_end = planes[1].end<uchar>();
	MatIterator_<uchar> it_Red = imgRed.begin<uchar>();
	MatIterator_<uchar> it_Green = imgGreen.begin<uchar>();

	// Parameters of brightness
	float gain_red_b = (1 - gain_red_a) * 125;
	float gain_green_b = (1 - gain_green_a) * 125;

	for (; it_Cr != it_Cr_end; ++it_Cr, ++it_Red, ++it_Green)
	{
		float cr = float(*it_Cr) * gain_red_a + gain_red_b;
		// RED, 145<Cr<470 
		if (cr > thresh_red_min && cr < thresh_red_max)
			*it_Red = 255;
		else
			*it_Red = 0;

		float cg = float(*it_Cr) * gain_green_a + gain_green_b;
		// GREEN£¬95<Cr<110
		if (cg > thresh_green_min && cg < thresh_green_max)
			*it_Green = 255;
		else
			*it_Green = 0;
	}
}

/** @function findLight */
vector<light> findLight(Mat &srcImg, Mat grayImg, int color)
{
	Mat threshold_output;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	vector<light> lgs;

	/// Detect edges using Threshold
	threshold(grayImg, threshold_output, 100, 255, THRESH_BINARY);
	/// Find contours
	findContours(threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	/// Approximate contours to polygons + get bounding rects and circles
	vector<vector<Point> > contours_poly(contours.size());
	vector<Rect> boundRect;
	vector<Point2f>center;
	vector<float>radius;
	vector<LightType> lts;
	vector<float> rats;

	for (int i = 0; i < contours.size(); i++){
		approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
		Rect rec = boundingRect(Mat(contours_poly[i]));
		if (rec.width > min_size && rec.height > min_size && (float)rec.width / (float)rec.height < w_vs_h && (float)rec.width / (float)rec.height > 1 / w_vs_h){
			Mat tmpImg = srcImg;
			//rectangle(tmpImg, rec.tl(), rec.br(), Scalar(255, 0, 0), 5, 8, 0);
			//imshow("temp", tmpImg);
			//waitKey(1);
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
			float area = count1 / sz;
			bool isLight = false;
			if (area > thresh_circle_min && area < thresh_circle_max){
				isLight = true; 
				if (recw < 15 && rech < 15){
					if (lr > thresh_lr){
						lts.push_back(LEFT);
						rats.push_back(lr);
					}
					else if (rl > thresh_lr){
						lts.push_back(RIGHT);
						rats.push_back(rl);
					}
					else if (ud > thresh_ud){
						lts.push_back(FORWARD);
						rats.push_back(ud);
					}
					else {
						lts.push_back(CIRCLE);
						rats.push_back(count1 / sz);
					}
				}
				else {
					lts.push_back(CIRCLE);
					rats.push_back(count1 / sz);
				}
			}
			else if (area > thresh_arrow_min && area < thresh_arrow_max){
				isLight = true;
				if (ud > thresh_ud){
					if (lr > ud){
						lts.push_back(LEFT);
						rats.push_back(lr);
					}
					else if (rl > ud){
						lts.push_back(RIGHT);
						rats.push_back(rl);
					}
					else {
						lts.push_back(FORWARD);
						rats.push_back(ud);
					}
				}
				else if (lr > thresh_lr && du < thresh_ud){
					lts.push_back(LEFT);
					rats.push_back(lr);
				}
				else if (rl > thresh_lr && du < thresh_ud){
					lts.push_back(RIGHT);
					rats.push_back(rl);
				}
				else {
					isLight = false;
				}
			}

			if (isLight){
				boundRect.push_back(rec);
				Point2f cen;
				float r;
				minEnclosingCircle((Mat)contours_poly[i], cen, r);
				center.push_back(cen);
				radius.push_back(r);

				if (showDebug){
					string msg = color == 0 ? "red: " : "green: ";
					msg += *(lts.end() - 1) == LEFT ? "left " : *(lts.end() - 1) == FORWARD ? "forward " : *(lts.end() - 1) == CIRCLE ? "circle " : "right ";
					cout << msg << rec << ", area " << area;
					cout << ", (ud, lr, rl) (" << ud << "," << lr << "," << rl << ")" << endl;
				}
			}
		}
	}

	// Draw polygonal contour + bonding rects + circles
	for (int i = 0; i< boundRect.size(); i++)
	{
		light lg;
		lg.pos = Point2f(boundRect[i].x, boundRect[i].y);
		lg.sz = Point2f(boundRect[i].width, boundRect[i].height);
		lg.lt = lts[i];
		lg.co = color;
		lg.rat = rats[i];
		lgs.push_back(lg);

		if (showDebug && color == 0) {
			if (lts[i] == CIRCLE){
				rectangle(srcImg, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 0, 0), 2, 8, 0);
			}
			else {
				circle(srcImg, center[i], radius[i] * lts[i], Scalar(0, 0, 0));
			}
		}
		else if (showDebug && color == 1) {
			if (lts[i] == CIRCLE){
				rectangle(srcImg, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 0, 255), 2, 8, 0);
			}
			else {
				circle(srcImg, center[i], radius[i] * lts[i], Scalar(0, 0, 255));
			}
		}
	}

	return lgs;
}

vector<light> validLight(vector<light> lts)
{
	vector<light> ltsres;
	if (lts.size() == 0)
		return ltsres;
	int result = 0;
	bool left = false, forward = false, right = false;
	light ltl, ltf, ltr;
	float areal = 0, areaf = 0, arear = 0;
	for (vector<light>::iterator it = lts.begin(); it != lts.end();){
		switch ((*it).lt)
		{
		case LEFT:
			if (!left || (left && (*it).rat > ltl.rat)){
				ltl = (*it);
				areal = ltl.sz.x * ltl.sz.y;
			}
			left = true;
			it = lts.erase(it);
			break;
		case RIGHT:
			if (!right || (right && (*it).rat > ltr.rat)){
				ltr = (*it);
				arear = ltr.sz.x * ltr.sz.y;
			}
			right = true;
			it = lts.erase(it);
			break;
		case FORWARD:
			if (!forward || (forward && (*it).rat > ltf.rat)){
				ltf = (*it);
				areaf = ltf.sz.x * ltf.sz.y;
			}
			forward = true;
			it = lts.erase(it);
			break;
		default:
			it++;
			break;
		}
	}

	light lttemp;
	if (forward && ltf.rat > lttemp.rat)
		lttemp = ltf;
	if (right && ltr.rat > lttemp.rat){
		lttemp = ltr;
	}
	if (left && ltl.rat > lttemp.rat)
		lttemp = ltl;

	//if (lttemp.co > -1){
	//	ltsres.push_back(lttemp);
	//}

	float dsz = 4, dy = 3 * lttemp.sz.y;
	if (forward && abs(lttemp.sz.x - ltf.sz.x) <= dsz && abs(lttemp.sz.y - ltf.sz.y) <= dsz && abs(lttemp.pos.y - ltf.pos.y) < dy){
		//ltsres.push_back(ltf);
		;
	}
	else {
		forward = false;
	}
	if (right && abs(lttemp.sz.x - ltr.sz.x) <= dsz && abs(lttemp.sz.y - ltr.sz.y) <= dsz && abs(lttemp.pos.y - ltr.pos.y) < dy){
		ltsres.push_back(ltr);
	}
	if (left && abs(lttemp.sz.x - ltl.sz.x) <= dsz && abs(lttemp.sz.y - ltl.sz.y) <= dsz && abs(lttemp.pos.y - ltl.pos.y) < dy){
		ltsres.push_back(ltl);
	}

	light lttemp1;
	if (lttemp.co > -1){
		float dis = 1000 * 1000;
		for (vector<light>::iterator it = lts.begin(); it != lts.end(); it++){
			if (abs((*it).sz.x - lttemp.sz.x < dsz) && abs((*it).sz.y - lttemp.sz.y) < dsz
				&& abs((*it).pos.y - lttemp.pos.y) < dy && abs((*it).pos.y - lttemp.pos.y) * abs((*it).pos.x - lttemp.pos.x) < dis){
				lttemp1 = *it;
				dis = abs((*it).pos.y - lttemp.pos.y) * abs((*it).pos.x - lttemp.pos.x);
			}
		}
	}
	else {
		for (vector<light>::iterator it = lts.begin(); it != lts.end(); it++){
			if (lttemp.co == -1 || (lttemp.co > -1 && (*it).rat > lttemp1.rat)){
				lttemp1 = *it;
			}
		}
	}
	if (lttemp1.co > -1){
		if (!forward)
			ltsres.push_back(lttemp1);
		else {
			if (ltf.rat > lttemp1.rat)
				ltsres.push_back(ltf);
			else
				ltsres.push_back(lttemp1);
		}
	}
	else if (forward)
		ltsres.push_back(ltf);

	return ltsres;
}

int genResult(vector<light> &ltrs, vector<light> &ltgs){

	for (vector<light>::iterator itr = ltrs.begin(); itr != ltrs.end();){
		for (vector<light>::iterator itg = ltgs.begin(); itg != ltgs.end();){
			if ((*itr).lt == (*itg).lt){
				if ((*itr).rat > (*itg).rat){
					itg = ltgs.erase(itg);
					itg--;
				}
				else{
					itr = ltrs.erase(itr);
					itr--;
				}
			}
			itg++;
		}
		itr++;
	}

	int l = 0, f = 0, r = 0;
	if (ltrs.size() == 1){
		ltrs[0].lt == LEFT ? l = 1 : ltrs[0].lt == RIGHT ? r = 1 : ltrs[0].lt == FORWARD ? f = 1 : (l = 1, f = 1);
		for (vector<light>::iterator it = ltgs.begin(); it != ltgs.end(); it++){
			if ((*it).lt == LEFT && l == 1){
				l = 0;
				break;
			}
		}
	}
	else if (ltrs.size() == 2){
		ltrs[0].lt == LEFT ? l = 1 : ltrs[0].lt == RIGHT ? r = 1 : ltrs[0].lt == FORWARD ? f = 1 : (l = 1, f = 1);
		ltrs[1].lt == LEFT ? l = 1 : ltrs[1].lt == RIGHT ? r = 1 : ltrs[1].lt == FORWARD ? f = 1 : (l = 1, f = 1);
		if (ltgs.size() > 0)
			ltgs[0].lt == LEFT ? l = 0 : ltgs[0].lt == RIGHT ? r = 0 : f = 0;
	}
	else if (ltrs.size() == 3){
		l = 1;
		f = 1;
		r = 1;
	}

	if (showDebug){
		cout << "red: ";
		for (vector<light>::iterator itr = ltrs.begin(); itr != ltrs.end(); itr++){
			string msg = (*itr).lt == LEFT ? "left" : (*itr).lt == FORWARD ? "forward" : (*itr).lt == CIRCLE ? "circle" : "right";
			cout << msg << " ";
		}
		cout << endl << "green: ";
		for (vector<light>::iterator itg = ltgs.begin(); itg != ltgs.end(); itg++){
			string msg = (*itg).lt == LEFT ? "left" : (*itg).lt == FORWARD ? "forward" : (*itg).lt == CIRCLE ? "circle" : "right";
			cout << msg << " ";
		}
		cout << endl;
	}

	return l * 4 + f * 2 + r;

}

int multiPreidict(int curResult){
	history_results.pop_front();
	history_results.push_back(curResult);
	if (curResult == lastResult)
		return curResult;
	else{
		map<int, int> results;
		int res = 0;
		int count = 1;
		int sameCount = 0;
		for (int i = history_results.size() - 1; i >= 0; i--){
			if (curResult == history_results[i])
				sameCount++;
			if (sameCount == 4){
				res = curResult;
				lastResult = res;
				return res;
			}
			map<int, int>::iterator ind = results.find(history_results[i]);
			if (ind != results.end()){
				ind->second++;
				if (ind->second > count){
					res = ind->first;
					count++;
				}
			}
			else
				results.insert(pair<int, int >(history_results[i], 1));
		}

		res =  count > 5 ? res : lastResult;
		lastResult = res;
		return res;
	}
}

int multiPreidict2(int curResult, vector<light> ltgs){

	int l, f, r;
	l = curResult / 4;
	f = (curResult - l * 4) / 2;
	r = curResult - l * 4 - f * 2;

	if (l == 1)
		countl = history_number;
	else if(lastl == 1){
		countl--;
		l = 1;
		if (countl == 0){
			l = 0;
		}
	}

	if (f == 1)
		countf = history_number;
	else if (lastf == 1){
		countf--;
		f = 1;
		if (countf == 0){
			f = 0;
		}
	}

	if (r == 1)
		countr = history_number;
	else if (lastr == 1){
		countr--;
		r = 1;
		if (countr == 0){
			r = 0;
		}
	}

	if (showDebug){
		cout << "countl: " << countl << "   countf: " << countf << "   countr: " << countr << endl;
	}

	for (vector<light>::iterator itg = ltgs.begin(); itg != ltgs.end(); itg++){
		(*itg).lt == LEFT ? l = 0 : (*itg).lt == RIGHT ? r = 0 : f = 0;
	}

	lastl = l;
	lastf = f;
	lastr = r;

	int res = l * 4 + f * 2 + r;
	return res;
}

Mat showResult(Mat img, int result, vector<light> redlgs, vector<light> greenlgs){

	int l = result / 4;
	int f = (result - l * 4) / 2;
	int r = result - l * 4 - f * 2;

	string msg = "left: " + to_string(l) + "   forward: " + to_string(f) + "   right: " + to_string(r);
	cout << msg << endl;

	string resFile = "images/" + to_string(l) + to_string(f) + to_string(r) + ".jpg";
	Mat mask = imread(resFile);
	if (!mask.data)
		cout << "fail to load mask image" << endl;
	Mat maskResize;
	resize(mask, maskResize, Size(), 0.5, 0.5);

	maskResize.copyTo(img(Rect(0, 0, maskResize.cols, maskResize.rows)));

	for (int i = 0; i < redlgs.size(); i++){
		circle(img, Point(redlgs[i].pos.x + redlgs[i].sz.x / 2, redlgs[i].pos.y + redlgs[i].sz.y / 2),
			max(redlgs[i].sz.x, redlgs[i].sz.y), Scalar(0, 255, 0));
	}
	for (int j = 0; j < greenlgs.size(); j++){
		circle(img, Point(greenlgs[j].pos.x + greenlgs[j].sz.x / 2, greenlgs[j].pos.y + greenlgs[j].sz.y / 2),
			max(greenlgs[j].sz.x, greenlgs[j].sz.y), Scalar(255, 0, 0));
	}

	return img;
}