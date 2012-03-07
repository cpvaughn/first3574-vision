#include <opencv2/core/core.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>

using namespace cv;
using namespace std;

int vmin, vmax, smin, smax, hmin, hmax, vmin2, vmax2, smin2, smax2, hmin2, hmax2;
int hi = 311, lo = 65;

//test commit comment
Mat findBall(Mat frame) {
	Mat hsvFrame;
	Mat thresholded;
	Mat thresholded2;

	//color ranges
	//first color ball
	Scalar hsvMin = Scalar(hmin,smin,vmin,0);
	Scalar hsvMax = Scalar(hmax,smax,vmax,0);

	//second color ball
	//Scalar hsvMin2 = Scalar(hmin2,smin2,vmin2,0);
	//Scalar hsvMax2 = Scalar(hmax2,smax2,vmax2,0);

	//convert to hsv
	cvtColor(frame, hsvFrame, CV_RGB2HSV);

	//two halves are detected and combined to handle color wrap-around
	inRange(hsvFrame, hsvMin, hsvMax, thresholded);

	//inRange(thresholded, hsvMin2, hsvMax2, thresholded2);

	//add(thresholded, thresholded2, thresholded);

	//smooth image
	GaussianBlur(thresholded, thresholded, Size(lo,lo),0,0);

	Mat savedThresh = thresholded;

	//Storage for circles
	vector<Vec3f> circles;

	//find circles
	HoughCircles(thresholded, circles, CV_HOUGH_GRADIENT, 3, hi, 100.0,  50.0, 10, 400 );

	//Draw each circle
	for(size_t i = 0; i < circles.size(); i++)
	{
		Point center(round(circles[i][0]), round(circles[i][1]));
		float  radius = round(circles[i][2]);

		cout<<center;

		//draw circle center
		circle(frame, center, 3, Scalar(0,255,0), -1, 8, 0);

		//draw circle outline
		circle(frame, center, radius, Scalar(0,0,255), 3, 8, 0);
	}

	return frame;
}

int main(int argc, char** argv)
{
	vmin = 89;
	smin = 209;
	hmin = 97;
	vmax = 255;
	smax = 255;
	hmax = 139;

	vmin2 = 255;
	smin2 = 255;
	hmin2 = 55;
	vmax2 = 255;
	smax2 = 255;
	hmax2 = 255;

	cv::VideoCapture videoCapture;

	Mat frame;

	videoCapture.open(0);

	cout << "Opened" << endl;

	videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

	cout << videoCapture.isOpened() << endl;
	videoCapture>>frame;
	cout << "First frame snagged" << endl;
	namedWindow("Output", CV_WINDOW_AUTOSIZE);
	namedWindow("Sliders", CV_WINDOW_AUTOSIZE);

	createTrackbar( "Hmin", "Sliders", &hmin, 255, 0 );
	createTrackbar( "Hmax", "Sliders", &hmax, 255, 0 );
	createTrackbar( "Smin", "Sliders", &smin, 255, 0 );
	createTrackbar( "Smax", "Sliders", &smax, 255, 0 );
	createTrackbar( "Vmin", "Sliders", &vmin, 255, 0 );
	createTrackbar( "Vmax", "Sliders", &vmax, 255, 0 );
	createTrackbar("hi", "Sliders", &hi, 1000);
	createTrackbar("lo", "Sliders", &lo, 1000);
	createTrackbar( "Hmin2", "Sliders", &hmin2, 255, 0 );
	createTrackbar( "Hmax2", "Sliders", &hmax2, 255, 0 );
	createTrackbar( "Smin2", "Sliders", &smin2, 255, 0 );
	createTrackbar( "Smax2", "Sliders", &smax2, 255, 0 );
	createTrackbar( "Vmin2", "Sliders", &vmin2, 255, 0 );
	createTrackbar( "Vmax2", "Sliders", &vmax2, 255, 0 );

	while(true) {
		videoCapture>>frame;

		cout << "Captured" << endl;
		imshow("Output", findBall(frame));
		if (waitKey(10) > 0)
			break;

	}


	waitKey();

	return 0;
}
