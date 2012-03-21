#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
#define HOST_MODE 0
#endif

#define OUTER_WIDTH 24
#define OUTER_HEIGHT 18
#define INNER_WIDTH 20
#define INNER_HEIGHT 14

#define TOP_HOOP 0
#define BOTTOM_HOOP 1
#define LEFT_HOOP 2
#define RIGHT_HOOP 3





void addBasket(vector<Point3f> & pts, float x, float y) {
	pts.push_back(Point3f(-OUTER_WIDTH / 2 + x, -OUTER_HEIGHT / 2 + y, 0)); // tl
	pts.push_back(Point3f(OUTER_WIDTH / 2 + x, -OUTER_HEIGHT / 2 + y, 0)); // tr
	pts.push_back(Point3f(OUTER_WIDTH / 2 + x, OUTER_HEIGHT / 2 + y, 0)); // br
	pts.push_back(Point3f(-OUTER_WIDTH / 2 + x, OUTER_HEIGHT / 2 + y, 0)); //bl

	//Inner points
	pts.push_back(Point3f(-INNER_WIDTH / 2 + x, -INNER_HEIGHT / 2 + y, 0)); // tl
	pts.push_back(Point3f(INNER_WIDTH / 2 + x, -INNER_HEIGHT / 2 + y, 0)); // tr
	pts.push_back(Point3f(INNER_WIDTH / 2 + x, INNER_HEIGHT / 2 + y, 0)); // br
	pts.push_back(Point3f(-INNER_WIDTH / 2 + x, INNER_HEIGHT / 2 + y, 0)); //bl
}

void sendMessage(string msg) {
    ClientSocket *client_socket = new ClientSocket( "10.35.74.2", 5001 );
	*client_socket << msg;
	delete(client_socket);
}

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

struct Basket {
	vector<Point> inner;
	vector<Point> outer;
	int whichBasket;
	Point2f innerCenter;
	Point2f outerCenter;

	Basket(vector<Point> _inner, vector<Point> _outer) {
		inner = _inner;
		outer = _outer;

		innerCenter = findMidPoint(inner);
		outerCenter = findMidPoint(outer);

		whichBasket = -1;
	}
};

vector<Point2f> lastHoopPos(4);
vector<int> lastHoopFrame(4);

void initLastPos() {
	for (int i = 0; i < 4; i++) {
		lastHoopPos[i] = Point2f(-1000,-1000);
		lastHoopFrame[i] = -1;
	}
}

/*
 * Euclidean distance between two points
 */
double euclidDistance(Point p1, Point p2) {

	double xDelta = p1.x - p2.x;
	double yDelta = p1.y - p2.y;

	return sqrt(xDelta * xDelta + yDelta * yDelta);
}

void updateLastBaskets(vector<Basket *> baskets, int frameNum) {
	for (int i = 0; i < baskets.size(); i++) {
		if (baskets[i]->whichBasket >= 0) {
			lastHoopPos[baskets[i]->whichBasket] = baskets[i]->innerCenter;
			lastHoopFrame[baskets[i]->whichBasket] = frameNum;
		}
	}

	for (int i = 0; i < 4; i++) {
		if (frameNum - lastHoopFrame[i] > 10) {
			lastHoopPos[i] = Point2f(-1000,-1000);
		}
	}
}

