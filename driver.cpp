#include <opencv2/core/core.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include "time.h"

#include "Player/Player.h"
#include "Ball/Ball.h"

using namespace cv;
using namespace std;

int vmin, vmax, smin, smax, hmin, hmax, vmin2, vmax2, smin2, smax2, hmin2, hmax2;


// The RGB values are stored in reverse order (i don't know why)
struct RGB { unsigned char b, g, r; };

// Assumes 1 byte for r,g,b
RGB& GetRGB(cv::Mat &mat, cv::Point p)
{
  assert((mat.step/mat.cols) == sizeof(RGB));
  RGB *data = (RGB*)mat.data;
  data += p.y * mat.cols + p.x;
  return *data;
}

Mat test(Mat frame, Scalar min, Scalar max){
	cvtColor(frame,frame,CV_RGB2HSV);
	inRange(frame,min,max,frame);
	return frame;
}


int main(int argc, char** argv)
{
	//Player1 hsv values
	hmin = 120;		hmax = 144;
	smin = 230;		smax = 255;
	vmin = 118;		vmax = 255;

	//Player2 hsv values
	hmin2 = 90;		hmax2 = 126;
	smin2 = 82;		smax2 = 255;
	vmin2 = 136;	vmax2 = 156;

	//Player1 thresh
	Scalar player1Min = Scalar(hmin,smin,vmin,0);
	Scalar player1Max = Scalar(hmax,smax,vmax,0);

	//Player2 thresh
	Scalar player2Min = Scalar(hmin2,smin2,vmin2,0);
	Scalar player2Max = Scalar(hmax2,smax2,vmax2,0);

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


	createTrackbar( "Hmin2", "Sliders", &hmin2, 255, 0 );
	createTrackbar( "Hmax2", "Sliders", &hmax2, 255, 0 );
	createTrackbar( "Smin2", "Sliders", &smin2, 255, 0 );
	createTrackbar( "Smax2", "Sliders", &smax2, 255, 0 );
	createTrackbar( "Vmin2", "Sliders", &vmin2, 255, 0 );
	createTrackbar( "Vmax2", "Sliders", &vmax2, 255, 0 );

	Player *player1 = new Player(frame,player1Min,player1Max);
	Player *player2 = new Player(frame, player2Min, player2Max);

	Ball *ball = new Ball(frame);

	while(true) {

		videoCapture>>frame;

		player1->Periodic();
		player2->Periodic();

		ball->MoveBall(player1->playerLastCenter, player2->playerLastCenter);

		frame = player1->Draw(0,"all");
		frame = player2->Draw(640, "all");

		frame = ball->Draw();


		//fortesting
//		Scalar testMin = Scalar(hmin2,smin2,vmin2,0);
//		Scalar testMax = Scalar(hmax2,smax2,vmax2,0);
//		imshow("Output",test(frame,testMin,testMax));

		imshow("Output", frame);
		if (waitKey(10) > 0)
			break;

	}


	waitKey();

	return 0;
}
