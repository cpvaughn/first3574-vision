#ifndef __BALL_H
#define __BALL_H

#include <opencv2/core/core.hpp>

class Ball
{
public:
	Ball(cv::Mat src);
	~Ball();
	cv::Point MoveBall(cv::Point player1, cv::Point player2);
	cv::Mat Draw();
	void SetSpeed(uint speed);
	void Reset();
	cv::Mat _src;
	cv::Point ball;
private:
	int dir;
	cv::Point ballTemp;
	uint speed;
	cv::Point AI;
	int hits;
};

#endif
