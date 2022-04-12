#define _MAIN_
#define _STRUCT_

#include <signal.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <termios.h>
#include <sys/time.h>
#include <errno.h>
#include <syslog.h>

#include "util.h"
#include "display-kms.h"
#include "v4l2.h"
#include "vpe-common.h"
#include "drawing.h"
#include "input_cmd.h"
#include "exam_cv.h"
#include "car_lib.h"

#include "variable.h"
#include "struct.h"

#define CAPTURE_IMG_W       1280
#define CAPTURE_IMG_H       720
#define CAPTURE_IMG_SIZE    (CAPTURE_IMG_W*CAPTURE_IMG_H*2) // YUYU : 16bpp
#define CAPTURE_IMG_FORMAT  "uyvy"

#define VPE_OUTPUT_W        320
#define VPE_OUTPUT_H        180

// display output & dump  format: NV12, w:320, h:180
//#define VPE_OUTPUT_IMG_SIZE    (VPE_OUTPUT_W*VPE_OUTPUT_H*3/2) // NV12 : 12bpp
//#define VPE_OUTPUT_FORMAT       "nv12"

// display output & dump  format: yuyv, w:320, h:180
//#define VPE_OUTPUT_IMG_SIZE    (VPE_OUTPUT_W*VPE_OUTPUT_H*2)
//#define VPE_OUTPUT_FORMAT       "yuyv"

#define VPE_OUTPUT_IMG_SIZE    (VPE_OUTPUT_W*VPE_OUTPUT_H*3)
#define VPE_OUTPUT_FORMAT       "bgr24"

#define OVERLAY_DISP_FORCC      FOURCC('A','R','2','4')
#define OVERLAY_DISP_W          480
#define OVERLAY_DISP_H          272

#define TIME_TEXT_X             385 //320
#define TIME_TEXT_Y             260 //240
#define TIME_TEXT_COLOR         0xffffffff //while



unsigned char status;
short speed;
unsigned char gain;
int position, posInit, posDes, posRead;
short angle;
int channel;
int data;
unsigned char Whitesensor;


struct thr_data {
	struct display *disp;
	struct v4l2 *v4l2;
	struct vpe *vpe;
	struct buffer **input_bufs;


