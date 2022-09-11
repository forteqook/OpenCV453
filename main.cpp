#include "opencv2/opencv.hpp"
#include <iostream>
#include <algorithm>
#include <utility>

using namespace cv;
using namespace std;

typedef pair<Point, double> resPair;

Mat blackBox_src1, blackBox_src2;
Mat roadSign1, roadSign2, mask1, mask2;

Mat blackBox_hsv1, dst;

int lower_hue = 40, upper_hue = 80;

void blackBox1();
void blackBox2();

void on_hue_changed(int, void*);

struct Labeling {
	vector<Rect> vrc;
	vector<resPair> vres;

	void setRect(Mat& img, const vector<Point>& pts) {
		Rect rc = boundingRect(pts);
		rectangle(img, rc, Scalar(0, 0, 255), 1);
		vrc.push_back(rc);
	}

	void setLabel(Mat& img, vector<Mat>& templ) {
		Mat copied;
		img.copyTo(copied);
		int cnt = 0;

		copied = copied + Scalar(50, 50, 50);

		Mat noise(copied.size(), CV_32SC3);
		randn(noise, 0, 10);
		add(copied, noise, copied, Mat(), CV_8UC3);
		cvtColor(copied, copied, COLOR_BGR2GRAY);

		for (Mat& t : templ) {
			//cout << "now" << cnt << endl;
			String name = to_string(cnt);

			for (Rect& r : vrc) {
				// 템플릿 이미지 사이즈를 rect 사이즈로 변경
				Mat m;
				resize(t, m, r.size());

				Mat res;
				matchTemplate(copied, m, res, TM_CCOEFF_NORMED);

				double maxv;
				Point maxloc;
				minMaxLoc(res, 0, &maxv, 0, &maxloc);
				Point _p(maxloc.x + (int)(0.5 * m.cols), maxloc.y + (int)(0.5 * m.rows));

				//if (cnt == 0) {
				//	drawMarker(img, _p, Scalar(255, 0, 0), MARKER_CROSS);
				//	rectangle(img, Rect(maxloc.x, maxloc.y, m.cols, m.rows), Scalar(255, 0, 0), 1);
				//}
				//else {
				//	drawMarker(img, _p, Scalar(0, 255, 0), MARKER_CROSS);
				//	rectangle(img, Rect(maxloc.x, maxloc.y, m.cols, m.rows), Scalar(0, 255, 0), 1);
				//}
				//drawMarker(img, _p, Scalar(0, 255, 0), MARKER_CROSS);
				//cout << "_p" << _p << endl;

				vres.push_back(make_pair(_p, maxv));
			}

			Point p = vres[max_element(vres.begin(), vres.end(), [](const auto& lhs, const auto& rhs) {return lhs.second < rhs.second;}) - vres.begin()].first;

			//cout << p << endl;

			for (Rect& r : vrc) {
				if (r.contains(p)) {
					if (cnt == 0) {
						putText(img, "Speed Limit", r.tl(), FONT_HERSHEY_PLAIN, 1, Scalar(0, 0, 255));
					}
					else if (cnt == 1) {
						putText(img, "U-turn", r.tl(), FONT_HERSHEY_PLAIN, 1, Scalar(0, 0, 255));
					}
					else {
						putText(img, "Sign", r.tl(), FONT_HERSHEY_PLAIN, 1, Scalar(0, 0, 255));
					}
				}
				else cout << "nope" << endl;
			}

			vres.clear();

			cnt++;
		}
	}
};

int main(void) {
	// 이미지 불러오기
	blackBox_src1 = imread("homework3_blackbox1.jpg", IMREAD_COLOR);
	blackBox_src2 = imread("homework3_blackbox2.jpg", IMREAD_COLOR);
	roadSign1 = imread("homework3_50limit.jpg", IMREAD_GRAYSCALE);
	roadSign2 = imread("homework3_uturn_.png", IMREAD_GRAYSCALE);
	mask1 = imread("homework3_mask1.bmp", IMREAD_GRAYSCALE);
	mask2 = imread("homework3_mask2.jpg", IMREAD_GRAYSCALE);
	// 불러온 이미지 검사
	vector<Mat> vec;
	vec.push_back(blackBox_src1);
	vec.push_back(blackBox_src2);
	vec.push_back(roadSign1);
	vec.push_back(roadSign2);
	vec.push_back(mask1);
	vec.push_back(mask2);

	for (Mat& i : vec) {
		if (i.empty()) {
			cerr << "Image load failed" << endl;
			return -1;
		}
	}
	vec.clear();
	vector<Mat>().swap(vec);

	blackBox1();
	blackBox2();

	//cvtColor(blackBox_src1, blackBox_hsv1, COLOR_BGR2HSV);
	//namedWindow("dst");
	//createTrackbar("Lower Hue", "dst", &lower_hue, 179, on_hue_changed);
	//createTrackbar("Upper Hue", "dst", &upper_hue, 179, on_hue_changed);
	//on_hue_changed(0, 0);

	//1.Red range 168-179
	//1.Blue range 101-114
	//1.Yellow range 8-18

	//2.Red range 164-177
	//2.Blue range 105-116

	waitKey();
	return 0;
}

