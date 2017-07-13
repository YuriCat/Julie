/*
 go_image.hpp
 Katsuki Ohto
 */

#ifndef GO_JULIE_GO_IMAGE_HPP_
#define GO_JULIE_GO_IMAGE_HPP_

#include "../go.hpp"

namespace Go{
    constexpr int IMAGE_PADDING = 2;
    constexpr int IMAGE_LENGTH = 19 + IMAGE_PADDING * 2;
    constexpr int IMAGE_PLAINS = 40;
    constexpr int IMAGE_SIZE = IMAGE_LENGTH * IMAGE_LENGTH;
    
    namespace Image{
        enum{
            OUT = 0, // 盤外
            MINE, // 自分の石
            OPP, //  相手の石
            SENSIBLENESS, // 合法(かつ眼でない)
            LAST1, // 前の手
            LAST2, // 前の前の手
            KO, // 現時点でコウ
            AFTERKO, // 打ったらコウになる
            LIBERTY = 8,
            AFTERLIBERTY = 16,
            CAPTURE = 24,
            SELFATARI = 32,
        };
    }
    
    struct ImageLog{
        int64_t mv;
        uint64_t img[IMAGE_LENGTH][IMAGE_LENGTH];
        
        void clear()noexcept{
            mv = 0;
            memset(img, 0, sizeof(img));
        }
        
        std::string toString()const{
            std::ostringstream oss;
            oss << mv;
            for(int x = 0; x < IMAGE_LENGTH; ++x){
                for(int y = 0; y < IMAGE_LENGTH; ++y){
                    oss << ' ' << img[x][y];
                }
            }
            return oss.str();
        }
    };
    
    /*template<class image_t>
    void amplifyImage(const image_t& img, image_t *const amp){
        // 対称な画像を生成
        for(int px = 0; px < IMAGE_LENGTH; ++px){
            for(int py = 0; py < IMAGE_LENGTH; ++py){
                uint64_t val = img.img[px][py];
                amp[0].img[IMAGE_LENGTH - 1 - px][py] = val;
                amp[1].img[px][IMAGE_LENGTH - 1 - py] = val;
                amp[2].img[IMAGE_LENGTH - 1 - px][IMAGE_LENGTH - 1 - py] = val;
                amp[3].img[py][px] = val;
                amp[4].img[IMAGE_LENGTH - 1 - py][px] = val;
                amp[5].img[py][IMAGE_LENGTH - 1 - px] = val;
                amp[6].img[IMAGE_LENGTH - 1 - py][IMAGE_LENGTH - 1 - px] = val;
            }
        }
    }

    void amplifyMove(const int mv, int *const amp){
        // 対称な着手位置を生成
        int px = mv / IMAGE_LENGTH;
        int py = mv % IMAGE_LENGTH;
        auto Foo = [](int x , int y)->int{ return x * IMAGE_LENGTH + y; };
        amp[0] = Foo(IMAGE_LENGTH - 1 - px, py);
        amp[1] = Foo(px, IMAGE_LENGTH - 1 - py);
        amp[2] = Foo(IMAGE_LENGTH - 1 - px, IMAGE_LENGTH - 1 - py);
        amp[3] = Foo(py, px);
        amp[4] = Foo(IMAGE_LENGTH - 1 - py, px);
        amp[5] = Foo(py, IMAGE_LENGTH - 1 - px);
        amp[6] = Foo(IMAGE_LENGTH - 1 - py, IMAGE_LENGTH - 1 - px);
    }*/
    
    
    template<class board_t>
    void genBoardImage(const board_t& bd, uint64_t img[IMAGE_LENGTH][IMAGE_LENGTH]){
        
        constexpr int L = 19;
        
        for(int x = 0; x < IMAGE_LENGTH; ++x){
            for(int y = 0; y < IMAGE_LENGTH; ++y){
                BitSet64 bs = 0;
                //bs.set(Image::ONE); // 全1
                bs.set(Image::OUT); // 盤外
                img[x][y] = bs.data();
            }
        }
        for(int ix = 0; ix < L; ++ix){
            for(int iy = 0; iy < L; ++iy){
                int gx = ix + IMAGE_PADDING, gy = iy + IMAGE_PADDING;
                BitSet64 bs = img[gx][gy];
                
                Color myColor = bd.getTurnColor();
                Color oppColor = flipColor(myColor);
                int myCI = toColorIndex(myColor);
                int oppCI = toColorIndex(oppColor);
                
                // 色
                Color c = bd.color(IXtoX(ix), IYtoY(iy));
                if(c == myColor){
                    bs.set(Image::MINE);
                }else if(c == oppColor){
                    bs.set(Image::OPP);
                }
                
                bs.reset(Image::OUT); // 盤外フラグを消す
                
                int z = IXIYtoZ<L>(ix, iy);
                
                if(bd.check(myColor, z) == VALID){
                    bs.set(Image::SENSIBLENESS);
                }
                
                board_t tbd;
                if(bd.color(z) != EMPTY){ // 石がある
                    int lib = bd.countLiberty(z);
                    if(lib > 0){
                        bs.set(Image::LIBERTY + min(7, lib - 1)); // 呼吸点
                    }
                }else{ // 空点
                    board_t tbd = bd;
                    int mcap = bd.captured[myCI];
                    if(isValid(tbd.move(myColor, z))){ // 眼でも要素を計算
                        // after ko
                        if(tbd.koZ != Z_PASS){ // 打ったらコウになる
                            bs.set(Image::AFTERKO);
                        }
                        // capture
                        int mcap2 = tbd.captured[myCI];
                        int mcapdist = mcap2 - mcap; // 取った石の数
                        if(mcapdist > 0){
                            bs.set(Image::CAPTURE + min(7, mcapdist - 1)); // 取れる相手の石の数
                        }
                        // after liberty, self atari
                        int lib, str;
                        tbd.countLibertyAndString(z, &lib, &str);
                        if(lib > 0){ // 呼吸点あり
                            bs.set(Image::AFTERLIBERTY + min(7, lib - 1));
                        }
                        if(lib == 1 && str > 0){ // 呼吸点1つなので取られる
                            bs.set(Image::SELFATARI + min(7, str - 1));
                        }
                    }
                }
                img[gx][gy] = bs.data();
            }
        }
        if(bd.lastZ != Z_PASS && bd.lastZ == Z_RESIGN){ // 以前の着手位置
            int ix = ZtoIX(bd.lastZ), iy = ZtoIY(bd.lastZ);
            int gx = ix + IMAGE_PADDING, gy = iy + IMAGE_PADDING;
            img[gx][gy] |= (1ULL << Image::LAST1);
        }
        if(bd.koZ != Z_PASS){ // 現時点でコウ
            int ix = ZtoIX(bd.lastZ), iy = ZtoIY(bd.lastZ);
            int gx = ix + IMAGE_PADDING, gy = iy + IMAGE_PADDING;
            img[gx][gy] |= (1ULL << Image::KO);
        }
        
        /*
         for(int x = 0; x < IMAGE_LENGTH; ++x){
         for(int y = 0; y < IMAGE_LENGTH; ++y){
         cerr << img[x][y] << " ";
         }
         }cerr << endl;*/
    }
    