	int msgq_id;
	bool bfull_screen;
	bool bstream_start;
	pthread_t threads[10];
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_s = PTHREAD_COND_INITIALIZER;

pthread_cond_t cond_P_v = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_P_h = PTHREAD_COND_INITIALIZER;

pthread_cond_t cond_Obs = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_Rtt = PTHREAD_COND_INITIALIZER; 
pthread_cond_t cond_T = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_TF = PTHREAD_COND_INITIALIZER;

static int allocate_input_buffers(struct thr_data *data){

	int i;
	struct vpe *vpe = data->vpe;

	data->input_bufs = calloc(NUMBUF, sizeof(*data->input_bufs));
	for(i = 0; i < NUMBUF; i++) {
		data->input_bufs[i] = alloc_buffer(vpe->disp, vpe->src.fourcc, vpe->src.width, vpe->src.height, false);
	}
	if (!data->input_bufs)
		ERROR("allocating shared buffer failed\n");

	for (i = 0; i < NUMBUF; i++) {

		vpe->input_buf_dmafd[i] = omap_bo_dmabuf(data->input_bufs[i]->bo[0]);
		data->input_bufs[i]->fd[0] = vpe->input_buf_dmafd[i];
	}
	return 0;
}

static void free_input_buffers(struct buffer **buffer, uint32_t n, bool bmultiplanar){
	uint32_t i;
	for (i = 0; i < n; i++) {
		if (buffer[i]) {
			close(buffer[i]->fd[0]);
			omap_bo_del(buffer[i]->bo[0]);
			if(bmultiplanar){
				close(buffer[i]->fd[1]);
				omap_bo_del(buffer[i]->bo[1]);
			}
		}
	}
	free(buffer);
}

static void draw_operatingtime(struct display *disp, uint32_t time)
{
	FrameBuffer tmpFrame;
	unsigned char* pbuf[4];
	char strtime[128];

	memset(strtime, 0, sizeof(strtime));

	sprintf(strtime, "%03d(ms)", time);

	if(get_framebuf(disp->overlay_p_bo, pbuf) == 0) {
		tmpFrame.buf = pbuf[0];
		tmpFrame.format = draw_get_pixel_foramt(disp->overlay_p_bo->fourcc);//FORMAT_RGB888; //alloc_overlay_plane() -- FOURCC('R','G','2','4');
		tmpFrame.stride = disp->overlay_p_bo->pitches[0];//tmpFrame.width*3;

		drawString(&tmpFrame, strtime, TIME_TEXT_X, TIME_TEXT_Y, 0, TIME_TEXT_COLOR);
	}
}

static Detected_Ob(struct display *disp, struct buffer *cambuf)
{
	unsigned char srcbuf[VPE_OUTPUT_W*VPE_OUTPUT_H*3];
	uint32_t optime;
	struct timeval st, et;
	unsigned char* cam_pbuf[4];

	if(get_framebuf(cambuf, cam_pbuf) == 0) {
		memcpy(srcbuf, cam_pbuf[0], VPE_OUTPUT_W*VPE_OUTPUT_H*3);

		gettimeofday(&st, NULL);

		Huedetected(srcbuf, VPE_OUTPUT_W, VPE_OUTPUT_H, cam_pbuf[0], VPE_OUTPUT_W, VPE_OUTPUT_H, 1);
		gettimeofday(&et, NULL);
		optime = ((et.tv_sec - st.tv_sec)*1000)+ ((int)et.tv_usec/1000 - (int)st.tv_usec/1000);
		draw_operatingtime(disp, optime);
	}
}
static White_Check(struct display *disp, struct buffer *cambuf){

	unsigned char srcbuf[VPE_OUTPUT_W*VPE_OUTPUT_H*3];
	unsigned char* cam_pbuf[4];

	if(get_framebuf(cambuf, cam_pbuf) == 0) {
		memcpy(srcbuf , cam_pbuf[0] , VPE_OUTPUT_W*VPE_OUTPUT_H*3);
		ROI(srcbuf, VPE_OUTPUT_W, VPE_OUTPUT_H, cam_pbuf[0], VPE_OUTPUT_W, VPE_OUTPUT_H);

	}


}
static void Hough_Circles(struct display *disp, struct buffer *cambuf)
{
	unsigned char srcbuf[VPE_OUTPUT_W*VPE_OUTPUT_H*3];
	uint32_t optime;
	struct timeval st, et;
	unsigned char* cam_pbuf[4];

	if(get_framebuf(cambuf, cam_pbuf) == 0) {
		memcpy(srcbuf, cam_pbuf[0], VPE_OUTPUT_W*VPE_OUTPUT_H*3);

		gettimeofday(&st, NULL);

		OpenCV_hough_circles(srcbuf, VPE_OUTPUT_W, VPE_OUTPUT_H, cam_pbuf[0], VPE_OUTPUT_W, VPE_OUTPUT_H);

		gettimeofday(&et, NULL);
		optime = ((et.tv_sec - st.tv_sec)*1000)+ ((int)et.tv_usec/1000 - (int)st.tv_usec/1000);
		draw_operatingtime(disp, optime);
	}
}


static void Run(struct display *disp, struct buffer *cambuf)
{
	unsigned char srcbuf[VPE_OUTPUT_W*VPE_OUTPUT_H*3];
	uint32_t optime;
	struct timeval st, et;
	unsigned char* cam_pbuf[4];

	float serve_num=0;
	int toX,toY = 0; // loss point

	double rect; // toX/toY

	float curve_x = 0; // curve line's ROI_Y point      
	float curve_const = 0;
	int curve_flag = 0; // curve line's steer
	int curve_flag2 = 0;

	int LS_1_x, LS_1_y; // // base lines : straight line's steer 1, curve steer
	int LS_2_x, LS_2_y; //

	int RS_1_x, RS_1_y; //
	int RS_2_x, RS_2_y; // //

	int SLS_1_x, SLS_1_y; // X
	int SLS_2_x, SLS_2_y; // SLS_2_X using straight line's steer 2

	int SRS_1_x, SRS_1_y; // X
	int SRS_2_x, SRS_2_y; // SRS_2_X using straight line's steer 2

	int Linestatus = 0; // 1 = straight line

	if(get_framebuf(cambuf, cam_pbuf) == 0) {
		memcpy(srcbuf, cam_pbuf[0], VPE_OUTPUT_W*VPE_OUTPUT_H*3);

		gettimeofday(&st, NULL);

		ROI(srcbuf, VPE_OUTPUT_W, VPE_OUTPUT_H, cam_pbuf[0], VPE_OUTPUT_W, VPE_OUTPUT_H);


		toX = topt.toX;
		toY = topt.toY;

		if(steercount %1 ==0){
			steercount = 0;


			if(Linenum == 1)
				toY = 1;
			else if(Linenum == 0)
				toX = L_toX;
			else;

			angle = 1500;


			LS_1_x = LS1.toX;
			LS_1_y = LS1.toY;

			LS_2_x = LS2.toX;
			LS_2_y = LS2.toY;

			RS_1_x = RS1.toX;
			RS_1_y = RS1.toY;

			RS_2_x = RS2.toX;
			RS_2_y = RS2.toY;

			SLS_1_x = SLS1.toX;
			SLS_1_y = SLS1.toY;

			SLS_2_x = SLS2.toX;
			SLS_2_y = SLS2.toY;

			SRS_1_x = SRS1.toX;
			SRS_1_y = SRS1.toY;

			SRS_2_x = SRS2.toX;
			SRS_2_y = SRS2.toY;

			if(toY != 0)
				rect = (toX/toY);



			if(rect >= 90 && rect <= 230){
				Linestatus = 1;
			}

			if((toX < 40 || toX > 280)){	//0

				movestatus =2;	// curve status
			}
			// range check!
			else if(toX >125 && toX < 195){
				turn_status = STRAIGHT;
				movestatus =1; // straight      
			}
			else
				movestatus = 0;
			if(movestatus == 1 && Linenum == 2){
				serve_num = toX-160;
				serve_num*=1.8;
				CameraYServoControl_Write(STRAIGHT_Y);
			}
			else if(Linenum == 0){  // no Line detected
				serve_num = toX-160;
				serve_num*= 1.8;
			}
			if(Linenum == 1){	

				if(SLS_2_x > -10){              //straight Line steer 2
					angle -= (SLS_2_x+10)*1.2;
				}
				else if(SRS_2_x < 370 && SRS_2_x > 180){
					angle -= (SRS_2_x-370)*1.2;
				}

				if(LS_2_x == 320){ //straight Line steer 1
					angle += (LS_1_y+96)*3.5;
				}

				if(RS_2_x == 320){//35
					angle -= (RS_2_y+96)*3.5;
				}
			}

			angle -= (int)serve_num;

			if(movestatus == 2){

				CameraYServoControl_Write(CURVE_Y);
				// start, end point check!

				if(LS_2_x == 320){

					curve_x = 320*(-63-LS_1_y)/(LS_2_y-LS_1_y);
					if(curve_x > 245){
						angle = 1500;
						curve_const = Curve_Turn_Table(curve_x);
						curve_flag = (int) (curve_x - 160)*curve_const;
					}
				}

				if(RS_2_x == 320){

					curve_x = 320*(-63-RS_1_y)/(RS_2_y-RS_1_y);
					if(curve_x <75){
						angle = 1500;
						curve_const = Curve_Turn_Table(curve_x);
						curve_flag2 = (int) (curve_x -160)*curve_const;
					}
				}

			}


			if( LS_2_x == 320 && RS_2_x == 320 )
				angle  -= (curve_flag+curve_flag2)/2;

			else if( LS_2_x == 320)
				angle -= curve_flag;

			else if (RS_2_x == 320)
				angle -= curve_flag2;

			if(angle>2000)
				angle=2000;

			if(angle<1000)
				angle=1000;



			if( WhiteLineflag == 1 && rotate_flag >=3 ){  // 회전교차로 탈출 조건문 
				if(rect>140 && rect<=180){

					WhiteLineflag = 2;
					Whitecheck_flag = 0;

					g_flag.rotate_road_flag = FLAG_OFF;

					mode_count++;

					g_flag.rotate_after_flag = FLAG_ON;
				}
			}	


			if(g_flag.rotate_road_flag){


				CameraYServoControl_Write(CURVE_Y);

				if(rotate_flag==2){
					if(rotate_count <= 10){
						angle = 1500;
						rotate_count++;
					} 
					speed=70;
				}
				else if(rotate_flag ==4){
					speed=110;
				}
				else
					speed=0;
			}
			else if(Whitecheck_flag == 1){
				speed = 60;
			}
			else{
				speed= STRAIGHT_SPEED;
				if(toX > 320 || toX < 0){
					speed = CURVE_SPEED;
				}
			}

			if(threefold_v.order==FINISH){
				speed=100;
			}	
			else if(g_flag.threefold_after_flag)
				speed = 60;
			else;

			if(traffic_light_flag == 3)
				speed = 95;	

			if(mode_count == 2)
				speed = 75;
			if((!g_flag.traffic_light_flag) || (traffic_light_flag == 3)){
				SteeringServoControl_Write(angle);
				DesireSpeed_Write(speed);
			}


			L_toX = (160 + (toX - 160)*0.95);

			gettimeofday(&et, NULL);
			optime = ((et.tv_sec - st.tv_sec)*1000)+ ((int)et.tv_usec/1000 - (int)st.tv_usec/1000);
			draw_operatingtime(disp, optime);

		}
		steercount++;
	}

}

void * capture_thread(void *arg)
{
	struct thr_data *data = (struct thr_data *)arg;
	struct v4l2 *v4l2 = data->v4l2;
	struct vpe *vpe = data->vpe;
	struct buffer *capt;
	bool isFirst = true;
	int index;
	int count = 0;
	int i;


	v4l2_reqbufs(v4l2, NUMBUF);

	// init vpe input
	vpe_input_init(vpe);

	// allocate vpe input buffer
	allocate_input_buffers(data);

	if(vpe->dst.coplanar)
		vpe->disp->multiplanar = true;
	else
		vpe->disp->multiplanar = false;
	printf("disp multiplanar:%d \n", vpe->disp->multiplanar);

	// init /allocate vpe output
	vpe_output_init(vpe);
	vpe_output_fullscreen(vpe, data->bfull_screen);

	for (i = 0; i < NUMBUF; i++)
		v4l2_qbuf(v4l2,vpe->input_buf_dmafd[i], i);

	for (i = 0; i < NUMBUF; i++)
		vpe_output_qbuf(vpe, i);

	v4l2_streamon(v4l2);
	vpe_stream_on(vpe->fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

	vpe->field = V4L2_FIELD_ANY;


	while(1) {


		index = v4l2_dqbuf(v4l2, &vpe->field);
		vpe_input_qbuf(vpe, index);

		if (isFirst) {
			vpe_stream_on(vpe->fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
			isFirst = false;
			MSG("streaming started...");
			data->bstream_start = true;
		}



		index = vpe_output_dqbuf(vpe);
		capt = vpe->disp_bufs[index];

		pthread_mutex_lock(&mutex);

#if 1
		if(g_flag.obstacle_detect_flag){
			Detected_Ob(vpe->disp,capt);
			if(Red_val>RED_LIMIT){
				g_flag.obstacle_flag= FLAG_ON;

				pthread_cond_signal(&cond_Obs); 


			}
			else {
				sleep(2);
				g_flag.obstacle_detect_flag= FLAG_OFF;
				g_flag.obstacle_after_flag= FLAG_ON;
			}
		}
		else if(traffic_light_flag == 1 && signal_light <= 2)
			Hough_Circles(vpe->disp,capt);
		else {

			if(count==3 && !g_flag.obstacle_after_flag){
				Detected_Ob(vpe->disp,capt);
				if(Red_val>RED_FIRST_LIMIT)
					g_flag.obstacle_detect_flag= FLAG_ON;
			}
			else if(count==6 || count == 9){

				if((mode_count == 4) || (mode_count == 7)){ // 회전교차로 진입, 신호등 진입
					stopline_check = 1;
					White_Check(vpe->disp,capt);
					stopline_check = 0;
				}
				pthread_cond_signal(&cond_s); // sensor thread on
			}	
			else {
				if(mode_count > 1)
					Run(vpe->disp,capt);
				if(count>=10)
					count=0;
			}	
		}
#endif

		count++;



		pthread_mutex_unlock(&mutex);
		if (disp_post_vid_buffer(vpe->disp, capt, 0, 0, vpe->dst.width, vpe->dst.height)) {
			ERROR("Post buffer failed");
			return NULL;
		}
		update_overlay_disp(vpe->disp); 



		vpe_output_qbuf(vpe, index);
		index = vpe_input_dqbuf(vpe);
		v4l2_qbuf(v4l2, vpe->input_buf_dmafd[index], index);

	}


	MSG("Ok!");
	return NULL;
}
void * sensor_thread(void *arg)
{
	while(1){
		MSG("                           SENSOR THR\n");  

		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond_s,&mutex);

		MSG( "MODE : %d \n", mode_count);

		switch(mode_count){
			case 1 : // start and run check
				if(race_start())
					mode_count++;
				break;
			case 2 : // overpass and downhill check
				if (overpass_downhill())
					mode_count++;
				break;
			case 3 : // parking area check

				if(g_flag.parking_after_flag)
					mode_count++;

#if 1
				if (DistanceSensor(2) > WALL_LIMIT && P_checkflag == 0){
					P_checkflag = 1;
				}
				if(DistanceSensor(3) > WALL_LIMIT && P_checkflag == 1)
					P_checkflag = 2;
				if(DistanceSensor(2) < 100 && P_checkflag == 2){
					P_checkflag = 3;
					posInit = 0;  //initialize
					EncoderCounter_Write(posInit);
				}
				posRead=EncoderCounter_Read();
				if (DistanceSensor(2) > WALL_LIMIT && P_checkflag == 3){
					if(posRead<600){
						P_checkflag=4;
						parking_type=0;
						posInit = 0;  //initialize
						EncoderCounter_Write(posInit);
					}
					else if(posRead>800){
						P_checkflag=4;
						parking_type=1;
						posInit = 0;  //initialize
						EncoderCounter_Write(posInit);
					}
					else;

				}

				if(DistanceSensor(3) > WALL_LIMIT && P_checkflag == 4){
					if(parking_type==0)
						pthread_cond_signal(&cond_P_v);
					else if(parking_type == 1)
						pthread_cond_signal(&cond_P_h);
					else;

				}

#endif
				break;
			case 4 : // rotate check
#if 1

				if(!g_flag.rotate_road_flag){
					Whitesensor = LineSensor_Read();
					if(Whitesensor == 0)
						WhiteLineflag = 1;
				}

				if(WhiteLineflag == 1){
					g_flag.rotate_road_flag = FLAG_ON;
					pthread_cond_signal(&cond_Rtt);
				}

#endif

				break;
			case 5 : // tunnel check

#if 1

				if(DistanceSensor(2)>1000 && DistanceSensor(6)>1000){
					tunnel_in();
					g_flag.tunnel_flag= FLAG_ON;

				}
				else if(DistanceSensor(2)<500 && DistanceSensor(6)<500 && g_flag.tunnel_flag){
					tunnel_out();
					WhiteLineflag = 3;
					mode_count++;
				}
				else;

#endif
				break;
			case 6 : // threefold check

				if (DistanceSensor(1) > 650&&threefold_v.order==0){                
					threefold_v.order= START;
					DesireSpeed_Write(0);
					usleep(200000);

					SteeringServoControl_Write(1500);
					usleep(500000);
					DesireSpeed_Write(-100);
					EncoderCounter_Write(0);      //엔코더 초기화
					while(1){
						posRead=EncoderCounter_Read();
						printf("posRead main : %d\n",posRead);
						printf("distance sensordkf:%d,%d\n",DistanceSensor(6),DistanceSensor(2));
						if(DistanceSensor(2)>700&&threefold_v.order==START)
							threefold_v.type=RIGHT_F;
						if(DistanceSensor(6)>700&&threefold_v.order==START)
							threefold_v.type=LEFT_F;

						if(posRead<-380)
							break;
					}

					DesireSpeed_Write(0);
					usleep(300000);

					pthread_cond_signal(&cond_TF); //send signal
				}

				if(DistanceSensor(6)<750&&DistanceSensor(5)<750&&threefold_v.order==2&&threefold_v.obstacle==LEFT){
					DesireSpeed_Write(0);
					usleep(200000);

					SteeringServoControl_Write(1500);
					usleep(300000);
					DesireSpeed_Write(-100);
					EncoderCounter_Write(0);      //엔코더 초기화

					while(1){
						posRead=EncoderCounter_Read();
						printf("posRead main : %d\n",posRead);
						if(posRead<-500)
							break;
					}
					DesireSpeed_Write(0);
					usleep(300000);

					pthread_cond_signal(&cond_TF); //send signal

					mode_count++;
				}
				else if(DistanceSensor(2)<750&&DistanceSensor(3)<750&&threefold_v.order==2&&threefold_v.obstacle==RIGHT){
					DesireSpeed_Write(0);
					usleep(200000);

					SteeringServoControl_Write(1500);
					usleep(300000);
					DesireSpeed_Write(-100);
					EncoderCounter_Write(0);      //엔코더 초기화

					while(1){
						posRead=EncoderCounter_Read();
						printf("posRead main : %d\n",posRead);
						if(posRead<-500)
							break;
					}
					DesireSpeed_Write(0);
					usleep(300000);

					pthread_cond_signal(&cond_TF); //send signal

					mode_count++;
				}
				else;

				break;
			case 7 : // traffic_light check and stop 

#if 1

				if(!g_flag.traffic_light_flag){
					Whitesensor = LineSensor_Read();
					if(Whitesensor == 0){
						WhiteLineflag = 4;
						g_flag.traffic_light_flag = FLAG_ON;
					}

				}

				if(g_flag.traffic_light_flag){
					pthread_cond_signal(&cond_T);
				}

#endif

				break;
			default :
				break;
		}

		pthread_mutex_unlock(&mutex);

	}
	return NULL;
}

void * obstacle_thread(void *arg)
{
	MSG("           obstacle THR\n");
	while(1){
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond_Obs,&mutex); // receive the mission signal 


		MSG(" OBSTACLE MODE \n");
		obstacle();

		pthread_mutex_unlock(&mutex);
		if(g_flag.obstacle_after_flag)
			break;

	}

	pthread_exit(NULL); 
	return NULL;
}
void * parking_v_thread(void *arg)
{

	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&cond_P_v,&mutex); // receive the mission signal 

