// Julie
// GTP対戦メイン
// katsuki Ohto

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
	if(com_log != nullptr){
		fprintf(com_log, "%s", ss.ss.str().c_str());
		ss.clear();
	}
}

//通信プロトコルとの対応

#include "constant.h"
#include "go.hpp"
#include "hash/hash.hpp"
#include "board.hpp"

using namespace Go;

namespace Go{
	namespace GTP{
		void send_gtp(const char *fmt,...){
			
			va_list ap;
			va_start(ap, fmt);
			vfprintf(stdout, fmt, ap);
			
			// GTP通信の内容をログ出力
			if(com_log != nullptr){
				fprintf(com_log, "client : ");
				fprintf(com_log, fmt, ap);
			}
			va_end(ap);
		}
		
		void sendPlay_gtp(int len, const int z){
			
			char str[16];
			char *tmp = str;
			*tmp='='; tmp++;
			*tmp=' '; tmp++;
            
			int x = ZtoX(z), y = ZtoY(z);
			
			if(isOnBoard(len, x, y)){
				tmp = XtoSTR(tmp,x);
				tmp = YtoSTR(tmp,y);
			}else{
				int z = XYtoZ(x,y);
				//CGFGOBANに対しては投了せずパスするのみ
#ifdef CGFGOBAN
				*tmp='p'; tmp++;
				*tmp='a'; tmp++;
				*tmp='s'; tmp++;
				*tmp='s'; tmp++;
#else
				if(z == Z_RESIGN){
					*tmp='r'; tmp++;
					*tmp='e'; tmp++;
					*tmp='s'; tmp++;
					*tmp='i'; tmp++;
					*tmp='g'; tmp++;
					*tmp='n'; tmp++;
				}else{
					*tmp='p'; tmp++;
					*tmp='a'; tmp++;
					*tmp='s'; tmp++;
					*tmp='s'; tmp++;
				}
#endif
			}
			*tmp='\n'; tmp++;
			*tmp='\n'; tmp++;
			*tmp='\0';
            
			send_gtp(str);
		}
		
		int recvPlay_gtp(const char *const play){

			if(std::strstr(play, "pass") || std::strstr(play, "PASS")){//パス
				return Z_PASS;
			}else{
				x = (int)play[0] - 'A' + 1;
				if(play[0] > 'I'){
					--x;
				}
				if('0' <= play[2] && play[2] <= '9'){ // 次が数字
					y = ((int)play[1] - '0') * 10 + ((int)play[2] - '0');
				}else{
					y = (int)play[1] - '0';
				}
				return XYtoZ(x, y);
			}
			
			return 0;
		}
		
	}
}

#include "kanon.hpp"

Kanon::Client client;

int main(int argc, char* argv[]){
	
	char str[256];
	int myMove;
	ClockMS clms;
	
	//基本的な初期化
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	
	//クライアント初期化
	client.initAllG();
	
	//試合フェーズ
	while(1){
		if(fgets(str, 256, stdin) == NULL){ break; }
		
		CERR << "server : " << str;
		
		out_ss();
		
		std::queue<std::string> q;
		
		char *tok = strtok(str," \n");
		if(tok != nullptr){
			q.push(std::string(tok));
			while(1){
				tok = strtok(nullptr, " \n");
				if(tok == nullptr){ break; }
				q.push(std::string(tok));
			}
		}
		
		std::string com = q.front();
		q.pop();
		
		if(boost::iequals(com, "boardsize")){
			int len = atoi(q.front().c_str());
			
			CERR << "length of board = " << len << endl;
			
			if(client.init1G(len) < 0){ return -1; }//試合前の初期化
			client.setHandicapStones();//置き石情報受け取り
			client.setTimeLimit(180000, 0);
			
			GTP::send_gtp("= \n\n");
		}else if(boost::iequals(com, "clear_board")){
			clms.start();
			GTP::send_gtp("= \n\n");
		}else if(boost::iequals(com, "name")){
			GTP::send_gtp("= %s\n\n", MY_NAME);
		}else if(boost::iequals(com, "version")){
			GTP::send_gtp("= %s\n\n", MY_VERSION);
		}else if(boost::iequals(com, "genmove")){
			
			if(boost::iequals(q.front(), "b")){
				myMove = client.play(BLACK);
				client.recvPlay(BLACK, myMove, clms.stop());
				GTP::sendPlay_gtp(client.boardLength, myMove);
				clms.start();//相手の時間計測開始
				//待ち状態に入る
				client.wait();
			}else if(boost::iequals(q.front(), "w")){
				myMove = client.play(WHITE);
				client.recvPlay(WHITE, myMove, clms.stop());
				GTP::sendPlay_gtp(client.boardLength, myMove);
				clms.start();//相手の時間計測開始
				//待ち状態に入る
				client.wait();
			}else{
			}
		}else if(boost::iequals(com, "play")){
			if(boost::iequals(q.front(), "b")){
				q.pop();
				int z = GTP::recvPlay_gtp(q.front().c_str());
				client.recvPlay(BLACK, z, clms.stop());
			}else if(boost::iequals(q.front(), "w")){
				q.pop();
				int z = GTP::recvPlay_gtp(q.front().c_str());
				client.recvPlay(WHITE, z, clms.stop());
			}else{
			}
			clms.start();//自分の時間計測開始
			GTP::send_gtp("= \n\n");
		}else{
			GTP::send_gtp("= \n\n");
		}
		
		out_ss();
	}
	
	CERR << "%s : all games were finished.\n";
	
	return 0;
}