    int saveImageLog(std::vector<ImageLog>& ilv, int lcnt, int BATCH, const std::string& opath){
        
        // 文字列で保存
        int tlcnt = 0;
        for(; (tlcnt + 1) * BATCH <= ilv.size(); ++tlcnt){
            // 1つファイルを作る
            std::ofstream ofs;
            std::ostringstream oss;
            oss << opath << (lcnt + tlcnt) << ".dat";
            ofs.open(oss.str(), std::ios::out);
            for(int i = 0; i < BATCH; ++i){
                ofs << ilv[tlcnt * BATCH + i].toString() << endl;
            }
            ofs.close();
        }
        ilv.clear();
        return tlcnt;
    }
    
    int makeImageLog(const std::string& ipath, const std::string& opath, int N){
        
        constexpr int L = 19;
        constexpr int BATCH = 256;
        
        std::random_device seed;
        std::mt19937 mt(seed() ^ 12345);
        
        std::vector<SGF::SGFLog> slv;
        std::vector<ImageLog> ilv;
        //std::array<ImageLog, BATCH> *pila = new std::array<ImageLog, BATCH>;
        int lcnt = 0;
        //int nila = 0;
        
        if(SGF::readSGFLog(ipath, &slv, [](const auto& sl)->bool{ return true; }) < 0){
            return -1;
        }
        
        cerr << slv.size() << " games were imported." << endl;
        
        for(int sym = 0; sym < 8; ++sym){
            for(const auto& sl : slv){
                if(sl.SZ != L){ continue; } // L路限定
                
                EasyBoard<L> bd;
                bd.init();
                
                // image-logに1局面ずつ保存
                for(int t = 0; t < sl.plays; ++t){
                    
                    ImageLog il;
                    
                    const int z0 = sl.play[t];
                    
                    if(z0 == Z_PASS){ // パス
                        break;
                    }

                    const int z = symmetryZ<L>(z0, sym);
                    
                    if(!isValid(bd.check(bd.getTurnColor(), z))){ // 非合法着手と判定された
                        break;
                    }
                    
                    //int iz = ZtoIZ<19>(sl.play[t]);
                    //int iz = 1;
                    
                    int ix = ZtoIX(z);
                    int iy = ZtoIY(z);
                    
                    genBoardImage(bd, il.img);
                    il.mv = (ix + IMAGE_PADDING) * (L + 2 * IMAGE_PADDING) + iy + IMAGE_PADDING;
                    
                    ilv.push_back(il);
                    
                    //(*pila)[cnt] = il;
                    // 8方向で対称な画像を生成
                    /*ImageLog il7[8 - 1];
                     int mv7[8 - 1];
                     amplifyImage(il, il7);
                     amplifyMove(il.mv, mv7);
                     
                     for(int i = 0; i < 7; ++i){
                     il7[i].mv = mv7[i];
                     ilv.push_back(il7[i]);
                     }*/
                    if(ilv.size() >= 4000000){
                        std::shuffle(ilv.begin(), ilv.end(), mt);
                        lcnt += saveImageLog(ilv, lcnt, BATCH, opath);
                    }
                    
                    bd.move<_YES>(bd.getTurnColor(), z);
                }
            }
        }
        
    END:
        std::shuffle(ilv.begin(), ilv.end(), mt);
        lcnt += saveImageLog(ilv, lcnt, BATCH, opath);
        
        //std::shuffle(pila->begin(), pila->begin() + cnt, mt);
        
        // バイナリ形式で保存
        /*FILE *pf = fopen(opath.c_str(), "wb");
         fwrite(pila, sizeof(ImageLog) * N, 1, pf);
         fclose(pf);*/
        
        
        
        //return cnt;
        return 0;
    }
}

#endif // GO_JULIE_GO_IMAGE_HPP_