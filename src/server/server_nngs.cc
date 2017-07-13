
// 囲碁
// NNGS Server
// Katsuki Ohto

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

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#else

#include <sys/param.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

#endif

// ルール
#include "settings.h"


//基本的定義、ユーティリティ
#include "../common/common_all.h"

using std::string;
using std::stringstream;

//エンジンとしてのビルドの場合、cerrの出力先を変える
struct StringStream{
	stringstream ss;

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
	if (com_log != nullptr){
		fprintf(com_log, "%s", ss.ss.str().c_str());
		ss.clear();
	}
}

//通信プロトコルとの対応

string ip = "jsb.cs.uec.ac.jp";
int port = 9696;
int fTurn = 0;
string my_id = "kanon";
string ops_id = "NoName";

const char *stone_str[2] = { "B", "W" };

struct NETWORKINFO{
	bool IsConnected;

	char IP[32];
	unsigned short PORT;
	char ID[32];
	char PASS[32];
	char NAME[32];

	int dstSocket;
	//HANDLE hThread;
	struct sockaddr_in dstAddr;
};

NETWORKINFO NetworkInfo;

#include "constant.h"
#include "tools.hpp"
#include "hash/hash.hpp"
#include "board.hpp"

int sendCommand(const char* cmd_send){
	if (strlen(cmd_send)>0){
		CERR << "Client << " << cmd_send << endl;
	}
	send(NetworkInfo.dstSocket, cmd_send, strlen(cmd_send) + 1, 0);
	return 0;
}

using namespace Go;

namespace Go{
	namespace NNGS{
		void sendPlay_nngs(int b_len, const int z){
			char str[16];
			int x, y;
			if (z == Z_PASS){
				snprintf(str, sizeof(str), "pass\n");
			}else{
				y = ZtoY(z);
				x = ZtoX(z);
				x = 'A' + x - 1;
				if (x >= 'I') x++;	// 'I' は使わないので。'A'-'H','J'-'T'までを使う。
				snprintf(str, sizeof(str), "%c%d\n", x, y);
			}
			sendCommand(str);
		}
		/*
		int recvPlay_nngs(POINT*const point, const char*const play){

			return 0;
		}*/

	}
}

#include "kanon.hpp"

Kanon::Client client;

void searchToken(const char* str, const int size, int *ret){
	for (int c = *ret; c < size - 1; ++c){
		if (str[c] == '\0'){
			if (str[c + 1] == '\0'){
				*ret = -1; return;
			}
			else{
				*ret = c + 1; return;
			}
		}
	}
	*ret = -1;
	return;
}



int connectNNGS(){

	char cmd_send[1024] = {0};

	//通信設定
	strcpy(NetworkInfo.IP, ip.c_str());
	NetworkInfo.PORT = port;
	NetworkInfo.IsConnected = false;

#ifdef _WIN32
	//Windows 独自の設定
	WSADATA data;
	if (SOCKET_ERROR == WSAStartup(MAKEWORD(2, 0), &data)){
		cerr << "failed to initialize WSA-data." << endl;
		return -1;
	}
#endif

	//open socket
	int dstSocket;
	dstSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (NetworkInfo.dstSocket<0){
		cerr << "failed to open socket." << endl;
	}

	//sockaddr_in 構造体のセット
	memset(&NetworkInfo.dstAddr, 0, sizeof(NetworkInfo.dstAddr));
	NetworkInfo.dstAddr.sin_port = htons(NetworkInfo.PORT);
	NetworkInfo.dstAddr.sin_family = AF_INET;
	NetworkInfo.dstAddr.sin_addr.s_addr = inet_addr(NetworkInfo.IP);

	//接続
	if (-1 == connect(NetworkInfo.dstSocket, (struct sockaddr *) &NetworkInfo.dstAddr, sizeof(NetworkInfo.dstAddr))){
		cerr << "failed to connect to server." << endl;
		return -1;
	}

	//ログイン
	snprintf(cmd_send, sizeof(cmd_send), "%s\n", my_id.c_str());
	sendCommand(cmd_send);

	//接続完了
	NetworkInfo.IsConnected = true;
	CERR << "connected to server." << endl;

	return 0;
}

