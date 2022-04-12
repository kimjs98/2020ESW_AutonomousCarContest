#ifndef EXAM_CV_H_
#define EXAM_CV_H_ 

#ifdef __cplusplus
extern "C" {
#endif


	void OpenCV_load_file(char* file, unsigned char* outBuf, int nw, int nh);
	void OpenCV_Bgr2RgbConvert(unsigned char* inBuf, int w, int h, unsigned char* outBuf);
	void OpenCV_hough_transform(unsigned char* srcBuf, int iw, int ih, unsigned char* outBuf, int nw, int nh);
	void OpenCV_hough_circles(unsigned char* srcBuf, int iw, int ih, unsigned char* outBuf, int nw, int nh);
	void ROI(unsigned char* srcBuf, int iw, int ih, unsigned char* outBuf, int nw, int nh);
	void Huedetected(unsigned char* srcBuf, int iw, int ih, unsigned char *outBuf, int nw, int nh, int Color);
	void Detected_Obstacle(unsigned char* srcBuf, int iw, int ih, unsigned char* outBuf, int nw, int nh);
	void White_Line_Check(unsigned char* srcBuf , int iw, int ih , unsigned char* outBuf , int nw, int nh);
	float Curve_Turn_Table(float toX);

#ifdef __cplusplus
}
#endif

#endif //EXAM_CV_H_

