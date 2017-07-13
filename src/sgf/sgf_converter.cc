 // sgf形式のファイルをSFGLog形式に変換して書き出し

#include <dirent.h>
#include <fstream>

#include "../go.hpp"
#include "sgfReader.hpp"

using namespace std;
using boost::lexical_cast;
using boost::algorithm::split;

std::string PATH_SGF = "/Users/ohto/Documents/data/go/sgf/42784games_nojp/";
std::string PATH_SGFLOG = "/Users/ohto/Documents/data/go/sgflog/";

namespace Go{
    namespace SGF{

        int makeSGFLogFile(){
            
            vector<string> sgfFileName;
            //string outFileName = PATH_SGFLOG + "sgflog.dat";
            //string outFileName = PATH_SGFLOG + "sgflog_mini.dat";
            string outFileName = PATH_SGFLOG + "sgflog_g100.dat";
            //string outFileName = PATH_SGFLOG + "sgflog_g1000.dat";
            
            // get file name
            string path = PATH_SGF;
            
            // DERR<<path<<endl;
            
            DIR *pdir;
            dirent *pentry;
            
            pdir = opendir(path.c_str());
            if (pdir == nullptr){
                return -1;
            }
            do {
                pentry = readdir(pdir);
                if (pentry != nullptr){
                    sgfFileName.emplace_back(path + string(pentry->d_name));
                }
            } while (pentry != nullptr);
            
            ofstream ofs;
            ofs.open(outFileName, ios::out);
            ofs.close();
            
            int cnt = 0;
            for (int i = 0; i < sgfFileName.size(); ++i){
                if(sgfFileName.at(i).find(".sgf") != string::npos){
                    
                    cerr << sgfFileName.at(i) << endl;
                    
                    SGFLog log;
                    
                    if(readSGF<SGFLog>(sgfFileName.at(i), &log) != 0){ // 正常に読めなかった
                        continue;
                    }
                    
                    ofstream ofs;
                    ofs.open(outFileName, ios::app);
                    ofs << log.toString() << endl;
                    ofs.close();
                    ++cnt;
                    if(cnt == 100){ break; }
                }
            }
            return 0;
        }
    }
}

int main(){
    return Go::SGF::makeSGFLogFile();
}
