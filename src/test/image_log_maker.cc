/*
 image_log_maker.cc
 Katsuki Ohto
 */

// 盤面の画像ログを作る

#include "../julie/julie_go.hpp"
#include "../sgf/sgf.h"

#include "../julie/go_image.hpp"

using namespace std;

int main(int argc, char *argv[]){
    
    using namespace Go;
    using namespace Go::SGF;
    
    std::string DIR_SGFLOG = "/Users/ohto/documents/data/go/sgflog/";
    std::string DIR_IMAGELOG = "/Users/ohto/documents/data/go/imagelog/";
    
    std::string ipath = DIR_SGFLOG + "sgflog.dat";
    //std::string opath = DIR_IMAGELOG + "imagelog.bin";
    std::string opath = DIR_IMAGELOG + "imagelog.dat";
    
    int N = 1000000;
    
    for (int i = 1; i < argc; ++i){
        if(strstr(argv[i], "-i")){ // sgflog directory path
            ipath = string(argv[i + 1]);
        }else if(strstr(argv[i], "-n")){ // max numbers of log
            N = atoi(argv[i + 1]);
        }else if(strstr(argv[i], "-o")){ // destination path
            opath = string(argv[i + 1]);
        }
    }
    
    return makeImageLog(ipath, opath, N);
}
