/*
 count_alpha_3x3.cc
 Katsuki Ohto
 */

# include "../julie/julie_go.hpp"

using namespace std;
using namespace Go;

constexpr int Z_CENTER = IXIYtoZ<3>(1, 1);

map<uint64_t, int> patternMap;
int cnt = 0;

uint64_t hash_table_stone_tmp[BOARD_ARRAY_SIZE_MAX][8] = {0};

Color typeToColor(int t){
    return (t < 4) ? BLACK : WHITE;
}

int countLiberty(BitArray64<4, 9> bd, int z, BitSet32 *const pflag){
    int iz = ZtoIZ<3>(z);
    int t = bd[iz];
    pflag->set(iz);
    int lib = 0;
    for(int i = 0; i < 4; ++i){
        const int tmpZ = z + dir4(i);
        int izz = ZtoIZ<3>(tmpZ);
        if(pflag->test(izz)){ continue; } // チェック済
        if(!isOnBoardZ<3>(tmpZ)){
            ++lib; // 隣が空なので呼吸点
        }else{
            
            int tt = bd[izz];
            if(tt == 0){
                pflag->set(izz);
                ++lib; // 隣が空なので呼吸点
            }else if(typeToColor(t) == typeToColor(tt)){
                // 隣が同じ色の石ならば連になるので、さらに探索
                lib += countLiberty(bd, tmpZ, pflag);
            }
        }
    }
    return lib;
}

bool check(BitArray64<4, 9> bd){
    // liberty(single)
    //cerr << bd << endl;
    for(int i = 0; i < 8; ++i){
        const int z = Z_CENTER + dir8(i);
        int t = bd.at(ZtoIZ<3>(z));
        if(t == 0){
            continue;
        }
        Color c = typeToColor(t);
        int lib = ((t - 1) % 3) + 1;
        
        BitSet32 flag = 0;
        //int max_lib = countLiberty(bd, z, &flag);
        int min_lib = 0;
        
        for(int ii = 0; ii < 4; ++ii){
            const int zz = z + dir4(ii);
            if(isOnBoardZ<3>(zz)){
                const int tt = bd.at(ZtoIZ<3>(zz));
                if(tt != 0){
                    if(c == typeToColor(tt) && t != tt){ // same string but different liberty
                        //cerr << ZtoIZ<3>(z) << ", " << ZtoIZ<3>(zz) << endl; getchar();
                        return false;
                    }
                }else{
                    min_lib++;
                }
            }
        }
        //if(lib > max_lib){ // too big liberty number
            //cerr << z << " : " << lib << " > " << max_lib << endl;
            //return false;
        //}
        if(lib < min_lib){
            return false;
        }
    }
    
    return true;
}

void count(uint64_t *const hash, int i){
    if(i == 8){
        if(check(hash[0])){
            patternMap[pickFrontByHashx8(hash)] = 1;
            //patternMap[hash[0]] = 1;
        }
        ++cnt;
        return;
    }
    for(int t = 0; t < 7; ++t){
        if(t){ procHashx8<3>(hash, t, Z_CENTER + dir8(i), hash_table_stone_tmp); }
        count(hash, i + 1);
        if(t){ unprocHashx8<3>(hash, t, Z_CENTER + dir8(i), hash_table_stone_tmp); }
    }
}

int main(){

    iterateAround8WithIndex(Z_CENTER, [](int i, int z){
        for(int t = 1; t < 7; ++t){
            hash_table_stone_tmp[z][t] = uint64_t(t) << (ZtoIZ<3>(z) * 4);
            hash_table_stone_tmp[z][t] = uint64_t(t) << (ZtoIZ<3>(z) * 4);
        }
    });
    
    uint64_t hash[8] = {0};
    //EasyBoard<3> bd;
    count(hash, 0);
    cerr << cnt << "," << patternMap.size() << endl;
    return 0;
}