	parking_vertical();

	parking_step++;

	pthread_mutex_unlock(&mutex);

	if(parking_step >=2)
		g_flag.parking_after_flag = FLAG_ON;

	pthread_exit(NULL);
	return NULL;
}
void * parking_h_thread(void *arg)
{

	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&cond_P_h,&mutex); // receive the mission signal 

	parking_horizontal();		

	parking_step++;

	pthread_mutex_unlock(&mutex);

	if(parking_step>=2)
		g_flag.parking_after_flag = FLAG_ON;

	pthread_exit(NULL);
	return NULL;
}


void * rotate_thread(void *arg)
{

	while(1){
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond_Rtt,&mutex); // receive the mission signal 

		MSG(" ROTATE MODE \n");
		rotate_road();

		pthread_mutex_unlock(&mutex);

		if(g_flag.rotate_after_flag){
			MSG("END ROTATE THREAD\n");
			break;
		}
	}
	pthread_exit(NULL);	
	return NULL;
}
void * threefold_thread(void *arg){

	MSG("           threefold THR\n");
	while(1){
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond_TF,&mutex); // receive the mission signal 

		threefold();

		pthread_mutex_unlock(&mutex);

		if(mode_count == 8){
			MSG("END three THREAD\n");
			break;
		}
	}
	pthread_exit(NULL);
	return NULL;

}


