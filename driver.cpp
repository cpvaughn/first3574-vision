#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <unistd.h>

#include <iostream>
#include "Socket/ClientSocket.h"

using namespace cv;
using namespace std;

int vmin, vmax, smin, smax, hmin, hmax;


// Flag set by the compiler to tell the program what mode to start in
// DEV = developer machine with a static video
// VSP_DEBUG = On the VSP, but displaying frames
// VSP_RELEASE = On the VSP, no graphical output

#ifndef HOST_MODE
#define HOST_MODE DEV
#endif

void sendMessage(string msg) {
    ClientSocket *client_socket = new ClientSocket( "10.35.74.2", 5001 );
	*client_socket << msg;
	delete(client_socket);
}

/*
 * Euclidean distance between two points
 */
double euclidDistance(Point p1, Point p2) {

	double xDelta = p1.x - p2.x;
	double yDelta = p1.y - p2.y;

	return sqrt(xDelta * xDelta + yDelta * yDelta);
}

Mat findRectangles(Mat frame, bool showMask) {
	Mat hsv;
	Mat thresh;

	cvtColor(frame, hsv, CV_BGR2HSV);

	inRange(hsv, Scalar(hmin, smin, vmin), Scalar(hmax, smax, vmax), thresh);

	Mat savedThresh = thresh.clone();

	if (showMask) {
		imshow("Mask", savedThresh);
	}

	//Track Square
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	findContours(thresh, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

	vector<Point> approx;

	// Stores the rectangle and it's index for hierarchy checks
	vector<vector<Point> > squares;
	vector<int> goodRects;


	// test each contour
	for( size_t i = 0; i < contours.size(); i++ )
	{
		// approximate contour with accuracy proportional
		// to the contour perimeter
		approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true)*0.02, true);

		// square contours should have 4 vertices after approximation
		// relatively large area (to filter out noisy contours)
		// and be convex.
		// Note: absolute value of an area is used because
		// area may be positive or negative - in accordance with the
		// contour orientation
		if( approx.size() == 4 && // Only interested if there are exactly 4 points
				(hierarchy[i][2] >= 0 || // And if it's got another rectange inside
				hierarchy[i][3] >= 0) && // or it is inside another rectangle
				fabs(contourArea(Mat(approx))) > 500 &&
				isContourConvex(Mat(approx)) )
		{
			squares.push_back(approx);
				goodRects.push_back(i);
		}
	}

	//draw loop
	for( size_t i = 0; i < goodRects.size(); i++ )
	{

		vector<Point> currentRectangle = squares[i];
		const Point* p = &currentRectangle[0];
		int n = (int)currentRectangle.size();

		// Red in BGR
		Scalar color = Scalar(0,0,255);

		// Is an inner rectangle
		if (hierarchy[goodRects[i]][3] >= 0) {
			color = Scalar(255,0,0);
		}

		//draw rectangles
		polylines(frame, &p, &n, 1, true, color, 2, CV_AA);


		// Check to make sure this rectangle has another inside it
        if (hierarchy[goodRects[i]][2] >= 0) {
        	/* Tell the difference between:
        	 *
        	 * 1-------2
        	 * |       |
        	 * |       |
        	 * 0-------3
        	 *
        	 * 2-------3
        	 * |       |
        	 * |       |
        	 * 1-------0
        	 *
        	 *
        	 */
        	double d1, d2; // The two horizontal lines
        	if (euclidDistance(currentRectangle[0],currentRectangle[1]) > euclidDistance(currentRectangle[1],currentRectangle[2])) {
        		d1 = euclidDistance(currentRectangle[0],currentRectangle[1]);
        		d2 = euclidDistance(currentRectangle[2],currentRectangle[3]);

        		line(frame, currentRectangle[0], currentRectangle[1], Scalar(0,255,255), 3, CV_AA);
        		line(frame, currentRectangle[2], currentRectangle[3], Scalar(0,255,255), 3, CV_AA);
        	} else {
        		d1 = euclidDistance(currentRectangle[1],currentRectangle[2]);
        		d2 = euclidDistance(currentRectangle[0],currentRectangle[3]);
        		line(frame, currentRectangle[1], currentRectangle[2], Scalar(0,255,255), 3, CV_AA);
        		line(frame, currentRectangle[0], currentRectangle[3], Scalar(0,255,255), 3, CV_AA);

        	}

        	//cout << d1 << ' ' << d2 << endl;

        	double midPointDistance = (d1 + d2) / 2;


        	double fovScale = 2.0 * 0.5317; // 2 * tan(56deg / 2)
        	cout << (640.0 / midPointDistance) * fovScale * 2.85 << " away-ish" << endl;
        }

		int centerX = (currentRectangle[0].x + currentRectangle[1].x + currentRectangle[2].x + currentRectangle[3].x)/4;
		int centerY = (currentRectangle[0].y + currentRectangle[1].y + currentRectangle[2].y + currentRectangle[3].y)/4;

		Point rectCenter(centerX,centerY);

		circle(frame,rectCenter, 5, Scalar(0,0,255),1, CV_AA);
		//log circles
		//cout <<Mat(rectCenter)<<endl;
	}
	return frame ;
}

int main(int argc, char** argv)
{


	vmin = 60;
	smin = 120;
	hmin = 41;
	vmax = 255;
	smax = 255;
	hmax = 90;

	cv::VideoCapture videoCapture;

	Mat frame;
	Mat savedFrame;

#if HOST_MODE == DEV
	videoCapture.open("/home/charles/full_range.mpg");
#else
	videoCapture.open(0);
#endif

	cout << "Opened" << endl;

	videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

	cout << videoCapture.isOpened() << endl;
	videoCapture>>frame;
	cout << "First frame snagged" << endl;
	namedWindow("Controls", CV_WINDOW_AUTOSIZE);
	createTrackbar( "Hmin", "Controls", &hmin, 255, 0 );
	createTrackbar( "Hmax", "Controls", &hmax, 255, 0 );
	createTrackbar( "Smin", "Controls", &smin, 255, 0 );
	createTrackbar( "Smax", "Controls", &smax, 255, 0 );
	createTrackbar( "Vmin", "Controls", &vmin, 255, 0 );
	createTrackbar( "Vmax", "Controls", &vmax, 255, 0 );

	int fc = 0;

    while(true) {
    	fc++;
    	videoCapture>>frame;

    	//cout << "Captured " << fc << endl;

    	savedFrame = findRectangles(frame, (fc % 10) == 0);

    	if (fc % 10 == 0) {
    	  imshow("Output", savedFrame);
    	}
    	if (waitKey(1) > 0)
    		break;

    }


    waitKey();

    return 0;
}
