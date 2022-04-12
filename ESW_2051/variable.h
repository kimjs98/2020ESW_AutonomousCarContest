
#ifdef _MAIN_
#ifndef	_EXT_
#define _EXT_
#endif 
#else
#ifndef	_EXT_
#define _EXT_	extern
#endif
#endif



#define FLAG_ON 	 1
#define FLAG_OFF 	 0

#define STOP		 0
#define BACK		 -100
#define GO		 100

#define LEFT		 0
#define RIGHT		 1



#define WALL_LIMIT       600


#define STRAIGHT_Y       1650 //        camera_angle_Y
#define CURVE_Y          1660   //      camera_angle_Y


#define STRAIGHT         0
#define LEFT_TURN        1
#define RIGHT_TURN       2

#define PI      3.1415926
#define YELLOW  0
#define RED     1
#define WHITE   2
#define WHITE_CHECK 4

#define HOUGHLINE_NUM 30 ////////40
#define THRESHOLD 180



#define STRAIGHT_SPEED 135      // 125  // 135  // 145  // 155 // 160 (steercount 0)
#define CURVE_SPEED 120         // 110  // 120  // 130  // 140 // 150 


#define RED_LIMIT 1000   // 수정 필요합니다
#define RED_FIRST_LIMIT 500

// WhiteLine flag state  
#define DRIVE_START	0
#define ROTATING	1
#define ROTATE_END	2
#define TRAFFIC_LIGHT	3
#define TRAFFIC_START	4



///////3차로미션 장애물 위치///
#define LEFT 0
#define RIGHT 1
////type
#define STANDARD 0                      //0:기본, 1:왼쪽 앞, 2:오른쪽앞, 3:오른쪽 뒤, 4:왼쪽 뒤
#define LEFT_F 1
#define RIGHT_F 2
#define RIGHT_B 3
#define LEFT_B 4
//order
#define START 1
#define FINISH 2
#define END 3
///////////////////////

/*PID gain*/
#define GAIN_P          43
#define GAIN_I          3
#define GAIN_D          20



_EXT_ int Linenum;
_EXT_ int Red_val;
_EXT_ int rotate_flag;
_EXT_ int P_checkflag;
_EXT_ int Whitecheck_flag;


/*run variable*/
_EXT_ int L_toX,
	  movestatus,
	  steercount,
	  turn_status;


/*parking variable*/
_EXT_ int parking_type,
	  parking_count,
	  parking_step;

/*rotate variable */
_EXT_ int WhiteLineflag, 	// main.c exam.cpp 
	  rotate_count, 	// main.c 
	  stopline_check;		// main.c 

/*threeefold variable*/
_EXT_ int threefoldcheck,
	  threefold_obstacleflag,
	  threefold_type;

/*tunnel variable*/
_EXT_ int E_tunnel_in_flag;


/*traffic_light variable*/
_EXT_ int traffic_light_flag,
	  stay_count,
	  traffic_sensor,
	  Whitecount,
	  slope,
	  signal_light;

////////////////////////////


_EXT_ int linemissionflag,
	  posInit,
	  posRead,
	  mode_count,
	  hue_change;