void * traffic_light_thread(void *arg)
{

	while(1){
		MSG("           traffic THR\n");
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond_T,&mutex); // receive the mission signal 

		MSG(" traffic light MODE \n");
		traffic_light();

		pthread_mutex_unlock(&mutex);

	}
	pthread_exit(NULL);
	return NULL;
}
static struct thr_data* pexam_data = NULL;

void signal_handler(int sig)
{
	if(sig == SIGINT) {
		pthread_cancel(pexam_data->threads[0]);
		pthread_cancel(pexam_data->threads[2]);
		pthread_cancel(pexam_data->threads[3]);
		pthread_cancel(pexam_data->threads[4]);
		pthread_cancel(pexam_data->threads[5]);
		pthread_cancel(pexam_data->threads[6]);
		pthread_cancel(pexam_data->threads[7]);

		msgctl(pexam_data->msgq_id, IPC_RMID, 0);

		v4l2_streamoff(pexam_data->v4l2);
		vpe_stream_off(pexam_data->vpe->fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
		vpe_stream_off(pexam_data->vpe->fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

		disp_free_buffers(pexam_data->vpe->disp, NUMBUF);
		free_input_buffers(pexam_data->input_bufs, NUMBUF, false);
		free_overlay_plane(pexam_data->vpe->disp);

		disp_close(pexam_data->vpe->disp);
		vpe_close(pexam_data->vpe);
		v4l2_close(pexam_data->v4l2);

		usleep(50000);

		SteeringServoControl_Write(1500);
		DesireSpeed_Write(0);

		usleep(50000);	

		printf("-- thread test End --\n");
	}
}


static void variable_init(){
	/*run_init*/
	L_toX = 160;
	movestatus = 0;
	steercount = 0;
	turn_status = 0;

	/*rotate init*/
	rotate_flag = 0;
	rotate_count = 0;		

	WhiteLineflag = 0;
	stopline_check = 0;


	/*tunnel init*/
	E_tunnel_in_flag = 0;	

	/*parking init*/
	parking_step=0;

	/*traffic light init*/
	traffic_light_flag = 0;
	stay_count = 0;
	Whitecount = 0;
	signal_light = 0;

	mode_count =1;

	memset( (void*)&g_flag, 0x00 , sizeof(bit_flag_t) );
	memset( (void*)&threefold_v, 0x00 , sizeof(threefold_variable));

	PositionControlOnOff_Write(UNCONTROL);
	SpeedControlOnOff_Write(CONTROL);

	SpeedPIDProportional_Write(GAIN_P);
	SpeedPIDIntegral_Write(GAIN_I);
	SpeedPIDDifferential_Write(GAIN_D);


}

int main(int argc, char **argv)
{
	struct v4l2 *v4l2;
	struct vpe *vpe;
	struct thr_data tdata;
	int disp_argc = 3;
	char* disp_argv[] = {"dummy", "-s", "4:480x272", "\0"}; 
	int ret = 0;


	CarControlInit();
	variable_init();


	CameraYServoControl_Write(STRAIGHT_Y);
	CameraXServoControl_Write(1485);


	sleep(1);

	printf("-- thread test Start --\n");


	// open vpe
	vpe = vpe_open();
	if(!vpe) {
		return 1;
	}
	// vpe input (v4l cameradata)
	vpe->src.width  = CAPTURE_IMG_W;
	vpe->src.height = CAPTURE_IMG_H;
	describeFormat(CAPTURE_IMG_FORMAT, &vpe->src);

	// vpe output (disp data)
	vpe->dst.width  = VPE_OUTPUT_W;
	vpe->dst.height = VPE_OUTPUT_H;
	describeFormat (VPE_OUTPUT_FORMAT, &vpe->dst);

	vpe->disp = disp_open(disp_argc, disp_argv);
	if (!vpe->disp) {
		ERROR("disp open error!");
		vpe_close(vpe);
		return 1;
	}

	set_z_order(vpe->disp, vpe->disp->overlay_p.id);
	set_global_alpha(vpe->disp, vpe->disp->overlay_p.id);
	set_pre_multiplied_alpha(vpe->disp, vpe->disp->overlay_p.id);
	alloc_overlay_plane(vpe->disp, OVERLAY_DISP_FORCC, 0, 0, OVERLAY_DISP_W, OVERLAY_DISP_H);

	//vpe->deint = 0;
	vpe->translen = 1;

	MSG ("Input(Camera) = %d x %d (%.4s)\nOutput(LCD) = %d x %d (%.4s)",
			vpe->src.width, vpe->src.height, (char*)&vpe->src.fourcc,
			vpe->dst.width, vpe->dst.height, (char*)&vpe->dst.fourcc);

	if (    vpe->src.height < 0 || vpe->src.width < 0 || vpe->src.fourcc < 0 || \
			vpe->dst.height < 0 || vpe->dst.width < 0 || vpe->dst.fourcc < 0) {
		ERROR("Invalid parameters\n");
	}

	v4l2 = v4l2_open(vpe->src.fourcc, vpe->src.width, vpe->src.height);
	if (!v4l2) {
		ERROR("v4l2 open error!");
		disp_close(vpe->disp);
		vpe_close(vpe);
		return 1;
	}

	tdata.disp = vpe->disp;
	tdata.v4l2 = v4l2;
	tdata.vpe = vpe;
	tdata.bfull_screen = true;
	tdata.bstream_start = false;


	pexam_data = &tdata;






	ret = pthread_create(&tdata.threads[0], NULL, capture_thread, &tdata);
	if(ret) {
		MSG("Failed creating capture thread");
	}
	pthread_detach(tdata.threads[0]);	

	ret = pthread_create(&tdata.threads[1], NULL, sensor_thread, &tdata);
	if(ret) {
		MSG("Failed creating sensor thread");
	}
	pthread_detach(tdata.threads[1]);

	ret = pthread_create(&tdata.threads[2], NULL, obstacle_thread, &tdata);
	if(ret) {
		MSG("Failed creating obstacle thread");
	}
	pthread_detach(tdata.threads[2]);
	ret = pthread_create(&tdata.threads[3], NULL, parking_v_thread, &tdata);
	if(ret) {
		MSG("Failed creating parking_V thread");
	}
	pthread_detach(tdata.threads[3]);
	ret = pthread_create(&tdata.threads[4], NULL, rotate_thread, &tdata);
	if(ret) {
		MSG("Failed creating rotate thread");
	}
	pthread_detach(tdata.threads[4]);
	ret = pthread_create(&tdata.threads[5], NULL, parking_h_thread, &tdata);
	if(ret) {
		MSG("Failed creating parking_H thread");
	}
	pthread_detach(tdata.threads[5]);
	ret = pthread_create(&tdata.threads[6], NULL, traffic_light_thread, &tdata);
	if(ret) {
		MSG("Failed creating traffic_light thread");
	}
	pthread_detach(tdata.threads[6]);
	ret = pthread_create(&tdata.threads[7], NULL, threefold_thread, &tdata);
	if(ret) {
		MSG("Failed creating threefold thread");
	}
	pthread_detach(tdata.threads[7]);

	/* register signal handler for <CTRL>+C in order to clean up */
	if(signal(SIGINT, signal_handler) == SIG_ERR) {
		MSG("could not register signal handler");
		closelog();
		exit(EXIT_FAILURE);
	}

	pause();

	return ret;

}