bool inferBaskets(vector<Basket *> baskets, int frameNum) {
	if (baskets.size() > 4) { // Shouldn't happen, but give up for now
		return false;
	}

	if (baskets.size() < 1) {
		return false;
	}

	// Store the relevant dimension for the hoop positions, y for the lowest and highest basket, x for the middles

	float leftLeastPos = baskets[0]->innerCenter.x;
	float rightMostPos = baskets[0]->innerCenter.x;
	float topLeastPos = baskets[0]->innerCenter.y;
	float bottomMostPos = baskets[0]->innerCenter.y;

	size_t leftLeast = 0;
	size_t rightMost = 0;
	size_t topLeast = 0;
	size_t bottomMost = 0;

	for (size_t i = 1; i < baskets.size(); i++) {
		if (baskets[i]->innerCenter.x < leftLeastPos) {
			leftLeastPos = baskets[i]->innerCenter.x;
			leftLeast = i;
		}

		if (baskets[i]->innerCenter.x > rightMostPos) {
			rightMostPos = baskets[i]->innerCenter.x;
			rightMost = i;
		}

		if (baskets[i]->innerCenter.y < topLeastPos) {
			topLeastPos = baskets[i]->innerCenter.y;
			topLeast = i;
		}

		if (baskets[i]->innerCenter.y > bottomMostPos) {
			bottomMostPos = baskets[i]->innerCenter.y;
			bottomMost = i;
		}
	}

	if (baskets.size() == 4) { // Awesome, we've found all the baskets
		baskets[leftLeast]->whichBasket = LEFT_HOOP;
		baskets[topLeast]->whichBasket = TOP_HOOP;
		baskets[rightMost]->whichBasket = RIGHT_HOOP;
		baskets[bottomMost]->whichBasket = BOTTOM_HOOP;

		return true;
	}

	if (baskets.size() > 1) { // Use relative offset to detect if the remaining baskets are predominantly on the vertical, horizontal or diagonal
		float smallestHoriz = 10000;
		float smallestVert = 10000;
		int horiz1, horiz2;
		int vert1, vert2;

		for (int i = 0; i < baskets.size() - 1; i++) {
			for (int j = i + 1; j < baskets.size(); j++) {
				if (fabs(baskets[i]->innerCenter.x - baskets[j]->innerCenter.x) < smallestHoriz) {
					smallestHoriz = fabs(baskets[i]->innerCenter.x - baskets[j]->innerCenter.x);
					horiz1 = i;
					horiz2 = j;
				}
				if (fabs(baskets[i]->innerCenter.y - baskets[j]->innerCenter.y) < smallestVert) {
					smallestVert = fabs(baskets[i]->innerCenter.y - baskets[j]->innerCenter.y);
					vert1 = i;
					vert2 = j;
				}
			}
		}


		if (baskets.size() == 3) {
			if (smallestVert < smallestHoriz) { // Since the vertical delta is smaller than horizontal, we know we have both middle baskets
				baskets[leftLeast]->whichBasket = LEFT_HOOP;
				baskets[rightMost]->whichBasket = RIGHT_HOOP;

				if (topLeast == leftLeast || topLeast == rightMost) { // The 'top' hoop is actually one of the middle ones
					baskets[bottomMost]->whichBasket = BOTTOM_HOOP;
					return true;
				} else {
					baskets[topLeast]->whichBasket = TOP_HOOP;
					return true;
				}
			} else {
				baskets[topLeast]->whichBasket = TOP_HOOP;
				baskets[bottomMost]->whichBasket = BOTTOM_HOOP;
				if (leftLeast == topLeast || leftLeast == bottomMost) { // The 'left' hoop is actually one of the top or bottom
					baskets[rightMost]->whichBasket = RIGHT_HOOP;
					return true;
				} else {
					baskets[leftLeast]->whichBasket = LEFT_HOOP;
					return true;
				}
			}
		} else { // We've only got two
			if (smallestVert / smallestHoriz < 0.2) { // Vertical delta dominates, both hoops are the middle ones
				baskets[leftLeast]->whichBasket = LEFT_HOOP;
				baskets[rightMost]->whichBasket = RIGHT_HOOP;
				return true;
			} else if (smallestHoriz / smallestVert < 0.2) { // Horizontal delta dominates, hoops are top and bottom
				baskets[topLeast]->whichBasket = TOP_HOOP;
				baskets[bottomMost]->whichBasket = BOTTOM_HOOP;
				return true;
			} else { // Handle the diagonals

				if (topLeast == leftLeast) { // Either Top and right hoop, or left and bottom hoop
					if (baskets[bottomMost]->innerCenter.y > 240) { // it's the bottom
						baskets[bottomMost]->whichBasket = BOTTOM_HOOP;
						baskets[leftLeast]->whichBasket = LEFT_HOOP;
						return true;
					} else {
						baskets[rightMost]->whichBasket = RIGHT_HOOP;
						baskets[topLeast]->whichBasket = TOP_HOOP;
						return true;

					}
				} else { // Either top and left, or right and bottom
					if (baskets[topLeast]->innerCenter.y < 240) { // it's the top
						baskets[leftLeast]->whichBasket = LEFT_HOOP;
						baskets[topLeast]->whichBasket = TOP_HOOP;
						return true;
					} else {
						baskets[rightMost]->whichBasket = RIGHT_HOOP;
						baskets[bottomMost]->whichBasket = BOTTOM_HOOP;
						return true;

					}
				}
			}
		}
	}


	float dist = 100000;
	int closest = -1;

	for (int i = 0; i < lastHoopPos.size(); i++) {
		float d = euclidDistance(lastHoopPos[i], baskets[0]->innerCenter);

		if (d < dist) {
			dist = d;
			closest = i;
		}
	}

	if (dist < 100) {
		baskets[0]->whichBasket = closest;
		return true;
	}

	if (baskets[0]->innerCenter.y > 300) {
		baskets[0]->whichBasket = BOTTOM_HOOP;
	} else if (baskets[0]->innerCenter.y < 180) {
		baskets[0]->whichBasket = TOP_HOOP;
	} else if (baskets[0]->innerCenter.x < 320) {
		baskets[0]->whichBasket = LEFT_HOOP;
	} else {
		baskets[0]->whichBasket = RIGHT_HOOP;
	}

	return true;


}

