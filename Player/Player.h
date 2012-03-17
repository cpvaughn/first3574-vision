#ifndef __PLAYER_H
#define __PLAYER_H

#include <opencv2/core/core.hpp>

class Player
{
public:
	Player(cv::Mat src, cv::Scalar p1Min, cv::Scalar p1Max);
	~Player();
	cv::Point Periodic();
	cv::Point PlayerMain();
	cv::Mat Draw(int position, std::string all);
	cv::Mat draw;
	cv::Mat player;
	cv::Point playerLastCenter;
	cv::Mat thresh;
private:
	//General
	cv::Mat _src1;
	cv::Mat _srcHsv1;
	cv::Scalar _color;

	//player
	cv::Scalar _p1Min;
	cv::Scalar _p1Max;
	cv::Point2f _playerRectPoints[4];
	cv::vector<cv::vector<cv::Point> > playerContours;
	cv::vector<cv::Vec4i> playerHierarchy;
	float playerRange;
};

#endif
