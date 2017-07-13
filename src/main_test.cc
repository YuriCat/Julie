
//叶音
//GTP対戦メイン
//katsuki Ohto

#include <cstdio>
#include <ctime>
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <cstring>
#include <cstdarg>
#include <string>
#include <random>
#include <array>
#include <vector>
#include <bitset>
#include <queue>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>

#include <unistd.h>
#include <sys/time.h>

#include <boost/algorithm/string.hpp>


//ルール
#include "settings.h"


//基本的定義、ユーティリティ
#include "../common/common_all.h"

//エンジンとしてのビルドの場合、cerrの出力先を変える
struct StringStream{
	std::stringstream ss;
	
	void clear(){
		ss.str("");
		ss.clear(stringstream::goodbit);
	}
};

StringStream ss;


#ifdef ENGINE

#ifdef CERR
#undef CERR
#define CERR ss.ss
#endif

#endif


FILE* com_log;//通信ログ
FILE* thinking_log;//思考ログ

void out_ss(){
	if( com_log!=nullptr ){
		fprintf( com_log, "%s", ss.ss.str().c_str());
		ss.clear();
	}
}

//通信プロトコルとの対応

#include "constant.h"
#include "tools.hpp"
#include "hash/hash.hpp"
#include "board.hpp"

using namespace Go;

namespace Go{
	namespace GTP{
		void send_gtp(const char *fmt,...){
			
			va_list ap;
			va_start(ap,fmt);
			vfprintf(stdout,fmt,ap);
			
			//GTP通信の内容をログ出力
			if( com_log != nullptr ){
				fprintf( com_log, "client : " );
				fprintf( com_log, fmt, ap );
			}
			
			va_end(ap);
		}
		
		void sendPlay_gtp(int b_len,const POINT& p){
			
			char str[16];
			char *tmp=str;
			*tmp='=';tmp++;
			*tmp=' ';tmp++;
			
			if( isOnBoard(b_len,p.x,p.y) ){
				tmp=XtoSTR(tmp,p.x);
				tmp=YtoSTR(tmp,p.y);
			}else{
				int z=XYtoZ(p.x,p.y);
				//CGFGOBANに対しては投了せずパスするのみ
#ifdef CGFGOBAN
				*tmp='p';tmp++;
				*tmp='a';tmp++;
				*tmp='s';tmp++;
				*tmp='s';tmp++;
#else
				if( z==Z_RESIGN ){
					*tmp='r';tmp++;
					*tmp='e';tmp++;
					*tmp='s';tmp++;
					*tmp='i';tmp++;
					*tmp='g';tmp++;
					*tmp='n';tmp++;
				}else{
					*tmp='p';tmp++;
					*tmp='a';tmp++;
					*tmp='s';tmp++;
					*tmp='s';tmp++;
				}
#endif
			}
			*tmp='\n';tmp++;
			*tmp='\n';tmp++;
			*tmp='\0';
			
			send_gtp(str);
		}
		
		int recvPlay_gtp(POINT*const point,const char*const play){
			int x,y;
			
			if( std::strstr(play,"pass") || std::strstr(play,"PASS") ){//パス
				point->setPASS();
			}else{
				x=(int)play[0]-'A'+1;
				if( play[0]>'I' ){
					--x;
				}
				
				if( 48<=play[2] && play[2]<=57 ){//次が数字
					y=((int)play[1]-48)*10+((int)play[2]-48);
				}else{
					y=(int)play[1]-48;
				}
				
				point->x=x;
				point->y=y;
				
			}
			
			return 0;
		}
		
	}
}

#include "kanon.hpp"

Kanon::Client client;

int main(int argc,char* argv[]){
	
	char str[256];
	POINT tmpPoint;
	POINT myMove;
	ClockMS clms;
	
	//基本的な初期化
	setvbuf(stdout,NULL,_IONBF,0);
	setvbuf(stdin,NULL,_IONBF,0);
	setvbuf(stderr,NULL,_IONBF,0);
	
	//クライアント初期化
	client.initAllG();
	
	constexpr int B_LENGTH=9;
	
	//簡易デバッグモード
	//ランダム着手の相手に対して戦う
	Board<B_LENGTH> bd;
	XorShift64 dice;
	dice.srand((uint32_t)time(NULL));
	bd.init();
	client.init1G(B_LENGTH);//試合前の初期化
	client.setTimeLimit(10000,15000);
	
	int col=BLACK;
	for(int t=0;t<500;++t){
		
		cerr<<"  turn : "<<t<<endl;
		
		int z;
		clms.start();
		if( col==BLACK ){
			myMove=client.play(BLACK);
			client.recvPlay(col,myMove,clms.stop());
			z=PtoZ(myMove);
			
		}else{
			
			//if( bd.last_z==Z_PASS ){
			//	z=Z_PASS;
			//}else{
			client.wait();
			
			Move mv[B_LENGTH*B_LENGTH+1];
			int nmoves=fill_legal_moves(mv,bd,col);
			assert( nmoves>0 );
			z=mv[ dice.rand()%nmoves ].z;
			client.recvPlay(col,POINT(z),clms.stop());
			//}
		}
		
		if(col==BLACK){
			cerr<<"BLACK : ";
		}else{
			cerr<<"WHITE : ";	
		}
		cerr<<OutZ(z)<<endl;
		
		if(z==Z_RESIGN ||(z==Z_PASS && bd.last_z==Z_PASS)){
			break;//連続パスで終了
		}
		bd.move(col,z);
		col=flipColor(col);
		cerr<<bd;
	}
	
	//結果表示
	int sc2=bd.count_score_CHINESE();
	cerr<<"Score : "<<sc2<<endl;
	
	if( sc2>0 ){
		cerr<<"BLACK";
	}else{
		cerr<<"WHITE";
	}
	cerr<<" WON!!"<<endl;
	
	return 0;
}



