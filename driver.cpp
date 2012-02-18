#include <opencv2/core/core.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>

using namespace cv;
using namespace std;

int vmin, vmax, smin, smax, hmin, hmax;

//test commit comment
Mat findBall(Mat frame) {
	Mat gray;
	Mat edges;

	cvtColor(frame, gray, CV_BGR2GRAY);
	    // smooth it, otherwise a lot of false circles may be detected
	    GaussianBlur( gray, gray, Size(9, 9), 2, 2 );
	    vector<Vec3f> circles;
	    HoughCircles(gray, circles, CV_HOUGH_GRADIENT,
	                 2, gray->rows/4, 200, 100 );
	    for( size_t i = 0; i < circles.size(); i++ )
	    {
	         Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
	         int radius = cvRound(circles[i][2]);
	         // draw the circle center
	         circle( img, center, 3, Scalar(0,255,0), -1, 8, 0 );
	         // draw the circle outline
	         circle( img, center, radius, Scalar(0,0,255), 3, 8, 0 );
	    }
	    return frame;
}

int main(int argc, char** argv)
{
	vmin = 20;
	smin = 55;
	hmin = 3;
	vmax = 255;
	smax = 22;
	hmax = 103;
	cv::VideoCapture videoCapture;

	Mat frame;

	videoCapture.open(1);

	cout << "Opened" << endl;

	videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

	cout << videoCapture.isOpened() << endl;
	videoCapture>>frame;
	cout << "First frame snagged" << endl;
	namedWindow("Output", CV_WINDOW_AUTOSIZE);
	createTrackbar( "Hmin", "Output", &hmin, 255, 0 );
	createTrackbar( "Hmax", "Output", &hmax, 255, 0 );
	createTrackbar( "Smin", "Output", &smin, 255, 0 );
	createTrackbar( "Smax", "Output", &smax, 255, 0 );
	createTrackbar( "Vmin", "Output", &vmin, 255, 0 );
	createTrackbar( "Vmax", "Output", &vmax, 255, 0 );

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