void blackBox1() {
	Mat blackBox_hsv1, blackBox_RBY, blackBox_R, blackBox_B, blackBox_Y;
	// 색 검출 특성을 증가시키기 위해, 히스토그램 평활화로 명암비 증가
	Mat src_ycrcb;
	cvtColor(blackBox_src1, src_ycrcb, COLOR_BGR2YCrCb);
	vector<Mat> ycrcb_planes;
	split(src_ycrcb, ycrcb_planes);
	equalizeHist(ycrcb_planes[0], ycrcb_planes[0]);

	Mat dst_ycrcb;
	merge(ycrcb_planes, dst_ycrcb);
	// 색 검출을 위해 HSV 공간 사용
	Mat _blackBox_src1;
	cvtColor(dst_ycrcb, _blackBox_src1, COLOR_YCrCb2BGR);
	cvtColor(_blackBox_src1, blackBox_hsv1, COLOR_BGR2HSV);
	// 미리 구해둔 hue range를 이용하여 색깔 별로 이미지 검출, 이후 합쳐줌
	Scalar lowerb_R(168, 100, 0), lowerb_B(101, 100, 0), lowerb_Y(8, 100, 0);
	Scalar upperb_R(179, 255, 255), upperb_B(114, 255, 255), upperb_Y(18, 255, 255);
	inRange(blackBox_hsv1, lowerb_R, upperb_R, blackBox_R);
	inRange(blackBox_hsv1, lowerb_B, upperb_B, blackBox_B);
	inRange(blackBox_hsv1, lowerb_Y, upperb_Y, blackBox_Y);
	blackBox_RBY = blackBox_R | blackBox_B | blackBox_Y;
	// 모폴로지 열기(opening)연산으로 노이즈 제거
	Mat blackBox_morph;
	morphologyEx(blackBox_RBY, blackBox_morph, MORPH_OPEN, Mat());

	vector<vector<Point>> contours;
	vector<double> contours_area;
	// 외곽선 검출
	findContours(blackBox_morph, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

	Mat blackBox_contours;
	cvtColor(blackBox_morph, blackBox_contours, COLOR_GRAY2BGR);
	// 외곽선 그리기
	for (int i = 0; i < contours.size(); i++) {
		Scalar c(rand() & 255, rand() & 255, rand() & 255);
		drawContours(blackBox_contours, contours, i, c, -1);
		//contours_area.push_back(contourArea(contours[i]));
	}

	// 이미지 면적 출력
	//sort(contours_area.begin(), contours_area.end(), greater<double>());
	//for (int i = 0; i < contours_area.size(); i++) {
	//	cout << contours_area[i] << ' ';
	//}
	//cout << endl;

	Labeling lb;

	for (vector<Point>& pts : contours) {
		if (contourArea(pts) < 4000) {
			continue;
		}
		lb.setRect(blackBox_src1, pts);
	}

	vector<Mat> templ = { roadSign1, roadSign2 };

	lb.setLabel(blackBox_src1, templ);

	imshow("dst1", blackBox_src1);
}

void blackBox2() {
	Mat blackBox_hsv2, blackBox_RB, blackBox_R, blackBox_B;

	cvtColor(blackBox_src2, blackBox_hsv2, COLOR_BGR2HSV);

	Scalar lowerb_R(164, 100, 0), lowerb_B(105, 100, 0);
	Scalar upperb_R(177, 255, 255), upperb_B(116, 255, 255);
	inRange(blackBox_hsv2, lowerb_R, upperb_R, blackBox_R);
	inRange(blackBox_hsv2, lowerb_B, upperb_B, blackBox_B);
	blackBox_RB = blackBox_R | blackBox_B;

	Mat blackBox_morph;
	morphologyEx(blackBox_RB, blackBox_morph, MORPH_OPEN, Mat());

	vector<vector<Point>> contours;
	vector<double> contours_area;
	findContours(blackBox_morph, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

	Mat blackBox_contours;
	cvtColor(blackBox_morph, blackBox_contours, COLOR_GRAY2BGR);

	for (int i = 0; i < contours.size(); i++) {
		Scalar c(rand() & 255, rand() & 255, rand() & 255);
		drawContours(blackBox_contours, contours, i, c, -1);
		contours_area.push_back(contourArea(contours[i]));
	}

	sort(contours_area.begin(), contours_area.end(), greater<double>());

	for (int i = 0; i < contours_area.size(); i++) {
		cout << contours_area[i] << ' ';
	}
	cout << endl;
	//upto 5, larger than 300 are needed
	//above condition is arbitrarly given

	Labeling lb;

	for (vector<Point>& pts : contours) {
		if (contourArea(pts) < 500) {
			continue;
		}
		lb.setRect(blackBox_src2, pts);
	}

	vector<Mat> templ = {roadSign1};

	lb.setLabel(blackBox_src2, templ);

	imshow("dst2", blackBox_src2);
}

void on_hue_changed(int, void*) {
	Scalar lowerb(lower_hue, 100, 0);
	Scalar upperb(upper_hue, 255, 255);
	inRange(blackBox_hsv1, lowerb, upperb, dst);

	imshow("dst", dst);
}