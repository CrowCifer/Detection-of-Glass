#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
#include <algorithm>
#include <iostream>
#include <cstring>
#include <utility>
#include <cstdio>
#include <vector>
#include <queue>
#include <map>

#define PI 3.14159265358979323

using namespace cv;
using namespace std;

const int thresh = 80;
const int max_thresh = 255;

double Stan = 100;
int dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
int dy[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
RNG rng(12345);

struct cmp {
	bool operator () (const Point& a, const Point& b) const {
		return a.x == b.x ? a.y < b.y : a.x < b.x;
	}
};

bool check1(vector<Point> ct) {
	bool res = true;
	map<Point, bool, cmp> exist;
	exist.clear();
	map<Point, int, cmp> deg;
	deg.clear();
	for (int i = 0; i < ct.size(); i++) {
		exist[ct[i]] = true;
	}
	for (int i = 0; i < ct.size(); i++) {
		for (int k = 0; k < 8; k++) {
			int tx = ct[i].x + dx[k];
			int ty = ct[i].y + dy[k];
			Point tmp = Point(tx, ty);
			if (exist[tmp] == true) {
				deg[ct[i]]++;
			}
		}
		if (deg[ct[i]] != 2) res = false;
	}
	return res;
}

map<pair<int, int>, int> mp;

void bfs(double& s, double& c, double& mx, double& mn, char* str, int x, int y) {
	IplImage* img = cvLoadImage(str);
	mp.clear();
	queue<pair<int, int>> que;
	pair<int, int> spe;
	que.push(make_pair(x, y));
	while (!que.empty()) {
		pair<int, int> t = que.front();
		mp[t]++;
		que.pop();
		double value = (cvGet2D(img, t.second, t.first).val[0] + cvGet2D(img, t.second, t.first).val[1] + cvGet2D(img, t.second, t.first).val[2]) / 3;
		mx = max(value, mx);
		mn = min(value, mn);
		s += value;
		c += 1.0;
		for (int i = 0; i < 8; i += 2) {
			int tx = dx[i] + t.first;
			int ty = dy[i] + t.second;
			double vl = (cvGet2D(img, t.second, t.first).val[0] + cvGet2D(img, t.second, t.first).val[1] + cvGet2D(img, t.second, t.first).val[2]) / 3;
			if (vl < 1e-7) {
				spe = make_pair(tx, ty);
				continue;
			}
			if (mp[make_pair(tx, ty)] == 0 && vl > 1e-7) {
				mp[make_pair(tx, ty)]++;
				que.push(make_pair(tx, ty));
			}
		}
	}
}

bool check2(vector<Point> ct, RotatedRect el, Mat img, char *str) {
	Mat tmp = img.clone(), nus;
	Scalar color = Scalar(0, 0, 0);
	ellipse(tmp, el, color, 4, 8);
	imwrite("buf.bmp", tmp);
	nus = img.clone();
	double sum = 0.0, cnt = 0.0, Max = -1e9, Min = 1e9;
	double sm = 0.0, cn = 0.0, mx = -1e9, mn = 1e9;
	bfs(sum, cnt, Max, Min, "buf.bmp", (int)el.center.x, (int)el.center.y);
	//cout << Max << " " << Min << " " << sum / cnt << " " << Stan << endl;
	if (fabs(sum / cnt - Stan) > 40) return false;
	return true;
}

void CheckEllipse(vector<vector<Point>> &ct, Mat img, char *str) {
	const double MAX_SIZE = 2000;
	const double MIN_SIZE = 300;
	Mat threshold_output;
	Mat drawing = img.clone();
	vector<RotatedRect> minRect(ct.size());
	vector<RotatedRect> minEllipse(ct.size());
	// 求椭圆，和外接矩形
	for (int i = 0; i < ct.size(); i++) {
		minRect[i] = minAreaRect(Mat(ct[i]));
		if (ct[i].size() > 5) {
			minEllipse[i] = fitEllipse(Mat(ct[i]));
		}
	}
	vector<vector<Point>>::iterator it = ct.begin();
	vector<RotatedRect>::iterator it1 = minRect.begin(), it2 = minEllipse.begin();
	while (it != ct.end()) {
		if (it1->size.area() > MAX_SIZE || it2->size.area() > MAX_SIZE || it1->size.area() < MIN_SIZE || it2->size.area() < MIN_SIZE || (it1->size.height / it1->size.width) > 5 || (it1->size.height / it1->size.width) < 0.2 || (it2->size.height / it2->size.width) > 4 || (it2->size.height / it2->size.width) < 0.25) {
			it1 = minRect.erase(it1);
			it = ct.erase(it);
			it2 = minEllipse.erase(it2);
		}
		else it++, it1++, it2++;
	}
	it = ct.begin(), it1 = minRect.begin(), it2 = minEllipse.begin();
	while (it != ct.end()) {
		if (!check2(*it, *it2, drawing, str)) {
			it1 = minRect.erase(it1);
			it = ct.erase(it);
			it2 = minEllipse.erase(it2);
		}
		else it++, it1++, it2++;
	}
	for (int i = 0; i < ct.size(); i++) {
		Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
		ellipse(drawing, minEllipse[i], color, 2, 8);
		Point2f rect_points[4]; minRect[i].points(rect_points);
		for (int j = 0; j < 4; j++)
			line(drawing, rect_points[j], rect_points[(j + 1) % 4], color, 1, 8);
	}
	namedWindow("Contours", CV_WINDOW_AUTOSIZE);
	imshow("Contours", drawing);
}


void ContoursValidiation(vector<vector<Point>>& ct)
{
	const int MIN_SIZE = 50;
	const int MAX_SIZE = 500;
	vector<vector<Point>>::iterator it = ct.begin();
	while (it != ct.end()) {
		if (it->size() > MAX_SIZE || it->size() < MIN_SIZE) { // 大小
			it = ct.erase(it);
		}
		else if (check1(*it)) { // 闭合
			it = ct.erase(it);
		}
		else it++;
	}
}

int main(int argc, char * argv[]) {
	Mat image = imread("C:/Users/aaa/Documents/Visual Studio 2013/Projects/bv/x64/Debug/32.bmp", 0);
	char *src = "C:/Users/aaa/Documents/Visual Studio 2013/Projects/bv/x64/Debug/32.bmp";
	Mat result1 = image.clone();
	IplImage* img = cvLoadImage(src);
	vector<double> gar;
	for (int i = 550; i <= 650; i++){
		for (int j = 450; j <= 550; j++) {
			gar.push_back((cvGet2D(img, j, i).val[0] + cvGet2D(img, j, i).val[1] + cvGet2D(img, j, i).val[2]) / 3);
		}
	}
	sort(gar.begin(), gar.end());
	double s1 = 0, c1 = 0;
	for (int i = gar.size() / 10; i < gar.size() - gar.size() / 10; i++) {
		c1++;
		s1 += gar[i];
	}
	Stan = s1 / c1;
	//imshow("das", result1);
	vector<vector<Point>> contours;
	//imshow("原始", image);
	// Canny 边缘检测
	Canny(image, result1, 40, 120, 3);
	imshow("Canny", result1);
	// 提取轮廓
	findContours(result1, contours, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE, cvPoint(0, 0));
	// 检查轮廓的合法性，删去过大（小）的轮廓
	ContoursValidiation(contours);
	Mat result2 = image.clone();
	// 第一次检查的轮廓
	drawContours(result2, contours, -1, Scalar(0), 2);   // -1 表示所有轮廓
	//imshow("删去条件外的轮廓", result2);
	// 检测外接矩形和椭圆的
	CheckEllipse(contours, image, src);
	waitKey(0);
	return 0;
}