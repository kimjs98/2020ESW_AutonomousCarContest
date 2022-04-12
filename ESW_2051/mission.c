
#include"car_lib.h"
#include"mission.h"

#include"variable.h"
#include"struct.h"


int race_start(){
	int race = 0;

	if(DistanceSensor(3) > 4000){
		printf("START!\n");
		race = 1;
	}
	return race;
}

int overpass_downhill(){
	int fin = 0;

	printf("D(2) : %d, D(3) : %d, ===  D(5) : %d, D(6) : %d \n ", DistanceSensor(2),DistanceSensor(3),DistanceSensor(5), DistanceSensor(6));

	if(((DistanceSensor(2) < 750) && (DistanceSensor(3) < 750)) && ((DistanceSensor(5) < 750 ) && (DistanceSensor(6) < 750)))

		fin = 1;

	return fin;
}


void obstacle(){
	printf("obstacle!\n");
	DesireSpeed_Write(0);
}
void parking_vertical(){


	printf("vertical parking!\n");

	int parking_count=0;

	DesireSpeed_Write(0);
	SteeringServoControl_Write(1000);
	usleep(500000);
	DesireSpeed_Write(-100);

	while(1){

		if( parking_count==0 && DistanceSensor(3)>1500 && DistanceSensor(5)>1500){

			printf("park step1\n");
			SteeringServoControl_Write(1500);
			DesireSpeed_Write(0);
			parking_count++;
		}
		else if( parking_count==1){

			printf("park step2\n");
			usleep(100000);
			DesireSpeed_Write(-100);
			parking_count++;
		}
		else if( parking_count==2 && DistanceSensor(4)>1800){

			printf("park step3\n");
			DesireSpeed_Write(0);

			usleep(500000);
			Alarm_Write(ON);
			usleep(800000);
			Alarm_Write(OFF);


			parking_count++;
		}
		else if( parking_count== 3 ){

			printf("park step4\n");
			DesireSpeed_Write(100);
			usleep(1000000);
			SteeringServoControl_Write(1000);
			usleep(2200000);
			break;
		}
	}
	P_checkflag=0;


}
void parking_horizontal(){

	printf("horizontal parking!\n");


	DesireSpeed_Write(0);
	usleep(80000);
	SteeringServoControl_Write(1100); //우측후진
	usleep(500000);


	DesireSpeed_Write(-100);
	while(1){
		if(DistanceSensor(4)>550&&DistanceSensor(3)>1100)
			break;
		printf("%d,%d",DistanceSensor(4),DistanceSensor(3));
		printf("check\n");
	}
	printf("%d\n",DistanceSensor(4));

	DesireSpeed_Write(0);
	usleep(80000);
	SteeringServoControl_Write(2000); //좌측후진
	usleep(500000);

	DesireSpeed_Write(-100);

	while(DistanceSensor(4)<2800){
		printf("%d",DistanceSensor(4));
		printf("check2\n");

	}

	DesireSpeed_Write(0);
	usleep(500000);
	Alarm_Write(ON);
	usleep(800000);
	Alarm_Write(OFF);

	SteeringServoControl_Write(2000); //좌측전진
	usleep(500000);
	DesireSpeed_Write(100);

	posInit = 0;  //initialize	
	EncoderCounter_Write(posInit);

	while(DistanceSensor(2)>400);

	SteeringServoControl_Write(1000); //우측전진
	usleep(500000);
	DesireSpeed_Write(100);

	posInit = 0;  //initialize
	EncoderCounter_Write(posInit);

	while(1){
		printf("in while parking\n");	
		posRead=EncoderCounter_Read();
		printf("posRead : %d\n",posRead);
		if(posRead>350)
			break; 
	}

	P_checkflag=0;

}

void rotate_road(){



	if(rotate_flag == 0){  // 정지후 전방 거리센서 인식

		if(DistanceSensor(1) > 500)
			rotate_flag = 1;
	}
	else if(rotate_flag == 1){ // 가까이있던 물체가 멀어지면 

		if(DistanceSensor(1) < 75)	
			rotate_flag = 2;
	}
	else if(rotate_flag == 2){  // 출발 , 전방센서에서 검출되면 3
		stay_count++; 
		if((DistanceSensor(1) > 1200) || (DistanceSensor(6) > 1200)|| (stay_count > 20)){
			rotate_flag = 3;
			stay_count = 0;
		}

	}
	else if(rotate_flag == 3){

		if((DistanceSensor(1) > 1200) || (DistanceSensor(6) > 1200))
			rotate_flag = 3;
		else if(DistanceSensor(4) > 700)
			rotate_flag = 4;

	}
	else if(rotate_flag == 4){

		if((DistanceSensor(1) > 1200) || (DistanceSensor(6) > 1200)) 
			rotate_flag = 3;
	}



}