int main(int argc, char* argv[]){

	char cmd_recv[1024];
	char cmd_send[1024];
	int myMove;
	ClockMS clms;

	//基本的な初期化
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	// 起動オプションをチェック
	if (argc >= 2 && *argv[1] == 'W') fTurn = 1; // 白番
	if (argc >= 3) ip = string(argv[2]);// サーバIPを指定した場合
	if (argc >= 4) port = atoi(argv[3]);//ポート
	if (argc >= 5) ops_id = string(argv[4]);//相手の名前

	if (connectNNGS()<0){
		CERR << "failed to make NNGS connection." << endl;
		return 1;
	}

	//クライアント初期化
	client.initAllG();
	CERR << MY_NAME << " : initialized for all games." << endl;

	int x, y, z;

	while (1){

		char *tok;
		int c;

		memset(cmd_recv, 0, sizeof(char) * 1024);
		memset(cmd_send, 0, sizeof(char) * 1024);

		int NBytesRead = recv(NetworkInfo.dstSocket, cmd_recv, sizeof(cmd_recv), 0);
		if (NBytesRead<0){ return -1; }

		c = 0;
		while (c >= 0){
			tok = &cmd_recv[c];

			CERR << "Server >> " << tok << endl;
			out_ss();

			if (strstr(tok, "No Name Go Server")){

				sendCommand("set client TRUE\n");// シンプルな通信モードに

			}
			else if (strstr(tok, "Set client to be True")){
				if (fTurn == 0){
					snprintf(cmd_send, sizeof(cmd_send), "match %s B 19 30 0\n", ops_id.c_str());
					sendCommand(cmd_send);	// 黒番の場合に、試合を申し込む
				}
			}
			else if (strstr(tok, "Match [19x19]")){

				if (fTurn == 1){
					snprintf(cmd_send, sizeof(cmd_send), "match %s W 19 30 0\n", ops_id.c_str());
					sendCommand(cmd_send);	// 白番の場合に、試合を受ける
				}

				CERR << MY_NAME << " : initialized board_size 19\n";

				if (client.init1G(19)<0){ return -1; };//試合前の初期化
				client.setHandicapStones();//置き石情報受け取り
				client.setTimeLimit(180000, 0);
			}
			else if (strstr(tok, "Match [9x9]")){

				if (fTurn == 1){
					snprintf(cmd_send, sizeof(cmd_send), "match %s W 9 30 0\n", ops_id.c_str());
					sendCommand(cmd_send);	// 白番の場合に、試合を受ける
				}

				CERR << MY_NAME << " : initialized board_size 9\n";

				if (client.init1G(9)<0){ return -1; };//試合前の初期化
				client.setHandicapStones();//置き石情報受け取り
				client.setTimeLimit(180000, 0);
				clms.start();//時間計測開始
			}
			else if (fTurn == 0 && strstr(tok, "accepted.")){// 白が応じたので初手を送れる。

				clms.start();

				myMove = client.play(BLACK);
				client.recvPlay(BLACK, myMove, clms.stop());
				NNGS::sendPlay_nngs(client.boardLength, myMove);
				clms.start();

			}else if (strstr(tok, "Illegal")){
				break;	// Errorの場合
			}else if (strstr(tok, "You can check your score")){// Passが連続した場合に来る(PASSした後の相手からのPASSは来ない）
				sendCommand("done\n");
			}else if (strstr(tok, "9 {Game") && strstr(tok, "resigns.")) {
				snprintf(cmd_send, sizeof(cmd_send), "%s vs %s", my_id.c_str(), ops_id.c_str());
				if (strstr(tok, cmd_send))break;	// game end
				snprintf(cmd_send, sizeof(cmd_send), "%s vs %s", ops_id.c_str(), my_id.c_str());
				if (strstr(tok, cmd_send))break;	// game end
			}
			else if (strstr(tok, "forfeits on time") && strstr(tok, my_id.c_str()) && strstr(tok, ops_id.c_str())) {
				//どちらかの時間切れ
				break;
			}
			else{
				sprintf(cmd_send, "{%s has disconnected}", ops_id.c_str());
				if (strstr(tok, ops_id.c_str())){
					break;//通信切断
				}
				snprintf(cmd_send, sizeof(cmd_send),"(%s): ", stone_str[1 - fTurn]);// 相手の手を受信する。
				char *p = strstr(tok, cmd_send);
				if (p != NULL) {// 通常の手を受信
					p += 5;
					if (strstr(p, "Pass")) {
						printf("相手=PASS!¥n");
						z = Z_PASS;
					}else{
						x = *p;
						y = atoi(p + 1);
						printf("相手=%c%d¥n", x, y);
						x = x - 'A' - (x > 'I') + 1;// 'I'を超えたら -1
					}
					client.recvPlay((fTurn == 0) ? WHITE : BLACK, XYtoZ(x, y), clms.stop());

					clms.start();//自分の時間計測開始

					int myCol = (fTurn == 1) ? WHITE : BLACK;
					myMove = client.play(myCol);

					client.recvPlay(myCol, myMove, clms.stop());
					NNGS::sendPlay_nngs(client.boardLength, myMove);
					clms.start();//相手の時間計測開始
					//待ち状態に入る
					client.wait();
				}
			}
			out_ss();
			searchToken(cmd_recv, NBytesRead, &c);
		}
	}
	CERR << "all games were finished.\n";
	out_ss();

	// ソケットを閉じる
#ifdef _WIN32
	closesocket(NetworkInfo.dstSocket);
	WSACleanup();
#else
	close(NetworkInfo.dstSocket);
#endif

	return 0;
}



