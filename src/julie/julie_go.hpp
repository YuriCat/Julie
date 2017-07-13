// 囲碁
// julie思考部ヘッダ

#ifndef GO_JULIE_GO_HPP_
#define GO_JULIE_GO_HPP_

#include "../go.hpp"

namespace Go{
    
    /**************************石のbitboard**************************/
    
    template<int L>
    struct BitBoard{
    public:
        constexpr static int BITS_ = (L + 2) * (L + 2);
        constexpr static int SIZE_ = (BITS_ - 1) / 64 + 1;
        
        constexpr static int size()noexcept{ return SIZE_; }
        
        BitSet64 bs_[SIZE_];
        
        bool operator[](int z)const{
            return bs_[z / 64].get(z % 64);
        }
        void set(int z){
            bs_[z / 64].set(z % 64);
        }
        void flip(int z){
            bs_[z / 64].flip(z % 64);
        }
        uint64_t test(int z)const{
            return bs_[z / 64].test(z % 64);
        }
        void reset(int z){
            bs_[z / 64].reset(z % 64);
        }
        void reset()noexcept{
            for(int i = 0; i < SIZE_; ++i){
                bs_[i] = 0;
            }
        }
    };
    
    template<int L, class callback_t>
    void iterate(const BitBoard<L>& bb, const callback_t& callback){
        int base = 0;
        for(int i = 0; i < bb.size(); ++i){
            iterate(bb.bs_[i], [&callback, base](int n)->void{
                callback(base + n);
            });
            base += 64;
        }
    }
    
    /**************************各スレッドの持ち物**************************/
    
    struct ThreadTools{
        using dice64_t = XorShift64;
        using move_t = Move;
        
        dice64_t dice; // サイコロ
        
        // 着手生成バッファ
        constexpr static int n_buf_len = 8192;
        move_t buf[n_buf_len];
        
        ThreadTools(){
            memset(buf, 0, sizeof(buf));
        }
    };
    
    /**************************設定ファイルの読み込み**************************/
    
    std::string DIR_PARAMS_IN(""), DIR_PARAMS_OUT(""), DIR_LOG("");
    
    int readConfigFile(){
#if defined(BUILD_VERSION)
        const std::string configFileName = "config" + MY_VERSION + ".txt";
#else
        const std::string configFileName = "config.txt";
#endif
        std::ifstream ifs;
        
        ifs.open(configFileName);
        
        if(!ifs){
            cerr << "failed to open config file." << endl; return -1;
        }
        ifs >> DIR_PARAMS_IN;
        ifs >> DIR_PARAMS_OUT;
        ifs >> DIR_LOG;
        
        ifs.close();
        return 0;
    }
    
    struct ConfigReader{
        ConfigReader(){
            readConfigFile();
        }
    };
    
    ConfigReader configReader;
}

#endif // GO_JULIE_GO_HPP_