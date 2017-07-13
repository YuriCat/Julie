// sgf形式のログファイル読み込み

#ifndef GO_SGF_SGFREADER_HPP_
#define GO_SGF_SGFREADER_HPP_

#include <dirent.h>
#include <fstream>
//#include <locale>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "../go.hpp"
#include "sgf.h"

using namespace std;
using boost::lexical_cast;
using boost::algorithm::split;

namespace Go{
    namespace SGF{
#define ToF(str) (atof((str).c_str()))
#define ToI(str) (atoi((str).c_str()))
        std::string extract(const std::string& str, int oi, int *const pni){
            std::string ret = "";
            *pni = oi;
            if(str[oi] == '['){
                int i = oi + 1;
                while(i < str.size() && str[i] != ']'){
                    ret += str[i];
                    ++i;
                }
                *pni = i + 1;
            }
            //cerr << "extracted = " << ret << endl;
            return ret;
        }
        int next(const std::string& str, int oi){
            int i = oi;
            while(i < str.size() && str[i] != ']'){
                ++i;
            }
            return i + 1;
        }

        template<class log_t>
        int readSGF(std::ifstream& ifs, log_t *const plog){
            
            //std::locale::global(std::locale("ja_JP.UTF-8"));
            
            using std::string;
            //using std::wstring;
            
            plog->clear();
            
            string str;
            string v = "";
            int i = 0;
            
            while(ifs){
                // 行ごとに読み込み
                if(getline(ifs, str)){
                    size_t pos = str.find("\r");
                    if(pos != string::npos){
                        str = str.erase(pos);
                    }
                    v += str;
                    //cerr << "str = " << str << endl;
                    //cerr << "v = " << v << endl;
                }
            }
            //cerr << "v = " << v << endl;
            
            while(i < v.size()){
                if(v[i] == ')'){
                    break;
                }else if(v[i] == ';'){
                    i += 1;
                }else if(v[i] == '('){
                    i += 1;
                }else if(v[i] == 'B' && v[i + 1] == '['){
                    plog->play[plog->plays++] = PositionStringtoInt(extract(v, i + 1, &i), plog->length());
                }else if(v[i] == 'W' && v[i + 1] == '['){
                    plog->play[plog->plays++] = PositionStringtoInt(extract(v, i + 1, &i), plog->length());
                }else if(v[i] == 'G' && v[i + 1] == 'M'){
                    plog->GM = ToI(extract(v, i + 2, &i));
                }else if(v[i] == 'F' && v[i + 1] == 'F'){
                    plog->FF = ToI(extract(v, i + 2, &i));
                }else if(v[i] == 'S' && v[i + 1] == 'Z'){
                    plog->SZ = ToI(extract(v, i + 2, &i));
                }else if(v[i] == 'C' && v[i + 1] == 'A'){
                    plog->CA = extract(v, i + 2, &i);
                }else if(v[i] == 'P' && v[i + 1] == 'W'){
                    //plog->PW =
                    extract(v, i + 2, &i);
                }else if(v[i] == 'P' && v[i + 1] == 'B'){
                    //plog->PB =
                    extract(v, i + 2, &i);
                }else if(v[i] == 'W' && v[i + 1] == 'R'){
                    plog->WR = RankStringtoInt(extract(v, i + 2, &i));
                }else if(v[i] == 'B' && v[i + 1] == 'R'){
                    plog->BR = RankStringtoInt(extract(v, i + 2, &i));
                }else if(v[i] == 'R' && v[i + 1] == 'U'){
                    plog->RU = RuleStringtoInt(extract(v, i + 2, &i));
                }else if(v[i] == 'K' && v[i + 1] == 'M'){
                    plog->KM = ToF(extract(v, i + 2, &i));
                }else{
                    // unknown command
                    i = next(v, i);
                }
                
            }
            return 0;
        }
        
        template<class log_t>
        int readSGF(const std::string& fName, log_t *const plog){
            std::ifstream ifs;
            ifs.open(fName, std::ios::in);
            if(!ifs){ return -1; }
            return readSGF(ifs, plog);
        }
        
        /*int readSGFMin(std::ifstream& ifs, log_t *const plog){
            
            using std::string;
            
            string str;
            string v = "";
            int i = 0;
            
            while(1){
                while(q.size() < 1000){
                    // コマンドを行ごとに読み込み
                    if(!getline(ifs, str,'\n')){
                        v += ")";
                        break;
                    }
                }
                if(v[i] == '('){
                    
                }else if(v[i] == 'G' && v[i + 1] == 'M'){
                    i += 2;
                }else if(v[i] == 'F' && v[i + 1] == 'F'){
                    i += 2;
                }else if(v[i] == 'S' && v[i + 1] == 'Z'){
                    plog->SZ = ToI(extract(v, i + 2, &i));
                }else if(v[i] == 'P' && v[i + 1] == 'W'){
                    plog->PW = extract(v, i + 2, &i);
                }else if(v[i] == 'P' && v[i + 1] == 'B'){
                    plog->PB = extract(v, i + 2, &i);
                }else if(v[i] == 'W' && v[i + 1] == 'R'){
                    plog->WR = ToI(extract(v, i + 2, &i));
                }else if(v[i] == 'B' && v[i + 1] == 'R'){
                    plog->BR = ToI(extract(v, i + 2, &i));
                }
            }
        }*/
#undef ToI
#undef ToF
#undef ToV
#undef Get
    }
}

#endif // GO_SGF_SGFREADER_HPP_
