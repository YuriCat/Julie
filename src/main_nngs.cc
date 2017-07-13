/*
 main_nngs.cc
 Katsuki Ohto
 */

// Julie
// NNGS対戦メイン
// katsuki Ohto

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

// 基本的定義、ユーティリティ
#include "../../common/common_all.h"

using std::string;
using std::stringstream;

// エンジンとしてのビルドの場合、cerrの出力先を変える
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


FILE* com_log; // 通信ログ
FILE* thinking_log; // 思考ログ

void out_ss(){
    if (com_log != nullptr){
        fprintf(com_log, "%s", ss.ss.str().c_str());
        ss.clear();
    }
}

// 通信プロトコルとの対応
int fTurn = 0;

const char *stone_str[2] = { "B", "W" };

int myMove;

#include "go.hpp"

using namespace Go;

template<class atomic_data_t>
struct ClientState{
    atomic_data_t st;
    
    uint32_t isPondering()const noexcept{ return st & 2U; }
    uint32_t isDecidingMove()const noexcept{ return st & 1U; }
    uint32_t requestedToStop()const noexcept{ return st & 4U; }
    uint32_t requestedToExit()const noexcept{ return st & 32U; }
    uint32_t requestedToPonder()const noexcept{ return st & 16U; }
    uint32_t requestedToDecideMove()const noexcept{ return st & 8U; }
    
    uint32_t isThinking()const noexcept{ return st & 3U; }
    
    void setPondering()noexcept{ st |= 2U; }
    void setDecidingMove()noexcept{ st |= 1U; }
    
    void requestToStop()noexcept{ st |= 4U; }
    void requestToPonder()noexcept{ st &= (~(4U | 32U)); st |= 16U; }
    void requestToDecideMove()noexcept{ st &= (~(4U | 32U)); st |= 8U; }
    void requestToExit()noexcept{ st |= 4U | 32U; }
    
    void resetRequestToPonder()noexcept{ st &= (~16U); }
    void resetRequestToDecideMove()noexcept{ st &= (~8U); }
    void resetPlay()noexcept{ st &= (4U | 32U); }
    
    void init()noexcept{ st = 0; }
};

ClientState<std::atomic<uint32_t>> gState;

#include "julie.hpp"

Julie::Client client;


int thinkThread(){
    //要求された思考が終了したら抜ける
    ClientState<uint32_t> tmp;
    tmp.st = gState.st;
    
    while (1){
        if (tmp.requestedToExit()){
            gState.init();
            return 0;
        }
        else if (tmp.requestedToStop()){
            gState.init();
            return 0;
        }
        else if (tmp.requestedToDecideMove()){
            CERR << "decidingMove" << endl;
            gState.setDecidingMove();
            gState.resetRequestToDecideMove();
            Color myColor = (fTurn == 1) ? WHITE : BLACK;
            myMove = client.play(myColor);
            gState.resetPlay();
        }
        else if (tmp.requestedToPonder()){
            CERR << "pondering" << endl;
            gState.setPondering();
            gState.resetRequestToPonder();
            Color myColor = (fTurn == 1) ? WHITE : BLACK;
            client.wait(flipColor(myColor));
            gState.resetPlay();
        }
        return 0;
    }
}

std::string toNNGSMoveString(int b_len, const int z){
    char str[16];
    int x, y;
    if (z == Z_RESIGN || z == Z_PASS){
        snprintf(str, sizeof(str), "pass");
    }
    else{
        y = ZtoY(z);
        x = ZtoX(z);
        x = 'A' + x - 1;
        if (x >= 'I') x++;	// 'I' は使わない。'A'-'H','J'-'T'までを使う。
        snprintf(str, sizeof(str), "%c%d", x, y);
    }
    return std::string(str);
}

int sendMessage(int sock, const std::string& msg){
    char sbuffer[256];
    memset(sbuffer, 0, sizeof(sbuffer));
    snprintf(sbuffer, sizeof(sbuffer), msg.c_str());
    if(strlen(sbuffer) > 0){
        //cerr << "Client << " << sbuffer;
    }
    int err = send(sock, sbuffer, strlen(sbuffer) + 1, 0);
    if(err < 0){
        CERR << "failed to send message(" << err << ")." << endl;
    }
    return err;
}

