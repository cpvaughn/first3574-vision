#include "Ball.h"
#include <opencv2/core/core.hpp>
#include "time.h"

Ball::Ball(cv::Mat src)
{
	_src = src;

	//1 = leftUp
	dir = rand() % 4 + 1;

	speed = 6;
	AI = cv::Point(31,255);
	hits = 0;

	ball = cv::Point(320,240);
	ballTemp = cv::Point(320,240);
}


Ball::~Ball()
{

}

cv::Point Ball::MoveBall(cv::Point player1, cv::Point player2){

	ballTemp = ball;

	if (( dir == 1 ) && ( ball.x >= 0 )&& ( ball.y >= 5 ))
	{
		if (( ball.x <= 6 ) && ( ball.y >= player1.y - 30 ) && ( ball.y <= player1.y + 30 ))
		{
			dir = rand() % 2 + 3;

			hits++;
		}
		else
		{
			ball.x -= speed;
			ball.y -= speed;
		}
	}
	else
		if (( dir == 2 ) && ( ball.x >= 0 ) && ( ball.y <= 640 ))
		{
			if (( ball.x <= 6 ) && ( ball.y >= player1.y -30 ) && ( ball.y <= player1.y + 30 ))
			{
				dir = rand()% 2 + 3;

				hits++;
			}
			else
			{
				ball.x -= speed;
				ball.y += speed;
			}
		}
		else
			if (( dir == 3 ) && ( ball.x <= 640 ) && ( ball.y >= 0 ))
			{
				if(( ball.x >= 634 ) && ( ball.y >= player2.y -30 )&& ( ball.y <= player2.y + 30 ))
				{
					dir = rand()% 2 + 1;

					hits++;
				}
				else
				{
					ball.x += speed;
					ball.y -= speed;
				}
			}
			else
				if ((dir == 4 ) && ( ball.x <= 635 ) && ( ball.y <= 475 ))
				{
					if(( ball.x >= 634 ) && ( ball.y >= player2.y -30 ) && ( ball.y <= player2.y + 30 ))
					{
						dir = rand()% 2 + 1;

						hits++;
					}
					else
					{
						ball.x += speed;
						ball.y += speed;
					}
				}
				else
				{
					if (( dir == 1 ) || ( dir == 3 ))
					{
						++dir;
					}
					else
						if (( dir == 2 ) || ( dir == 4 ))
						{
							--dir;
						}
				}

	return ball;
}

cv::Mat Ball::Draw()
{
	//	drawCircle
	cv::circle(_src,ball,5,cv::Scalar(255,255,255),10);

	return _src;
}



