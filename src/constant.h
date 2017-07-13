//囲碁定数

namespace Go{
	
	//汎用
	constexpr int BOARD_ARRAY_LENGTH=32;
	constexpr int MC_DEPTH_MAX=50;
	
	constexpr int MAX_DIAMOND=2;
	
	constexpr int NEXT_2EYE_STOP_MAX=40;
	
	constexpr int N_REN_MAX=10;
	
	constexpr int TE_INFO_MAX=10;
	
	constexpr int N_KOKYU_MAX=4;
	
	//9路
	constexpr int REN_SIZE_MAX_9=61;
	constexpr int MC_EXTRACT_LINE_9=6;
	//19路
	constexpr int MC_EXTRACT_LINE_19=10;
	
	
	//定数を返す関数
	constexpr int getBoardSize(int b_len)noexcept{
		return b_len * b_len;
	}
	constexpr int getBoardArraySize(int b_len){
		return BOARD_ARRAY_LENGTH * (b_len + 2);
	}
	constexpr int getRenSizeMax(int b_len){
		return  (b_len==9) ? REN_SIZE_MAX_9
		:( 4*b_len*(b_len-1)/5 + 4 );
	}
	constexpr int getMCExtractLine(int b_len){
		return (b_len==9) ? MC_EXTRACT_LINE_9
		: MC_EXTRACT_LINE_19;
	}
	constexpr int getNTurnsMax(int b_len)noexcept{
		return b_len * b_len + 200;
	}
	
	//盤の端の座標
	constexpr int U=-BOARD_ARRAY_LENGTH;
	constexpr int D=+BOARD_ARRAY_LENGTH;
	constexpr int L=-1;
	constexpr int R=+1;
	
#define Z_FIRST ( 1+(BOARD_ARRAY_LENGTH) )
#define X_FIRST (1)
#define Y_FIRST (1)

	constexpr int getZLast(int b_len){
		return (BOARD_ARRAY_LENGTH)*(1+b_len) + b_len;
	}
	constexpr int getXLast(int b_len){
		return b_len;
	}
	constexpr int getYLast(int b_len){
		return b_len;
	}
	
}