void tunnel_in(){
	printf(" tunnel in\n");
	E_tunnel_in_flag = 1;
	CarLight_Write(FRONT_ON);
}
void tunnel_out(){
	printf(" tunnel out\n");
	E_tunnel_in_flag = 0;

	CarLight_Write(ALL_OFF);
}



void threefold(){


	if(threefold_v.order==START){                                          //추월 시작

		DesireSpeed_Write(0);
		usleep(500000);	
		if(threefold_v.type==STANDARD){		//기본 상황, 추월대상과 장애물이 같은 라인에 위치
			SteeringServoControl_Write(2000);		//왼쪽살짝 주행
			usleep(300000);
			DesireSpeed_Write(80);
			EncoderCounter_Write(0);                        //엔코더의 초기화
			while(1){
				posRead=EncoderCounter_Read();
				printf("posRead st1: %d",posRead);
				if(posRead<10000)
					if(posRead>360)
						break;
			}
			DesireSpeed_Write(0);

			if(DistanceSensor(1)>100)
				threefold_v.obstacle=LEFT;	
			else if(DistanceSensor(1)<100)
				threefold_v.obstacle=RIGHT;

			usleep(500000);
			DesireSpeed_Write(-80);
			EncoderCounter_Write(0);       	                //엔코더의 초기화
			while(1){
				posRead=EncoderCounter_Read();
				printf("posRead st2: %d",posRead);
				if(posRead>-10000)
					if(posRead<-300)
						break;
			}
			DesireSpeed_Write(0);
			usleep(500000);
		} //threefold_v.type==STANDARD end
		else if(threefold_v.type==RIGHT_F){
			threefold_v.obstacle=RIGHT;		
		}
		else if(threefold_v.type==LEFT_F){
			threefold_v.obstacle=LEFT;
		}
		else;
		if(threefold_v.obstacle==LEFT){                         //장애물 좌측  위치(우측진행)
			threefold_v.order=FINISH;                       //추월탈출을 위한 체크
			printf("obstacletestleft");
			SteeringServoControl_Write(1000);               //우측전진
			usleep(500000);
			DesireSpeed_Write(100);
			EncoderCounter_Write(0);                        //엔코더의 초기화
			Winker_Write(RIGHT_ON);

			while(1){
				posRead=EncoderCounter_Read();
				printf("posRead le1: %d",posRead);
				if(posRead<10000)
					if(posRead>550)
						break;
			}
			Winker_Write(ALL_OFF);
			SteeringServoControl_Write(1950);               //좌측전진
			EncoderCounter_Write(0);
			while(1){
				posRead=EncoderCounter_Read();
				printf("posRead le2: %d",posRead);
				if(posRead<10000)
					if(posRead>750)
						break;
			}
		} //threefold_obstacleflag==LEFT end
		else if(threefold_v.obstacle==RIGHT){                 //장애물 우측 위치(좌측진행)
			threefold_v.order=FINISH;                     //추월탈출을 위한 체크
			printf("obstacletestright");
			SteeringServoControl_Write(2000);               //좌측전진
			usleep(500000);
			DesireSpeed_Write(100);
			EncoderCounter_Write(0);                        //엔코더의 초기화
			Winker_Write(LEFT_ON);

			while(1){
				posRead=EncoderCounter_Read();
				printf("posRead r1: %d",posRead);
				if(posRead<10000)
					if(posRead>560)
						break;
			}
			Winker_Write(ALL_OFF);
			SteeringServoControl_Write(1050);               //우측전진
			EncoderCounter_Write(0);
			while(1){
				posRead=EncoderCounter_Read();
				printf("posRead r2: %d",posRead);
				if(posRead<10000)
					if(posRead>750)
						break;
			}
		} //threefold_obstalceflag==RIGHT end
		else;

	}//threefoldcheck==1 end

	else if(threefold_v.order==FINISH){                                     //추월 끝
		if(threefold_v.obstacle==LEFT){                       //장애물 좌측위치(우측진행)
			threefold_v.order=END;                               //추월탈출을 위한
			printf("outtestright");
			SteeringServoControl_Write(2000);               //좌측전진
			usleep(300000);
			DesireSpeed_Write(100);
			EncoderCounter_Write(0);                       //엔코더의 초기화
			Winker_Write(LEFT_ON);
			while(1){
				posRead=EncoderCounter_Read();
				printf("posRead lo1: %d",posRead);
				if(posRead<10000)
					if(posRead>580)
						break;
			}
			Winker_Write(ALL_OFF);
			SteeringServoControl_Write(1050);               //우측전진
			EncoderCounter_Write(0);
			while(1){
				posRead=EncoderCounter_Read();
				printf("posRead lo2: %d",posRead);
				if(posRead<10000)
					if(posRead>720)
						break;
			}
		}
		else if(threefold_v.obstacle==RIGHT){                 //장애물우측위치(좌측진)
			threefold_v.order=END;                               //추월탈출을 위한 체크
			printf("outtestleft");
			SteeringServoControl_Write(1000);               //우측전진
			usleep(300000);
			DesireSpeed_Write(100);
			EncoderCounter_Write(0);                        //엔코더의 초기화행
			Winker_Write(RIGHT_ON);
			while(1){
				posRead=EncoderCounter_Read();
				printf("posRead ro1: %d",posRead);
				if(posRead<10000)
					if(posRead>650)
						break;
			}
			Winker_Write(ALL_OFF);
			SteeringServoControl_Write(1950);               //좌측전진
			EncoderCounter_Write(0);
			while(1){
				posRead=EncoderCounter_Read();
				printf("posRead ro2: %d",posRead);
				if(posRead<10000)
					if(posRead>720)
						break;
			}

		}
		threefold_v.type=5;
		g_flag.threefold_after_flag = 1;
	} //threefoldcheck==2 end

}


