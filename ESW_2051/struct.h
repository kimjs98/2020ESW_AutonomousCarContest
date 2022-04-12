//#ifndef _STRUCT_H_
//#define _STRUCT_H_

#ifdef _STRUCT_

#undef _STRUCT_
#define _EXT_STRUCT_ 

#else

#define _EXT_STRUCT_	extern

#endif

typedef volatile struct bit_flag{
        int run_flag:1;
        int start_flag:1;
        int circle_flag:1;
        int obstacle_flag:1;
        int obstacle_detect_flag:1;

        int dist_flag:1;
        int parking_flag:1;
        int rotate_road_flag:1;
        int WhiteLine_flag:1;

        int tunnel_flag:1;
	int threefold_flag:1;
	int traffic_light_flag:1;

	int yellow_white_flag:1;

        /*after flag*/
        int parking_after_flag:1;
        int obstacle_after_flag:1;
        int rotate_after_flag:1;
	int threefold_after_flag:1;
	int tunnel_after_flag:1;


}bit_flag_t;

_EXT_STRUCT_ bit_flag_t g_flag;

typedef volatile struct Circle{
	int center_x;
	int center_y;
	int radius;
}Circle_val;


_EXT_STRUCT_ Circle_val cir; // detected circle value


typedef volatile struct PT{
    double toX;
    double toY;
}PT_val;

 
_EXT_STRUCT_ PT_val topt, // loss point x,y 

		    LS1, // hough_transform 1
		    LS2, //
		    RS1, //
		    RS2, //

		    SLS1, // hough_transform 2
		    SLS2, //
		    SRS1, //
		    SRS2, //

		    PT2; //  pt 1 origin pt 2 new



typedef volatile struct Threefold_Variable
{
        int obstacle;                   //3차로 장애물
        int order;                      //3차로 추월지시과정
        int type;                       //0:기본, 1:왼쪽앞, 2:오른쪽앞, 3:오른쪽뒤, 4:왼쪽뒤
}threefold_variable;

_EXT_STRUCT_ threefold_variable threefold_v;