Mat camera_matrix, distortion_matrix;



// Returns the points starting from top left, going clockwise
vector<Point2f> orderPoints(vector<Point> pts, Point2f ctr) {
	vector<Point2f> rval(4);

	for (size_t p = 0; p < pts.size(); p++) {
		Point curPoint = pts[p];

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

float distFromBasket(Basket b) {
	vector<Point2f> box;

	box = orderPoints(b.outer, b.outerCenter);

	float od1 = euclidDistance(box[0], box[1]);
	float od2 = euclidDistance(box[3], box[2]);

	box = orderPoints(b.inner, b.innerCenter);

	float id1 = euclidDistance(box[0], box[1]);
	float id2 = euclidDistance(box[3], box[2]);

	od1 = od1 / OUTER_HEIGHT;
	od2 = od2 / OUTER_HEIGHT;

	id1 = id1 / INNER_HEIGHT;
	id2 = id2 / INNER_HEIGHT;

	//cout << id1 << ' ' << id2 << ' ' << od1 << ' ' << od2 << endl;

	return (id1 + id2 + od1 + od2) / 4.f;
}

void lockOn(float ang, float dist, float r, float m, float l) {

	char buf[300];
	memset(buf, 0, 300);

	sprintf(buf, "a%f,d%f,r%f,m%f,l%f", ang, dist,r,m,l);

#if HOST_MODE == 1
	sendMessage(buf);
#else
	cout << buf << endl;
#endif

}

void drawBasket(Basket b, Mat f) {
	const Point* p1 = &b.outer[0];
	int n1 = (int)b.outer.size();

	const Point* p2 = &b.inner[0];
	int n2 = (int)b.inner.size();


	// Red in BGR
	Scalar color = Scalar(0,0,255);

	if (b.whichBasket < 0) {
		color = Scalar(255,255,255);
	} else if (b.whichBasket == 0) {
		color = Scalar(255,255,0);
	} else if (b.whichBasket == 1) {
		color = Scalar(255,0,0);
	} else if (b.whichBasket == 2) {
		color = Scalar(0,255,0);
	}

	circle(f, b.innerCenter, 5, color,1, CV_AA);
	circle(f, b.outerCenter, 5, color,1, CV_AA);

	//draw rectangles
	polylines(f, &p1, &n1, 1, true, color, 2, CV_AA);
	polylines(f, &p2, &n2, 1, true, color, 2, CV_AA);
}

void loadPnPvecs(vector<Basket *> baskets, vector<Point3f>& world, vector<Point2f>& camera) {
	for (size_t i = 0; i < baskets.size(); i++) {
		switch (baskets[i]->whichBasket) {
		case TOP_HOOP: addBasket(world, 0, 109); break;
		case BOTTOM_HOOP: addBasket(world, 0, 39); break;
		case LEFT_HOOP: addBasket(world, -27.380, 72); break;
		case RIGHT_HOOP: addBasket(world, 27.380, 72); break;
		}

		for (size_t j = 0; j < baskets[i]->outer.size(); j++) {
			camera.push_back(baskets[i]->outer[j]);
		}

		for (size_t j = 0; j < baskets[i]->inner.size(); j++) {
			camera.push_back(baskets[i]->inner[j]);
		}


	}
}

float getRotationalDelta(float xPos) {
	return (xPos - 320.0f) / 28.0f;
}

Mat tvec, rvec;
bool useGuess = false;
Mat findRectangles(Mat frame, bool showMask, int frameCount) {
	Mat hsv;
	Mat thresh;

	Mat cThresh;

	cvtColor(frame, hsv, CV_BGR2HSV);

	inRange(hsv, Scalar(hmin, smin, vmin), Scalar(hmax, smax, vmax), thresh);

	int morph_size=9;
	Mat element = getStructuringElement( CV_SHAPE_RECT, Size( 2*morph_size + 1, 2*morph_size+1 ), Point( morph_size, morph_size ) );




	morphologyEx( thresh, cThresh, CV_MOP_CLOSE, element );

	thresh = cThresh;

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

	vector<Basket *> bsks;

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

	for (int i = 0; i < backBoards.size(); i++) {
		vector<Point> inner, outer;

		outer = contours[backBoards[i].y];
		inner = contours[backBoards[i].x];
		bsks.push_back(new Basket(outer, inner));
	}

	int maxY = -100000;

	Point2i targetPoint(-1,-1);

	inferBaskets(bsks, frameCount);
	updateLastBaskets(bsks, frameCount);

	vector<Point3f> world;
	vector<Point2f> cam;

	float dFB = 0.f;

	for (int i = 0; i < bsks.size(); i++) {
		if (showMask)
			drawBasket(*bsks[i], frame);
		dFB += distFromBasket(*bsks[i]);
	}

	if (bsks.size() > 0) {
		float r = -10000;
		float m = -10000;
		float l = -10000;

		for (size_t i = 0; i < bsks.size(); i++) {
			switch (bsks[i]->whichBasket) {
			case TOP_HOOP: m = getRotationalDelta(bsks[i]->innerCenter.x); break;
			case BOTTOM_HOOP: m = getRotationalDelta(bsks[i]->innerCenter.x); break;
			case RIGHT_HOOP: r = getRotationalDelta(bsks[i]->innerCenter.x); break;
			case LEFT_HOOP: l = getRotationalDelta(bsks[i]->innerCenter.x); break;
			}
		}

		lockOn(0, (bsks.size() / dFB) * 1200.0, r, m, l);
	} else {
		lockOn(-10000, -10000, -10000, -10000, -10000);
	}



	for (size_t i = 0; i < bsks.size(); i++) {
		delete(bsks[i]);
	}

	return frame ;
}

int fC = 0;

int main(int argc, char** argv)
{
    bool haveDisplay = getenv("DISPLAY");

	vmin = 60;
	smin = 120;
	hmin = 41;
	vmax = 255;
	smax = 255;
	hmax = 90;

	initLastPos();

	// This stores camera parameters from camera_calibration, used in solvePnPRansac
	cv::FileStorage fs("ps3eye.yml", FileStorage::READ);


	fs["camera_matrix"] >> camera_matrix;
	fs["distortion_coefficients"] >> distortion_matrix;

	cv::VideoCapture videoCapture;

	Mat frame;
	Mat savedFrame;

#if HOST_MODE == 0
	videoCapture.open("/home/charles/drive2.mpg");
#else
	videoCapture.open(0);
#endif

	cout << "Opened" << endl;

	videoCapture.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	videoCapture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

	cout << videoCapture.isOpened() << endl;
	videoCapture>>frame;
	cout << "First frame snagged" << endl;

	if (haveDisplay) {
		namedWindow("Controls", CV_WINDOW_AUTOSIZE);
		createTrackbar( "Hmin", "Controls", &hmin, 255, 0 );
		createTrackbar( "Hmax", "Controls", &hmax, 255, 0 );
		createTrackbar( "Smin", "Controls", &smin, 255, 0 );
		createTrackbar( "Smax", "Controls", &smax, 255, 0 );
		createTrackbar( "Vmin", "Controls", &vmin, 255, 0 );
		createTrackbar( "Vmax", "Controls", &vmax, 255, 0 );
	}
	int fc = 0;

	bool showFrame = haveDisplay;

    while(true) {
    	fc++;
    	videoCapture>>frame;

#if HOST_MODE == 1
    	showFrame = haveDisplay && (fc % 10 == 0);
#endif

    	savedFrame = findRectangles(frame, showFrame, fc);
    	if (showFrame) {
    		imshow("Output", savedFrame);
    	}

    	if (haveDisplay && waitKey(1) > 0)
    		break;

    }

    return 0;
}