int main(int argc, char* argv[]){
    
    char cmd_recv[1024];
    int myMove;
    ClockMS clms;
    
    //基本的な初期化
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    
    const uint64_t time_limit = 1740000ULL;
    std::string ip = "127.0.0.1";
    std::string ops_id = "noName";
    int port = 9696;
    struct hostent *host = nullptr;
    
#ifdef NAME_ADD1
    std::string my_id = "julie1";
#else
    std::string my_id = "julie";
#endif
    
    // 起動オプションをチェック
    if (argc >= 2 && *argv[1] == 'W') fTurn = 1; // 白番
    if (argc >= 3) ip = std::string(argv[2]); // サーバIPを指定した場合
    if (argc >= 4) port = atoi(argv[3]); // ポート
    if (argc >= 5) ops_id = std::string(argv[4]); // 相手の名前
    if (argc >= 6){ // モード
        if(strstr(argv[4], "-py")){ // pythonとの通信
            
        }
    }
    
    
    CERR << "ip : " << ip << endl;
    CERR << "port : " << port << endl;
    CERR << "my_id : " << my_id << endl;
    CERR << "ops_id : " << ops_id << endl;
    
    gState.init();
    
    //クライアント初期化
    client.initMatch();
    CERR << MY_NAME << " : initialized for all games." << endl;
    
#ifdef _WIN32
    //Windows 独自の設定
    WSADATA data;
    if (SOCKET_ERROR == WSAStartup(MAKEWORD(2, 0), &data)){
        cerr << "failed to initialize WSA-data." << endl;
        return -1;
    }
#endif
    
    // open socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        cerr << "failed to open socket." << endl;
    }
    // sockaddr_in 構造体のセット
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    
    // IPアドレスを数値に変換
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        host = gethostbyname(ip.c_str()); // ホスト名から情報を取得してIPを調べる
        CERR << host << endl;
        if (host == NULL) {
            cerr << "failed to get host by name." << endl;
            return 1;
        }
        addr.sin_addr.s_addr = *((unsigned long *)((host->h_addr_list)[0]));
    }else {
        // host = gethostbyaddr((const char *)&addrIP, 4, AF_INET); // Localな環境では失敗する
    }
    
    // 接続
    if (-1 == connect(sock, (struct sockaddr *) &addr, sizeof(addr))){
        cerr << "failed to connect to server." << endl;
        return 1;
    }
    
    // ログイン
    sendMessage(sock, my_id + "\n");
    
    // 接続完了
    CERR << "connected to server." << endl;
    
    int x, y, z;
    
    int ingame = 0;
    
    // 通信
    int L = 8192;
    char *rbuffer = new char[L];
    memset(rbuffer, 0, L);
    std::string unfinished = "";
    
    while (1){
        
        if(strlen(rbuffer) <= 0){
            int bytes = recv(sock, rbuffer, L, 0);
            CERR << bytes << endl;
            if(bytes == 0){
                CERR << "disconnected..." << endl;
                exit(1);
            }else{
                while(bytes == L){
                    L = 2 * L;
                    char *nrbuffer = new char[L];
                    memset(nrbuffer, 0, L);
                    memcpy(nrbuffer, rbuffer, bytes);
                    delete[] rbuffer;
                    rbuffer = nrbuffer;
                    bytes += recv(sock, rbuffer + bytes, L - bytes, 0);
                    //cerr << L << endl;
                }
            }
        }
        
        std::string str = std::string(rbuffer);
        int len = strlen(rbuffer);
        DERR << len << endl;
        memmove(rbuffer, rbuffer + len + 1, L - len - 1);
        memset(rbuffer + L - len - 1, 0, len + 1);
        
        std::vector<std::string> commands = split(str, '\n');
        DERR << commands.size() << endl;
        
        for(std::string& command : commands){
            if(command.size() <= 0){ continue; }
            
            CERR << "Server >> " << command << endl;
            out_ss();
            
            if (contains(command, "No Name Go Server")){
                sendMessage(sock, "set client TRUE\n"); // シンプルな通信モードに
            }
            if (contains(command, "Set client to be True")){
                if (fTurn == 0 && ingame == 0){
                    ingame = 1;
                    sendMessage(sock, "match " + ops_id + " B 19 30 0\n"); // 黒番の場合に、試合を申し込む
                    
                    CERR << MY_NAME << " : initialized board_size 19\n";
                    
                    if (client.initGame(19) < 0){ return -1; }; // 試合前の初期化
                    client.setHandicapStones(); // 置き石情報受け取り
                    client.setTimeLimit(time_limit, 0);
                }
            }
            if (contains(command, "Match [19x19]")){
                
                if (fTurn == 1 && ingame == 0){
                    ingame = 1;
                    sendMessage(sock, "match " + ops_id + " W 19 30 0\n"); // 白番の場合に、試合を受ける
                    
                    CERR << MY_NAME << " : initialized board_size 19\n";
                    
                    if (client.initGame(19) < 0){ return -1; }; // 試合前の初期化
                    client.setHandicapStones(); // 置き石情報受け取り
                    client.setTimeLimit(time_limit, 0);
                    clms.start(); // 時間計測開始
                }
                
            }
            if (contains(command, "Match [9x9]")){
                
                if (fTurn == 1 && ingame == 0){
                    ingame = 1;
                    sendMessage(sock, "match " + ops_id + " W 9 30 0\n"); // 白番の場合に、試合を受ける
                    
                    CERR << MY_NAME << " : initialized board_size 9\n";
                    
                    if (client.initGame(9) < 0){ return -1; }; // 試合前の初期化
                    client.setHandicapStones(); // 置き石情報受け取り
                    client.setTimeLimit(time_limit, 0);
                    clms.start(); // 時間計測開始
                }
                
            }
            if (fTurn == 0 && contains(command, "accepted.")){ // 白が応じたので初手を送る
                
                CERR << "BLACK shote." << endl;
                
                clms.start();
                gState.setDecidingMove();
                Color myCol = (fTurn == 1) ? WHITE : BLACK;
                myMove = client.play(myCol);
                gState.resetPlay();
                
                CERR << "MY MOVE : " << z << " " << Point(myMove) << endl;
                
                client.recvPlay(BLACK, myMove, clms.stop());
                sendMessage(sock, toNNGSMoveString(client.boardLength, myMove) + "\n"); // 手を送る
                
                clms.start();
                
                gState.requestToPonder();
                std::thread think(thinkThread);
                think.detach();
            }
            if (contains(command, "Illegal")){
                break;
            }
            if (contains(command, "You can check your score")){ // Passが連続した場合に来る(PASSした後の相手からのPASSは来ない）
                sendMessage(sock, "done\n");
                gState.requestToExit();
                goto LAST;
            }
            if (contains(command, "9 {Game") && contains(command, "resigns.")) {
                if (contains(command, my_id + " vs " + ops_id)){ gState.requestToExit(); return 0; }	// game end
                if (contains(command, ops_id + " vs " + my_id)){ gState.requestToExit(); return 0; }	// game end
            }
            if (contains(command, "forfeits on time") && contains(command, my_id) && contains(command, ops_id)) {
                //どちらかの時間切れ
                gState.requestToExit();
                goto LAST;
            }
            {
                if (contains(command, "{" + ops_id + " has disconnected}")){
                    gState.requestToExit();
                    goto LAST; //通信切断
                }
                char tmp[1024];
                snprintf(tmp, sizeof(tmp), "(%s): ", stone_str[1 - fTurn]); // 相手の手を受信する。
                const char *p = strstr(command.c_str(), tmp);
                
                //CERR << "pointer" << (uint64_t)p << endl;
                
                if (p != nullptr) { // 通常の手を受信
                    
                    if (gState.isThinking()){
                        gState.requestToStop(); // 思考終了命令を出す
                        while (gState.isThinking()){}
                        CERR << "finished pondering" << endl;
                    }
                    gState.init();
                    
                    p += 5;
                    if (strstr(p, "Pass") || strstr(p, "pass")) {
                        cerr << "pass!" << endl;
                        z = Z_PASS;
                    }
                    else{
                        x = *p;
                        y = atoi(p + 1);
                        x = x - 'A' - (x > 'I') + 1; // 'I'を超えたら -1
                        z = XYtoZ(x, y);
                    }
                    
                    client.recvPlay((fTurn == 0) ? WHITE : BLACK, z, clms.stop());
                    
                    clms.start(); // 自分の時間計測開始
                    
                    gState.setDecidingMove();
                    int myCol = (fTurn == 1) ? WHITE : BLACK;
                    myMove = client.play(myCol);
                    gState.resetPlay();
                    
                    if(myMove == Z_PASS){
                        CERR << "MY MOVE : Pass (" << myMove << ")" << endl;
                    }else{
                        CERR << "MY MOVE : " << Point(myMove) << " (" << myMove << ")" << endl;
                    }
                    
                    client.recvPlay((fTurn == 1) ? WHITE : BLACK, myMove, clms.stop());
                    
                    sendMessage(sock, toNNGSMoveString(client.boardLength, myMove) + "\n"); // 手を送る
                    
                    clms.start(); // 相手の時間計測開始
                    // 待ち状態に入る
                    
                    gState.init();
#ifdef PONDER
                    // 先読みありの場合
                    gState.requestToPonder();
                    std::thread think(thinkThread);
                    think.detach();
#endif
                }
            }
            out_ss();
        }
    }
LAST:
    
    CERR << "score2 : " << client.countScore2() << endl;
    CERR << "all games were finished.\n";
    out_ss();
    
    // ソケットを閉じる
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    
    return 0;
}