void traffic_light(){


	int i = 0;

	if(traffic_light_flag == 0){
		CameraYServoControl_Write(1500);
		DesireSpeed_Write(0);

		Alarm_Write(ON);
		usleep(100000);
		Alarm_Write(OFF);

		traffic_light_flag = 1;
	}
	else if(traffic_light_flag == 1){
		printf("signal_light %d \n",signal_light);
		if(signal_light >= 3){
			traffic_light_flag = 2;
		}
	}
	else if(traffic_light_flag == 2){
		CameraYServoControl_Write(1650);
		stay_count++;
		printf("stay_count : %d \n", stay_count);
		if(stay_count >= 7){
			traffic_light_flag = 3;
			stay_count = 0;
		}
	}
	else if(traffic_light_flag == 3){
		printf("DDD \n");
	}
	else if(traffic_light_flag == 4){
		if(signal_light == 5){
			SteeringServoControl_Write(2000);
			traffic_light_flag = 5;
		}
		else if(signal_light == 6){
			SteeringServoControl_Write(1000);
			traffic_light_flag = 5;
		}
	}
	else if(traffic_light_flag == 5){
		DesireSpeed_Write(85);
		EncoderCounter_Write(0);
		Winker_Write(LEFT_ON);
		while(1){
			posRead = EncoderCounter_Read();
			printf(" posRead : %d \n", posRead);
			if(posRead<10000){
				if(signal_light == 5){
					if((posRead>(1060 + slope)) && (posRead < 10000)){
						traffic_light_flag = 6;
						break;
					}
				}
				if(signal_light == 6){
					if((posRead>(925 - slope)) && (posRead < 10000)){
						traffic_light_flag = 6;
						break;
					}
				}

			}
		}
	}
	else if(traffic_light_flag == 6){
		SteeringServoControl_Write(1500);
		Whitecount = 0;
		Winker_Write(RIGHT_ON);
		traffic_sensor = LineSensor_Read();        // black:1, white:0
		for(i=0; i<7; i++){

			if(!(traffic_sensor & 0x40)){
				Whitecount++;
			}
			traffic_sensor = traffic_sensor << 1;

			if(Whitecount >= 3)
				traffic_light_flag = 7;
		}

	}
	else if(traffic_light_flag == 7){
		DesireSpeed_Write(85);
		sleep(1);
		DesireSpeed_Write(0);

		Alarm_Write(ON);
		usleep(100000);
		Alarm_Write(OFF);

		CarLight_Write(ALL_OFF);
		Winker_Write(ALL_OFF);

		traffic_light_flag = 8;
		mode_count ++;
	}


}
