#include <iostream>
#include <stdio.h>
#include <string.h>
#include "car_lib.h"
//#include <sys/time.h>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/gpu/device/utility.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include "exam_cv.h"

using namespace std;
using namespace cv;

extern "C" {

#include "variable.h"
#include "struct.h"


	cv::Point com_x;

	int cnt=0;
	int left_signal_count = 0;
	int right_signal_count = 0;

	int roi_flag = 0;

	struct PT Last_LPT1; // 이전 차선의 왼쪽 좌표 저장
	struct PT Last_LPT2;
	struct PT Last_RPT1; // 이전 차선의 오른쪽 좌표 저장
	struct PT Last_RPT2;

	int lf_cout;
	int slf_cout;

	int rg_cout;
	int srg_cout;


	cv::Point lf_pt1;
	cv::Point lf_pt2;

	cv::Point slf_pt1;
	cv::Point slf_pt2;

	cv::Point rg_pt1;
	cv::Point rg_pt2;

	cv::Point srg_pt1;
	cv::Point srg_pt2;

	cv::Point o_p;

	Scalar yellow_hsv_min = Scalar(15, 60, 65, 0);// 20, 60 70
	Scalar yellow_hsv_max = Scalar(35, 255, 255, 0);

	Scalar red_hsv_min = Scalar(155, 130, 145, 0);
	Scalar red_hsv_max = Scalar(195, 255, 255, 0);

	Scalar green_hsv_min = Scalar(30, 55, 70, 0);
	Scalar green_hsv_max = Scalar(90, 255, 255, 0);

	Scalar white_hsv_min = Scalar(0, 0, 170, 0);            // 175
	Scalar white_hsv_max = Scalar(180, 60, 255, 0);         //55, 255

	Scalar white_bgr_min = Scalar(160,160,160);
	Scalar white_bgr_max = Scalar(255,255,255);


	void ROI(unsigned char* srcBuf, int iw, int ih, unsigned char* outBuf, int nw, int nh)
	{
		Point points[1][4];

		if(stopline_check != 1){
			roi_flag++;

			if(roi_flag >= 2){
				roi_flag = 0;
			}
		}

#if 1	
		points[0][0] = Point(0, ih);
		points[0][1] = Point(0, ih*0.5);
		points[0][2] = Point(iw, ih*0.5);
		points[0][3] = Point(iw, ih);

#endif

		Point Lpoints[1][4];
		Point Rpoints[1][4];

		Lpoints[0][0] = Point(0, ih);
		Lpoints[0][1] = Point(0, ih*0.45);
		Lpoints[0][2] = Point(120, ih*0.45);
		Lpoints[0][3] = Point(120, ih);

		Rpoints[0][0] = Point(iw-120, ih);
		Rpoints[0][1] = Point(iw-120, ih*0.45);
		Rpoints[0][2] = Point(iw, ih*0.45);
		Rpoints[0][3] = Point(iw, ih);


		Mat dstRGB(nh, nw, CV_8UC3, outBuf);
		Mat srcRGB(ih, iw, CV_8UC3, srcBuf);

		Mat ROIimg(ih, iw, CV_8UC3);
		Mat ROIWimg(ih, iw, CV_8UC3);

		ROIimg = Scalar(0);
		ROIWimg = Scalar(0);

		Mat bitwise(ih, iw, CV_8UC3, srcBuf);

		const Point* ppt[1] = {points[0]};	
		const Point* lpt[1] = {Lpoints[0]};
		const Point* rpt[1] = {Rpoints[0]};

		int npt[] = {4};
		int lnpt[] = {4};
		int rnpt[] = {4};

		if(stopline_check == 1){
			fillPoly(ROIWimg, ppt, npt, 1, Scalar(255,255,255));
			bitwise_and(srcRGB, ROIWimg, bitwise);
		}
		else if(roi_flag == 0){
			fillPoly(ROIimg, lpt, lnpt, 1, Scalar(255,255,255));
			bitwise_and(srcRGB, ROIimg, bitwise);
		}
		else if(roi_flag == 1){
			fillPoly(ROIimg, rpt, rnpt, 1, Scalar(255,255,255));
			bitwise_and(srcRGB, ROIimg, bitwise);
		}
		else;


#if 1
		if(stopline_check == 1) // 흰색 정지선  탐색
			Huedetected(srcBuf, iw, ih, outBuf , nw, nh, WHITE_CHECK);

		else if(WhiteLineflag == 1 || WhiteLineflag== 3)//  흰선+노란선 주행
			Huedetected(srcBuf, iw, ih, outBuf, nw, nh, WHITE);
		else  // 노란선주행
			Huedetected(srcBuf, iw, ih, outBuf, nw, nh, YELLOW);

#endif
	}

	void Huedetected(unsigned char* srcBuf, int iw, int ih, unsigned char *outBuf, int nw, int nh, int Color)
	{
		int x,y;

		if( E_tunnel_in_flag){
			Scalar yellow_hsv_min = Scalar(15, 70, 65,0);	// 터널 진입 시 노란선 스칼라값 수정 요망 
			Scalar yellow_hsv_max = Scalar(35, 255, 255, 0);
		}
		else;	

		Mat dstRGB(nh, nw, CV_8UC3, outBuf);
		Mat srcRGB(ih, iw, CV_8UC3, srcBuf);

		Mat hsv(ih, iw, CV_8UC3);

		Mat InR_img(ih, iw, CV_8UC1);
		Mat InR_img_w(ih, iw, CV_8UC1);
		Mat bitwise(ih, iw, CV_8UC1);
		Mat binary_img(ih, iw, CV_8UC1);


		Mat InR_3img(ih, iw, CV_8UC3, srcBuf);
		cvtColor(srcRGB, hsv, COLOR_BGR2HSV);


#if 1
		if(Color == YELLOW){
			inRange(hsv, yellow_hsv_min, yellow_hsv_max, InR_img);
			cvtColor(InR_img, InR_3img, CV_GRAY2BGR);

			OpenCV_hough_transform(srcBuf, iw, ih, outBuf, nw, nh);
		}
		else if(Color == RED){

			Red_val=0;
			inRange(hsv, red_hsv_min, red_hsv_max, InR_img);	
			for( x=0; x<iw; x++){

				for( y=0; y<ih; y++){
					unsigned char *p = InR_img.ptr<unsigned char>(y,x);
					if(*p==255)
						Red_val++;
				}
			}

			printf("Red_val: %d\n", Red_val);

			cv::resize(srcRGB, dstRGB, cv::Size(nw, nh), 0, 0, CV_INTER_LINEAR);

		}
		else if(Color == WHITE){

#if 1
			inRange(hsv, yellow_hsv_min, yellow_hsv_max, InR_img);
			inRange(hsv, white_hsv_min, white_hsv_max, InR_img_w);          
			medianBlur(InR_img_w, InR_img_w, 7);

			bitwise_or(InR_img, InR_img_w,bitwise);
			cvtColor(bitwise, InR_3img, CV_GRAY2BGR);

#endif

			OpenCV_hough_transform(srcBuf, iw, ih, outBuf, nw, nh);

		}
		else if(Color == WHITE_CHECK){
			inRange(hsv, white_hsv_min, white_hsv_max, InR_img_w);
			medianBlur(InR_img_w, InR_img_w, 5);

			cvtColor(InR_img_w, InR_3img, CV_GRAY2BGR);

			White_Line_Check(srcBuf, iw, ih, outBuf, nw, nh);
		}
		else;

#endif





	}

	void OpenCV_hough_transform(unsigned char* srcBuf, int iw, int ih, unsigned char* outBuf, int nw, int nh)
	{
		Scalar lineColor = cv::Scalar(255,255,255);

		Mat dstRGB(nh, nw, CV_8UC3, outBuf);

		Mat srcRGB(ih, iw, CV_8UC3, srcBuf);

		o_p.x = 0;
		o_p.y = 0;

		if(roi_flag == 0){

			lf_pt1 = o_p;
			lf_pt2 = o_p;

			slf_pt1 = o_p;	
			slf_pt2 = o_p;

			lf_cout = 0;
			slf_cout = 0;

		}
		else if(roi_flag == 1){

			rg_pt1 = o_p;
			rg_pt2 = o_p;	

			srg_pt1 = o_p;
			srg_pt2 = o_p;

			rg_cout = 0;
			srg_cout = 0;
		}

		cv::Point to_o;	

		to_o.x = 160; //center location
		to_o.y = 180; 

		cv::Point to2_x;
		cv::Point to_x; // 0 ~ 320

		cv::Mat contours;
		cv::Canny(srcRGB, contours, 3, 100);

		std::vector<cv::Vec2f> lines;	/////HOUGHLINE_NUM
		cv::HoughLines(contours, lines, 1, PI/180,HOUGHLINE_NUM,0,0);  

		cv::Mat result(contours.rows, contours.cols, CV_8UC3, lineColor);

		std::vector<cv::Vec2f>::const_iterator it= lines.begin();

		if(lines.size() != 0){

			while (it!=lines.end()){

				float rho = (*it)[0];
				float theta = (*it)[1]; 

				if (theta < PI/4. || theta > 3.*PI/4.){
					cv::Point spt1(rho/cos(theta), 0); 
					cv::Point spt2((rho-result.rows*sin(theta))/cos(theta), result.rows);
					if(roi_flag == 0){	 

						if(spt1.x >= spt2.x){
							slf_pt1 += spt1;
							slf_pt2 += spt2;

							slf_cout++;
						}
					}
					else if(roi_flag == 1){

						if(spt1.x < spt2.x){	
							srg_pt1 += spt1;
							srg_pt2 += spt2;

							srg_cout++;
						}
					}
				} 
				else{ 
					cv::Point pt1(0,rho/sin(theta));
					cv::Point pt2(result.cols,(rho-result.cols*cos(theta))/sin(theta));
					if(roi_flag == 0){		

						if(pt1.y >= pt2.y){

							if(pt1.y < (pt2.y+40)){

								if(traffic_light_flag == 3){
									slope = (pt1.y - pt2.y)*25;
									if(signal_light == 3){
										signal_light = 5;
										traffic_light_flag = 4;
									}

									else if(signal_light == 4){
										signal_light = 6;
										traffic_light_flag = 4;
									}
								}
							}

							else if(!(signal_light == 5)){
								lf_pt1 += pt1;
								lf_pt2 += pt2;

								lf_cout++;
							}
						}
					}	
					else if(roi_flag == 1){	

						if(pt1.y < pt2.y){ // horizon line except
							if(pt1.y > (pt2.y-40)){

								if(traffic_light_flag == 3){
									slope = (pt1.y - pt2.y)*25;
									if(signal_light == 3){
										signal_light = 5;
										traffic_light_flag = 4;
									}

									else if(signal_light == 4){
										signal_light = 6;
										traffic_light_flag = 4;
									}
								}
							}
							else if(!(signal_light == 6)){

								rg_pt1 += pt1;
								rg_pt2 += pt2;	

								rg_cout++;
							}
						}
					}
				}// line end
				++it;
			}// while end

			if(roi_flag == 0){

				if(slf_cout != 0){

					slf_pt1.y /= slf_cout;
					slf_pt2.y /= slf_cout;

					slf_pt1.x /= slf_cout;
					slf_pt2.x /= slf_cout;
				}
				else
					slf_pt2.x = -50;

				if(lf_cout != 0){
					lf_pt1.y /= lf_cout;
					lf_pt2.y /= lf_cout;

					lf_pt1.x /= lf_cout;
					lf_pt2.x /= lf_cout;
				}
			}

			else if(roi_flag == 1){

				if(srg_cout != 0){

					srg_pt1.y /= srg_cout;
					srg_pt2.y /= srg_cout;

					srg_pt1.x /= srg_cout;
					srg_pt2.x /= srg_cout;
				}
				else
					srg_pt2.x = -50;	

				if(rg_cout != 0){
					rg_pt1.y /= rg_cout;
					rg_pt2.y /= rg_cout;

					rg_pt1.x /= rg_cout;
					rg_pt2.x /= rg_cout;
				}
			}
			cv::line(srcRGB, lf_pt1, lf_pt2, lineColor, 2);	// base line
			cv::line(srcRGB, rg_pt1, rg_pt2, lineColor, 2); // 

			if((rg_cout != 0) || (lf_cout != 0)){

				Linenum = 2; // str state

				if(roi_flag == 0){
					lf_pt1.y *= -1;
					lf_pt2.y *= -1;
				}
				else if(roi_flag == 1){
					rg_pt1.y *= -1;
					rg_pt2.y *= -1;
				}

				to_x.x = ((320)*(rg_pt1.y-lf_pt1.y)/(lf_pt2.y-lf_pt1.y-rg_pt2.y+rg_pt1.y));
				if(lf_cout != 0)
					to_x.y = (lf_pt1.y+((lf_pt2.y-lf_pt1.y)*to_x.x/320))*-1;

				else if(rg_cout != 0)
					to_x.y = (rg_pt1.y+((rg_pt2.y-rg_pt1.y)*to_x.x/320))*-1;

				if((rg_cout == 0) || (lf_cout == 0))
					Linenum = 1;

				if((rg_cout == 0) && (lf_cout == 0))
					Linenum = 0;

				LS1.toX = lf_pt1.x;
				LS1.toY = lf_pt1.y;

				LS2.toX = lf_pt2.x;
				LS2.toY = lf_pt2.y;

				SLS1.toX = slf_pt1.x;
				SLS1.toY = slf_pt1.y;

				SLS2.toX = slf_pt2.x;
				SLS2.toY = slf_pt2.y;


				RS1.toX = rg_pt1.x;
				RS1.toY = rg_pt1.y;

				RS2.toX = rg_pt2.x;
				RS2.toY = rg_pt2.y;

				SRS1.toX = srg_pt1.x;
				SRS1.toY = srg_pt1.y;

				SRS2.toX = srg_pt2.x;
				SRS2.toY = srg_pt2.y;

				topt.toX = to_x.x;
				topt.toY = to_x.y;

				if(roi_flag == 0){
					Last_LPT1.toX = lf_pt1.x;
					Last_LPT1.toY = lf_pt1.y;

					Last_LPT2.toX = lf_pt2.x;
					Last_LPT2.toY = lf_pt2.y;
				}

				else if(roi_flag == 1){
					Last_RPT1.toX = rg_pt1.x;
					Last_RPT1.toY = rg_pt1.y;

					Last_RPT2.toX = rg_pt2.x;
					Last_RPT2.toY = rg_pt2.y;
				}

				cv::line(srcRGB, to_o, to_x, Scalar(0,255,255), 2);
			}
			else{
				Linenum = 0;
				cout << "no Lines detected" << endl;
			}
		}

		cv::resize(srcRGB, dstRGB, cv::Size(nw, nh), 0, 0, CV_INTER_LINEAR);	
	}

	float Curve_Turn_Table(float toX)
	{
		float Turn_const;
		if(WhiteLineflag == 1){
			if( toX < 170 && toX > 150 )	
				Turn_const = 2.7;
			else if( toX < 200 && toX > 120 )		
				Turn_const = 3.1;
			else if( toX < 240 && toX > 100 )
				Turn_const = 3.5;
			else if( toX < 290 && toX > 70 )
				Turn_const = 4.1;
			else if( toX < 350 && toX > 40 )
				Turn_const = 5;
			else if( toX < 360 && toX >  0 )
				Turn_const = 6;
			else if( toX < 375 && toX > - 15 )
				Turn_const = 7;
			else
				Turn_const = 8;
		}	
		else {
			if( toX < 170 && toX > 150 )
				Turn_const = 2.4;
			else if( toX < 200 && toX > 120 )
				Turn_const = 2.7;
			else if( toX < 240 && toX > 100 )
				Turn_const = 3.1;
			else if( toX < 290 && toX > 70 )
				Turn_const = 3.6;
			else if( toX < 350 && toX > 40 )
				Turn_const = 4.2;
			else if( toX < 360 && toX >  0 )
				Turn_const = 5;
			else if( toX < 375 && toX > - 15 )
				Turn_const = 6;
			else
				Turn_const = 8;
		}


		return Turn_const;
	}

	void OpenCV_hough_circles(unsigned char* srcBuf, int iw, int ih, unsigned char* outBuf, int nw, int nh){
		int R_count = 0;
		int Y_count = 0;
		int G_count = 0;

		Mat dstRGB(nh, nw, CV_8UC3, outBuf);
		Mat srcRGB(ih, iw, CV_8UC3, srcBuf);

		Mat gray_image(nh, nw, CV_8UC1);
		Mat hsv(ih, iw, CV_8UC3);

		Mat InR_img_R(ih, iw, CV_8UC1);
		Mat InR_img_Y(ih, iw, CV_8UC1);
		Mat InR_img_G(ih, iw, CV_8UC1);

		cvtColor(srcRGB, hsv, COLOR_BGR2HSV);
		cvtColor(srcRGB, gray_image, COLOR_BGR2GRAY);

		blur(gray_image,gray_image, Size(3,3));

		inRange(hsv, red_hsv_min, red_hsv_max, InR_img_R);
		inRange(hsv, yellow_hsv_min, yellow_hsv_max, InR_img_Y);
		inRange(hsv, green_hsv_min, green_hsv_max, InR_img_G);

		vector<Vec3f> circles;                                                          //30    //40

		HoughCircles(gray_image, circles, CV_HOUGH_GRADIENT, 1, 30 , 200, 40, 0, 0);
		inRange(hsv, red_hsv_min, red_hsv_max, InR_img_R);
		inRange(hsv, yellow_hsv_min, yellow_hsv_max, InR_img_Y);
		inRange(hsv, green_hsv_min, green_hsv_max, InR_img_G);

		for(size_t i = 0; i< circles.size(); i++)
		{
			cir.center_x = cvRound(circles[i][0]);
			cir.center_y = cvRound(circles[i][1]);
			cir.radius = cvRound(circles[i][2]);

			Point center(cvRound(circles[i][0]),cvRound(circles[i][1]));

			circle(srcRGB, center, 3, Scalar(0, 255, 0), 2);
			circle(srcRGB, center, cir.radius, Scalar(0,0,255), 3);

			for(int x = cir.center_x - cir.radius; x <= cir.center_x + cir.radius; x++){

				for(int y = cir.center_y - cir.radius; y <= cir.center_y + cir.radius; y++){

					unsigned char *pr = InR_img_R.ptr<unsigned char>(y,x);
					unsigned char *pg = InR_img_G.ptr<unsigned char>(y,x);

					if(*pr == 255)
						R_count += 1;
					else if(*pg == 255)
						G_count += 1;
				}



			}
		}
		printf("R_COUNT : %d Y_COUNT : %d G_COUNT : %d\n", R_count,Y_count,G_count);

		if(signal_light == 0 && R_count >= 50)
			signal_light = 1;

		else if(signal_light == 1 && G_count >= 50 && G_count < 300){
			left_signal_count++;
			right_signal_count = 0;
		}
		else if(signal_light == 1 && G_count >= 300){
			right_signal_count++;
			left_signal_count = 0;
		}
		if(left_signal_count >= 10){
			signal_light = 3;
			WhiteLineflag = TRAFFIC_START;
		}
		else if(right_signal_count >= 10){
			signal_light = 4;
			WhiteLineflag = TRAFFIC_START;
		}

		cv::resize(srcRGB, dstRGB, cv::Size(nw, nh), 0, 0, CV_INTER_LINEAR);



	}

	void White_Line_Check(unsigned char* srcBuf, int iw, int ih, unsigned char* outBuf, int nw, int nh){

		Mat dstRGB(nh, nw, CV_8UC3, outBuf);
		Mat srcRGB(ih, iw, CV_8UC3, srcBuf);

		Scalar lineColor = cv::Scalar(255,255,255);

		cv::Point lf_pt1;
		cv::Point lf_pt2;

		int lf_cout = 0;

		cv::Point rg_pt1;
		cv::Point rg_pt2;

		int rg_cout = 0;

		cv::Mat contours;
		cv::Canny(srcRGB, contours, 3, 100);

		std::vector<cv::Vec2f> lines;
		cv::HoughLines(contours, lines, 1,PI/180,90,0,0); 
		cv::Mat result(contours.rows, contours.cols, CV_8UC3, lineColor);

		std::vector<cv::Vec2f>::const_iterator it= lines.begin();

		if(lines.size() != 0){

			while(it!=lines.end()){

				float rho = (*it)[0];   
				float theta = (*it)[1]; 

				if (theta < PI/4. || theta > 3.*PI/4.){
				}
				else{
					cv::Point pt1(0,rho/sin(theta)); 
					cv::Point pt2(result.cols,(rho-result.cols*cos(theta))/sin(theta));

					if(pt1.y >= pt2.y){
						if(pt1.y < (pt2.y+10)){
							Whitecheck_flag = 1;
						}               		
					}
					else if(pt1.y < pt2.y){ // horizon line except

						if(pt1.y > (pt2.y-10)){
							Whitecheck_flag = 1;
						}
					}

				}// line end
				++it;

			}// while end

		}
		cv::resize(srcRGB, dstRGB, cv::Size(nw, nh), 0, 0, CV_INTER_LINEAR);
	}



} // end extern "C"
