#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <vector>
#include <stack>
using namespace cv;
using namespace std;

struct Data
{
	int minR, minC, maxR, maxC, length;
	double centerx, centery, radius;
	Data() {}
	Data(int minr, int minc, int maxr, int maxc)
	{
		minR = minr;
		minC = minc;
		maxR = maxr;
		maxC = maxc;
		centerx = 1.0*(maxC + minC) / 2.0;
		centery = 1.0*(maxR + minR) / 2.0;
		radius = 1.0*((maxC - minC) + (maxR - minR)) / 4.0;
		length = ((maxc - minc) + (maxr - minr)) / 2;
	}
};

int lc = 0, lr = 0, rc = 1280, rr = 1024;
Mat src;
int dc[8] = { 0, 1, 1, 1, 0, -1, -1, -1 }, dr[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
const double eps = 0.1;
bool used[2000][2000];
stack<pair<int, int>> sta;
vector<Data> circles, make_circles;
int numberOfRust;

bool cmp(Data d1, Data d2)
{
	return d1.length < d2.length;
}
void create_contours(); 
bool isStore(int minC, int minR, int maxC, int  maxR);
void find_adjacent();
void drawcircles();
void icvprCcaByTwoPass(const Mat& _binImg, Mat& _lableImg, int * numberOfPixelPerLabel, Point *minx, Point *miny, Point *maxx, Point *maxy);
Scalar icvprGetRandomColor();
void icvprLabelColor(const cv::Mat& _labelImg, cv::Mat& _colorLabelImg, int * numberOfPixelPerLabel, Point *minx, Point *miny, Point *maxx, Point *maxy, Mat &yuantu);
//numberOfPixelPerLabel[i]��ʾ��ͨ��i��numberOfPixelPerLabel[i]������
int numberOfPixelPerLabel[10000000];
Point minx[10000000], miny[10000000], maxx[10000000], maxy[10000000];
bool cmp2(Data a, Data b)
{
	return a.length > b.length;
}
void FindCircle(int &indexOfLittleCircle);
void FindRoi(char *image, int indexOfLittleCircle, IplImage* &my_src, IplImage* &res, IplImage* &roi, IplImage* &roi2, IplImage* &res2, IplImage* &annular);
void FindResult(char *image);
int main(int argc,char **argv)
{
	freopen("rust.txt","w",stdout) ;
	numberOfRust = 0;
	char * image = argv[1];
	IplImage *my_src, *res, *roi, *roi2, *res2, *annular;
	int indexOfLittleCircle = 0;
	src = imread(image, 1);
	FindCircle(indexOfLittleCircle);
	FindRoi(image, indexOfLittleCircle,my_src, res, roi, roi2, res2, annular);
	FindResult(image);
	
	cout << numberOfRust << endl;
	//system("pause");

	cvDestroyAllWindows();
	cvReleaseImage(&my_src);
	cvReleaseImage(&res);
	cvReleaseImage(&roi);
	cvReleaseImage(&annular);
	cvReleaseImage(&roi2);
	cvReleaseImage(&res2);
	cvReleaseImage(&annular);

	return 0;
}

void create_contours()
{
	Mat src_gray, canny_output;
	int thresh = 30, max_thresh = 255;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	char* window = "Source";

	cvtColor(src, src_gray, CV_BGR2GRAY);
	blur(src_gray, src_gray, Size(3, 3));

	Canny(src_gray, canny_output, thresh, thresh * 1.5, 3);

	findContours(canny_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
	Mat drawing = Mat::zeros(canny_output.size(), CV_8UC3);
	for (int i = 0; i < contours.size(); i++)
	{
		Scalar color = Scalar(255, 255, 255);//������ɫΪ��ɫ
		drawContours(drawing, contours, i, color, 1, 8, hierarchy, 0, Point());
	}
	imwrite("canny.jpg", drawing);
	//imshow("canny",drawing);
}

void find_adjacent()
{
	memset(used, false, sizeof used);
	Mat S, mid, dst;
	S = imread("canny.jpg");
	cvtColor(S, mid, CV_BGR2GRAY);
	threshold(mid, dst, 145, 255, THRESH_BINARY);

	for (int r = lr; r < rr; r++)
	{
		for (int c = lc; c < rc; c++)
		{
			if (!used[r][c] && dst.at<uchar>(r, c) == 255)
			{
				int minC = c, minR = r, maxC = c, maxR = r;
				sta.push(make_pair(r, c));
				used[r][c] = true;
				while (!sta.empty())
				{
					int tmpR = sta.top().first, tmpC = sta.top().second;
					sta.pop();
					for (int k = 0; k <8; k++)
					{
						int tc = tmpC + dc[k], tr = tmpR + dr[k];
						if (tr < rr && tr >= lr && tc < rc&& tc >= lc)
						{
							if (!used[tr][tc] && dst.at<uchar>(tr, tc) == 255)
							{
								sta.push(make_pair(tr, tc));
								used[tr][tc] = true;
								minC = min(minC, tc);
								minR = min(minR, tr);
								maxC = max(maxC, tc);
								maxR = max(maxR, tr);
							}
						}
					}
				}
				if (isStore(minC, minR, maxC, maxR))
				{
					Data d = Data(minR, minC, maxR, maxC);
					circles.push_back(d);
				}
			}
		}
	}
}

bool isStore(int minC, int minR, int maxC, int  maxR)
{
	int CL = maxC - minC, RL = maxR - minR;
	double rate = CL * 1.0 / RL;
	if (fabs(rate - 1) <= eps && RL >= 20 && CL >= 20)
	{
		return true;
	}
	return false;
}

void drawcircles()
{
	Mat drawing = Mat::zeros(src.size(), CV_8UC3);
	int minr, minc, maxr, maxc;
	minr = circles[0].minR;
	minc = circles[0].minC;
	maxr = circles[0].maxR;
	maxc = circles[0].maxC;
	for (int i = 1; i < circles.size(); i++)
	{
		minr = min(minr, circles[i].minR);
		minc = min(minc, circles[i].minC);
		maxr = max(maxr, circles[i].maxR);
		maxc = max(maxc, circles[i].maxC);
	}
	int length = ((maxc - minc) + (maxr - minr)) / 2;
	int centerR = (maxr + minr) / 2;
	int centerC = (maxc + minc) / 2;

	for (int i = 0; i <circles.size(); i++)
	{
		int tempC = (circles[i].maxC + circles[i].minC) / 2;
		int tempR = (circles[i].maxR + circles[i].minR) / 2;
		if (circles[i].minC <= minc + 50 && circles[i].minC != minc) continue;
		if (tempC >= centerC - 50 && tempC <= centerC + 50 && tempR <= centerR + 50 && tempR >= centerR - 50)
		{
			make_circles.push_back(circles[i]);
		}
	}
	circles.clear();
	sort(make_circles.begin(), make_circles.end(), cmp);

	Data d1 = make_circles[0];
	Data d2 = make_circles[1];
	circles.push_back(d1);
	circles.push_back(d2);
	for (int i = 0; i < 2; i++)
	{
		Point center((make_circles[i].maxC + make_circles[i].minC) / 2, (make_circles[i].maxR + make_circles[i].minR) / 2);
		int radius = ((make_circles[i].maxC - make_circles[i].minC) + (make_circles[i].maxR - make_circles[i].minR)) / 4;

		circle(src, center, radius, Scalar(0, 255, 255), 1, 8, 0);
	}

	int std_length = (d1.length + d2.length) / 2;
	for (int i = 2; i < make_circles.size(); i++)
	{
		Point center((make_circles[i].maxC + make_circles[i].minC) / 2, (make_circles[i].maxR + make_circles[i].minR) / 2);
		int radius = ((make_circles[i].maxC - make_circles[i].minC) + (make_circles[i].maxR - make_circles[i].minR)) / 4;
		circle(src, center, radius, Scalar(0, 255, 255), 1, 8, 0);
		circles.push_back(make_circles[i]);
	}
	//imshow("window", src);
	imwrite("circles.jpg", src);
}

void icvprCcaByTwoPass(const Mat& _binImg, Mat& _lableImg, int * numberOfPixelPerLabel, Point *minx, Point *miny, Point *maxx, Point *maxy)
{
	// connected component analysis (4-component)  
	// use two-pass algorithm  
	// 1. first pass: label each foreground pixel with a label  
	// 2. second pass: visit each labeled pixel and merge neighbor labels  
	//   
	// foreground pixel: _binImg(x,y) = 1  
	// background pixel: _binImg(x,y) = 0  

	memset(numberOfPixelPerLabel, 0, sizeof numberOfPixelPerLabel);
	memset(maxx, 0, sizeof maxx);
	memset(maxy, 0, sizeof maxy);
	for (int i = 0; i < 10000000; i++){
		minx[i].x = minx[i].y = 1e9;
		miny[i].x = miny[i].y = 1e9;
	}
	if (_binImg.empty() ||
		_binImg.type() != CV_8UC1)
	{
		return;
	}

	// 1. first pass  
	_lableImg.release();
	_binImg.convertTo(_lableImg, CV_32SC1);

	int label = 1;  // start by 2  
	std::vector<int> labelSet;

	labelSet.push_back(0);   // background: 0  
	labelSet.push_back(1);   // foreground: 1  

	int rows = _binImg.rows;
	int cols = _binImg.cols;
	for (int i = 1; i < rows; i++)
	{
		int* data_preRow = _lableImg.ptr<int>(i - 1);
		int* data_curRow = _lableImg.ptr<int>(i);
		for (int j = 1; j < cols; j++)
		{
			if (data_curRow[j] == 1)
			{
				std::vector<int> neighborLabels;
				neighborLabels.reserve(2);
				int leftPixel = data_curRow[j - 1];
				int upPixel = data_preRow[j];
				if (leftPixel > 1)
				{
					neighborLabels.push_back(leftPixel);
				}
				if (upPixel > 1)
				{
					neighborLabels.push_back(upPixel);
				}

				if (neighborLabels.empty())
				{
					labelSet.push_back(++label);  // assign to a new label  
					data_curRow[j] = label;
					labelSet[label] = label;
				}
				else
				{
					std::sort(neighborLabels.begin(), neighborLabels.end());
					int smallestLabel = neighborLabels[0];
					data_curRow[j] = smallestLabel;

					// save equivalence  
					for (size_t k = 1; k < neighborLabels.size(); k++)
					{
						int tempLabel = neighborLabels[k];
						int& oldSmallestLabel = labelSet[tempLabel];
						if (oldSmallestLabel > smallestLabel)
						{
							labelSet[oldSmallestLabel] = smallestLabel;
							oldSmallestLabel = smallestLabel;
						}
						else if (oldSmallestLabel < smallestLabel)
						{
							labelSet[smallestLabel] = oldSmallestLabel;
						}
					}
				}
			}
		}
	}

	// update equivalent labels  
	// assigned with the smallest label in each equivalent label set  
	for (size_t i = 2; i < labelSet.size(); i++)
	{
		int curLabel = labelSet[i];
		int preLabel = labelSet[curLabel];
		while (preLabel != curLabel)
		{
			curLabel = preLabel;
			preLabel = labelSet[preLabel];
		}
		labelSet[i] = curLabel;
	}

	int x = 0;
	// 2. second pass  
	for (int i = 0; i < rows; i++)
	{
		int * data = _lableImg.ptr<int>(i);
		for (int j = 0; j < cols; j++)
		{

			int& pixelLabel = data[j];
			pixelLabel = labelSet[pixelLabel];
			numberOfPixelPerLabel[pixelLabel * 10] += 1;
			if (minx[pixelLabel * 10].x>j){
				minx[pixelLabel * 10].x = j;
				minx[pixelLabel * 10].y = i;
			}

			if (miny[pixelLabel * 10].y > i){
				miny[pixelLabel * 10].y = i;
				miny[pixelLabel * 10].x = j;
			}

			if (maxx[pixelLabel * 10].x < j){
				maxx[pixelLabel * 10].x = j;
				maxx[pixelLabel * 10].y = i;
			}

			if (maxy[pixelLabel * 10].y < i){
				maxy[pixelLabel * 10].y = i;
				maxy[pixelLabel * 10].x = j;
			}
		}
	}

}

Scalar icvprGetRandomColor()
{
	uchar r = 255 * (rand() / (1.0 + RAND_MAX));
	uchar g = 255 * (rand() / (1.0 + RAND_MAX));
	uchar b = 255 * (rand() / (1.0 + RAND_MAX));
	return cv::Scalar(b, g, r);
}

void icvprLabelColor(const cv::Mat& _labelImg, cv::Mat& _colorLabelImg, int * numberOfPixelPerLabel, Point *minx, Point *miny, Point *maxx, Point *maxy, Mat &yuantu)
{

	if (_labelImg.empty() ||
		_labelImg.type() != CV_32SC1)
	{
		return;
	}

	std::map<int, cv::Scalar> colors;

	int rows = _labelImg.rows;
	int cols = _labelImg.cols;

	_colorLabelImg.release();
	_colorLabelImg.create(rows, cols, CV_8UC3);
	_colorLabelImg = cv::Scalar::all(0);
	for (int i = 10; i < rows; i++)
	{
		const int* data_src = (int*)_labelImg.ptr<int>(i);
		uchar* data_dst = _colorLabelImg.ptr<uchar>(i);
		for (int j = 10; j < cols; j++)
		{
			int pixelValue = data_src[j];
			if (pixelValue > 1)
			{
				int a = abs(maxx[pixelValue].x - minx[pixelValue].x);
				int b = abs(maxy[pixelValue].y - miny[pixelValue].y);
				if (a>b) swap(a, b);

				if ((1.0*b) / (1.0*a)<1.5  &&colors.count(pixelValue) <= 0 && numberOfPixelPerLabel[pixelValue] > 30 && numberOfPixelPerLabel[pixelValue] < 300)
				{
					numberOfRust++;
					Point center = Point(1.0*(miny[pixelValue].x + maxy[pixelValue].x) / 2, 1.0*(miny[pixelValue].y + maxy[pixelValue].y) / 2);
					rectangle(yuantu, Point(minx[pixelValue].x, miny[pixelValue].y), Point(maxx[pixelValue].x, maxy[pixelValue].y), Scalar(0, 0, 255));
					circle(_colorLabelImg, center, 50, Scalar(255, 0, 0));
					circle(_colorLabelImg, center, 70, Scalar(0, 0, 255));
					colors[pixelValue] = icvprGetRandomColor();
				}
				cv::Scalar color = colors[pixelValue];
				*data_dst++ = color[0];
				*data_dst++ = color[1];
				*data_dst++ = color[2];
			}
			else
			{
				data_dst++;
				data_dst++;
				data_dst++;
			}
		}
	}
}


void FindCircle(int &indexOfLittleCircle)
{
	create_contours();
	find_adjacent();
	drawcircles();
	sort(circles.begin(), circles.end(), cmp2);
	for (int i = 1; i < circles.size(); i++){
		if (fabs(circles[i].centerx - circles[0].centerx) < 50 && fabs(circles[i].centery - circles[0].centery) < 50 && circles[0].radius - circles[i].radius>100 && circles[0].radius - circles[i].radius<250){
			indexOfLittleCircle = i;
			break;
		}
	}
}
void FindRoi(char *image, int indexOfLittleCircle, IplImage* &my_src, IplImage* &res, IplImage* &roi, IplImage* &roi2, IplImage* &res2, IplImage* &annular)
{

	my_src = cvLoadImage(image, 1);
	res = cvCreateImage(cvGetSize(my_src), 8, 3);
	res2 = cvCreateImage(cvGetSize(my_src), 8, 3);
	roi = cvCreateImage(cvGetSize(my_src), 8, 1);
	roi2 = cvCreateImage(cvGetSize(my_src), 8, 1);
	annular = cvCreateImage(cvGetSize(my_src), 8, 3);
	/* prepare the 'ROI' image */
	cvZero(roi);
	cvZero(roi2);
	/* Note that you can use any shape for the ROI */
	cvCircle(
		roi,
		cvPoint(circles[0].centerx, circles[0].centery),
		circles[0].radius,
		CV_RGB(255, 255, 255),
		-1, 8, 0
		);
	cvCircle(
		roi2,
		cvPoint(circles[indexOfLittleCircle].centerx, circles[indexOfLittleCircle].centery), circles[indexOfLittleCircle].radius + 10,
		CV_RGB(255, 255, 255),
		-1, 8, 0
		);
	/* extract subimage */
	//src��src���� �� �����res ,ָ��������Ϊroi ,����������ȥ
	cvAnd(my_src, my_src, res, roi);
	cvAnd(my_src, my_src, res2, roi2);

	/*
	* do the main processing with subimage here.
	* in this example, we simply invert the subimage
	*/
	//��һ��res��λȡ��������ڶ���res
	//cvNot(res, res);
	//cvNot(res2, res2);

	/* 'restore' subimage */
	IplImage* roi_C3 = cvCreateImage(cvGetSize(my_src), 8, 3);
	cvMerge(roi, roi, roi, NULL, roi_C3);
	cvAnd(res, roi_C3, res, NULL);
	IplImage* roi2_C3 = cvCreateImage(cvGetSize(my_src), 8, 3);
	cvMerge(roi2, roi2, roi2, NULL, roi2_C3);
	cvAnd(res2, roi2_C3, res2, NULL);
	/*
	������Щ��Ϊ�˻�ԭ����Ȥ����ROI)�ı���ͼ���������Ŀ���ò���
	//merge subimage with original image
	cvNot(roi, roi);
	cvAdd(src, res, res, roi);
	*/
	/* show result */
	cvXor(res, res2, annular);

	//cvNot(annular, annular);
	//cvShowImage("annular", annular);
	/* be tidy */

	Mat result = cvarrToMat(annular, true);
	//imshow("result", result);
	imwrite("ROI.bmp", result);
}
void FindResult(char *image)
{
	Mat binImage = imread("ROI.bmp", 0);
	//ͼ���ֵ��
	threshold(binImage, binImage, 100, 1, CV_THRESH_BINARY_INV);

	Mat labelImg;
	icvprCcaByTwoPass(binImage, labelImg, numberOfPixelPerLabel, minx, miny, maxx, maxy);

	Mat grayImg;
	labelImg *= 10;
	labelImg.convertTo(grayImg, CV_8UC1);

	Mat yuantu;
	Mat colorLabelImg;
	yuantu = imread(image);
	icvprLabelColor(labelImg, colorLabelImg, numberOfPixelPerLabel, minx, miny, maxx, maxy, yuantu);
	//imshow("ԭͼ", yuantu);
	//imshow("colorImg", colorLabelImg);
	//imshow("labelImg", grayImg);
	imwrite("../../store/rustResult.jpg", yuantu);
}