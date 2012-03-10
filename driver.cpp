#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"

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

Mat camera_matrix, distortion_matrix;

Point2f findMidPoint(vector<Point> p) {
	Point2f mp;

	for (size_t i = 0; i <p.size(); i++) {
		mp.x += p[i].x;
		mp.y += p[i].y;
	}

	mp.x /= (float)p.size();
	mp.y /= (float)p.size();

	return mp;
}

// Returns the points starting from top left, going clockwise
vector<Point2f> orderPoints(vector<Point> pts, Point2f ctr) {
	vector<Point2f> rval(4);

	for (size_t p = 0; p < pts.size(); p++) {
		Point curPoint = pts[p];

		//430.75, 405.5
		//409, 388; 453, 423; 447, 388; 414, 423;
/*
		matchPoints.push_back(Point3f(-14, -10, 0)); // tl
		matchPoints.push_back(Point3f(14, -10, 0)); // tr
		matchPoints.push_back(Point3f(14, 10, 0)); // br
		matchPoints.push_back(Point3f(-14, 10, 0)); //bl
*/
		if (curPoint.x > ctr.x) {
			if (curPoint.y > ctr.y) { // bottom right
				rval[2] = Point2f(curPoint.x, curPoint.y);
			} else { // top right
				rval[1] = Point2f(curPoint.x, curPoint.y);
			}
		} else {
			if (curPoint.y > ctr.y) {
				rval[3] = Point2f(curPoint.x, curPoint.y);
			} else {
				rval[0] = Point2f(curPoint.x, curPoint.y);
			}
		}
	}

	return rval;
}
Mat tvec, rvec;
bool useGuess = false;
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

	vector<Point> backBoards;

	// test each contour
	for( size_t i = 0; i < contours.size(); i++ )
	{
		// approximate contour with accuracy proportional
		// to the contour perimeter
		approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true)*0.02, true);

		contours[i] = approx;

		// square contours should have 4 vertices after approximation
		// relatively large area (to filter out noisy contours)
		// and be convex.
		// Note: absolute value of an area is used because
		// area may be positive or negative - in accordance with the
		// contour orientation
		if( approx.size() == 4 && // Only interested if there are exactly 4 points
				(hierarchy[i][2] >= 0 /*|| // And if it's got another rectange inside
				hierarchy[i][3] >= 0*/) && // or it is inside another rectangle
				fabs(contourArea(Mat(approx))) > 500 &&
				isContourConvex(Mat(approx)) )
		{
			squares.push_back(approx);
				goodRects.push_back(i);

			backBoards.push_back(Point(i, hierarchy[i][2]));
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
		int centerX = (currentRectangle[0].x + currentRectangle[1].x + currentRectangle[2].x + currentRectangle[3].x)/4;
		int centerY = (currentRectangle[0].y + currentRectangle[1].y + currentRectangle[2].y + currentRectangle[3].y)/4;
		Point rectCenter(centerX,centerY);
		circle(frame,rectCenter, 5, Scalar(0,0,255),1, CV_AA);
	}


	//draw loop
	for( size_t i = 0; i < backBoards.size(); i++ )
	{

		vector<Point> outer = contours[backBoards[i].x];
		vector<Point> inner = contours[backBoards[i].y];

		Point2f oC = findMidPoint(outer);
		Point2f iC = findMidPoint(inner);

		//cout << oC << ' ' << iC << endl;

		vector<Point2f> oO = orderPoints(outer, oC);
		vector<Point2f> iO = orderPoints(inner, iC);


		vector<Point3f> matchPoints;
		vector<Point2f> sourcePoints;


		matchPoints.push_back(Point3f(-14, -10, 0)); // tl
		matchPoints.push_back(Point3f(14, -10, 0)); // tr
		matchPoints.push_back(Point3f(14, 10, 0)); // br
		matchPoints.push_back(Point3f(-14, 10, 0)); //bl
		matchPoints.push_back(Point3f(-12, -8, 0)); // tl
		matchPoints.push_back(Point3f(12, -8, 0)); // tr
		matchPoints.push_back(Point3f(12, 8, 0)); // br
		matchPoints.push_back(Point3f(-12, 8, 0)); //bl


		for (size_t i = 0; i < oO.size(); i++) {
			sourcePoints.push_back(oO[i]);
		}

		for (size_t i = 0; i < iO.size(); i++) {
			sourcePoints.push_back(iO[i]);
		}

		//cout << sourcePoints << endl;


		solvePnPRansac(Mat(matchPoints), Mat(sourcePoints), camera_matrix, distortion_matrix, rvec, tvec, false, 30);
		useGuess = true;

		//cout << tvec << ' ' << rvec << endl;

		double d = (tvec.ptr<double>(0))[2];
		cout << d << " in away (" << (d / 12.) << " ft)" << endl;

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

	// This stores camera parameters from camera_calibration, used in solvePnPRansac
	cv::FileStorage fs("ps3eye.yml", FileStorage::READ);


	fs["camera_matrix"] >> camera_matrix;
	fs["distortion_coefficients"] >> distortion_matrix;

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
