#include "Player.h"
#include <opencv2/core/core.hpp>
#include "opencv2/imgproc/imgproc.hpp"

Player::Player(cv::Mat src, cv::Scalar p1Min, cv::Scalar p1Max){
	_src1 = src;
	draw = src;
	_p1Min = p1Min;
	_p1Max = p1Max;
	_color = cv::Scalar( 0,0,255 );
	playerLastCenter = cv::Point(0, 240);
}

Player::~Player(){
}


cv::Point Player::Periodic(){
	//Convert _color to HSV
	cv::cvtColor(_src1,_srcHsv1,CV_RGB2HSV);

	//Thresh for player
	cv::inRange(_srcHsv1,_p1Min,_p1Max,player);
	cv::morphologyEx(player,player, cv::MORPH_CLOSE,cv::Mat(30,10,CV_8U,cv::Scalar(30)));

	// Find playerContours
	cv::findContours( player, playerContours, playerHierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );

	// Find the rotated rectangles
	cv::vector<cv::RotatedRect> minRect( playerContours.size() );

	for( uint i = 0; i < playerContours.size(); i++ )
	{
		minRect[i] = cv::minAreaRect( cv::Mat(playerContours[i]) );

	}

	for( uint i = 0; i < playerContours.size(); i++ ){

		// rotated rectangle
		minRect[0].points( _playerRectPoints );
		playerRange = abs(_playerRectPoints[0].x - _playerRectPoints[2].x );

		if(playerRange > 20){

			playerLastCenter = cv::Point((_playerRectPoints[0].x + _playerRectPoints[1].x + _playerRectPoints[2].x + _playerRectPoints[3].x)/4 ,(_playerRectPoints[0].y + _playerRectPoints[1].y + _playerRectPoints[2].y + _playerRectPoints[3].y)/4);
		}
	}

	return playerLastCenter;
}


cv::Mat Player::Draw(int positionX, std::string all)
{
	if(all == "all" || all == "All"){
		if(playerRange > 20){
			for( int j = 0; j < 4; j++ ){
				cv::line( draw, _playerRectPoints[j], _playerRectPoints[(j+1)%4], _color, 2, 8 );
			}
		}
	}
	cv::line(draw, cv::Point(positionX,playerLastCenter.y - 30), cv::Point(positionX,playerLastCenter.y + 30), _color,16,8);
	return draw;
}

