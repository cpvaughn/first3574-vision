#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <unistd.h>

#include <iostream>
#include "Socket/ClientSocket.h"

using namespace cv;
using namespace std;

int vmin, vmax, smin, smax, hmin, hmax;


void sendMessage(string msg) {
    ClientSocket *client_socket = new ClientSocket( "10.35.74.2", 5001 );
	*client_socket << msg;
	delete(client_socket);
}

Mat findRectangles(Mat frame) {
	Mat hsv;
	Mat thresh;

	cvtColor(frame, hsv, CV_BGR2HSV);

	inRange(hsv, Scalar(hmin, smin, vmin), Scalar(hmax, smax, vmax), thresh);

	Mat savedThresh = thresh.clone();

	//Track Square
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	findContours(thresh, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

	vector<Point> approx;
	vector<vector<Point> > squares;


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
		if( approx.size() == 4 &&
				(hierarchy[i][2] >= 0 ||
				hierarchy[i][3] >= 0) &&
				fabs(contourArea(Mat(approx))) > 1000 &&
				isContourConvex(Mat(approx)) )
		{
			//double maxCosine = 0;

			/*for( int j = 2; j < 5; j++ )
			{
				// find the maximum cosine of the angle between joint edges
				double cosine = fabs(angle(approx[j%4], approx[j-2], approx[j-1]));
				maxCosine = MAX(maxCosine, cosine);
			}*/

			// if cosines of all angles are small
			// (all angles are ~90 degree) then write quandrange
			// vertices to resultant sequence
			//if( maxCosine < 0.3 )
				squares.push_back(approx);
		}
	}

	//cout << "Squares:: " << squares.size() << endl;

	//draw loop
	for( size_t i = 0; i < squares.size(); i++ )
	{
		const Point* p = &squares[i][0];
		int n = (int)squares[i].size();

		//draw rectangles
		polylines(frame, &p, &n, 1, true, Scalar(0,0,255), 2, CV_AA);

		//log points of rectangles
		//cout << Mat(squares[1])<<endl;

		vector <Point> currentRectangle = squares[i];

		RotatedRect minRect;

		minRect = minAreaRect( Mat(contours[i]) );


		// Check to make sure this rectangle has another inside it
        if (hierarchy[i][2] >= 0) {
        	//cout <<minRect.size.width << "x" << minRect.size.height << " " << minRect.angle << endl;
        	double mWidth = max(minRect.size.width, minRect.size.height);
        	double pixScale = 640.0 / mWidth;
        	double fovScale = 2.0 * 0.814; // tan(75deg / 2)
        	cout << pixScale * fovScale * 28.6 << "in away-ish" << endl;
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

	sendMessage("500");

	      usleep(3000 * 1000);
    sendMessage("0");


	      ClientSocket client_socket2 ( "10.35.74.2", 5001 );
		  client_socket2 << "0";


	vmin = 142;
	smin = 160;
	hmin = 64;
	vmax = 255;
	smax = 255;
	hmax = 98;
	vmin = 130;
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
	createTrackbar( "Hmin", "Output", &hmin, 255, 0 );
	createTrackbar( "Hmax", "Output", &hmax, 255, 0 );
	createTrackbar( "Smin", "Output", &smin, 255, 0 );
	createTrackbar( "Smax", "Output", &smax, 255, 0 );
	createTrackbar( "Vmin", "Output", &vmin, 255, 0 );
	createTrackbar( "Vmax", "Output", &vmax, 255, 0 );

    while(true) {
    	videoCapture>>frame;

    	cout << "Captured" << endl;
    	imshow("Output", findRectangles(frame));
    	if (waitKey(10) > 0)
    		break;

    }


    waitKey();

    return 0;
}
