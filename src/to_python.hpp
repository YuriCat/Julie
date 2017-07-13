/*
 to_ython.hpp
 Katsuki Ohto
 */

// 囲碁
// python側との通信

#ifndef GO_TO_PYTHON_HPP_
#define GO_TO_PYTHON_HPP_

#include "go.hpp"
#include "go_image.hpp"

namespace Go{

    int sockToPython = -1;
    
    int makeConnectionToPython(const std::string& ip, int port = 3333){
        // open socket
        sockToPython = socket(AF_INET, SOCK_STREAM, 0);
        if (sockToPython < 0){
            cerr << "failed to open socket to python." << endl;
            return -1;
        }
        // sockaddr_in 構造体のセット
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_port = htons(port);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip);
        
        // 接続
        if (-1 == connect(sockToPython, (struct sockaddr *) &addr, sizeof(addr))){
            cerr << "failed to connect to python." << endl;
            return -1;
        }
        return -1;
    }
    
    template<class board_t>
    int calcCNNPolicy(const board_t& bd, double prob[IMAGE_SIZE]){
        uint64_t image[IMAGE_LENGTH][IMAGE_LENGTH];
        genBoardImage(bd, image); // 画像作成
        
        // 画像を文字列にする
        std::ostringstream oss;
        for(int gx = 0; gx < IMAGE_LENGTH; ++gx){
            for(int gy = 0; gx < IMAGE_LENGTH; ++gy){
                oss << image[gx][gy] << " ";
            }
        }
        
        // pythonに情報伝達
        char sbuffer[8192];
        memset(sbuffer, 0, sizeof(sbuffer));
        snprintf(sbuffer, sizeof(sbuffer), oss.str().c_str());
        int err = send(sockToPython, sbuffer, strlen(sbuffer) + 1, 0);
        
        // 結果が返ってくる
        char rbuffer[4096];
        memset(rbuffer, 0, 4096);
        int bytes = recv(sockToPython, rbuffer, 4096, 0);
        std::string str = std::string(rbuffer);
        std::vector<std::string> v = split(str, ' ')
        for(int z = 0; z < IMAGE_SIZE; ++i){
            prob[z] = atof(v[z].c_str());
        }
    }
    
}

#endif // GO_TO_PYTHON_HPP_