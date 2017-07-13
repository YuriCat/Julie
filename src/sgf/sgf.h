// SGFログを保存する構造体

#ifndef GO_SGF_H_
#define GO_SGF_H_

#include "../go.hpp"

namespace Go{
    /*
     struct SGFMinLog{
     int SZ;
     SGFPlayLog play[1024];
     
     int length()const noexcept{ return SZ; }
     const std::string& player(Color c)const{
     if(c == BLACK){
     }
     };
     };
     */
    namespace SGF{
        class SGFLog{
        public:
            constexpr static int N_PLAYS_ = getNTurnsMax(19);
            constexpr static int BOARD_SIZE_ = getBoardSize(19);
            
            constexpr static int max_plays()noexcept{ return N_PLAYS_; }
            constexpr static int boardSize()noexcept{ return BOARD_SIZE_; }
            
            int GM, FF, SZ;
            std::string CA;
            //std::string PW, PB;
            int WR, BR;
            int RU;
            float KM;
            
            int handicaps;
            int plays;
            short handicap[BOARD_SIZE_];
            short play[N_PLAYS_];
            
            void clear()noexcept{
                handicaps = plays = 0;
                CA = "";
                //PW = PB = "default";
                WR = BR = 0;
                GM = 1;
                FF = 4;
                SZ = 19;
                RU = JAPANESE;
                KM = 6.5;
            }
            
            int length()const noexcept{ return SZ; }
            /*
            std::string player(Color c)const{
                if(c == BLACK){
                    return PB;
                }else if(c == WHITE){
                    return PW;
                }else{
                    return "";
                }
            }*/
            
            std::string toString()const{
                std::ostringstream oss;
                oss << SZ << " " << RU << " " << KM << " ";
                //oss << PW << " " << PB
                oss << " " << WR << " " << BR << " ";
                oss << plays;
                for(int t = 0; t < plays; ++t){
                    oss << " " << play[t];
                }
                return oss.str();
            }
        };
        
        template<class accessor_t, typename callback_t>
        int readSGFLog(std::ifstream& ifs, accessor_t *const pdb,
                       const callback_t& callback = [](const auto& game)->bool{ return true; }){
            using log_t = typename accessor_t::value_type;
            log_t tmp;
            int games = 0;
            while(ifs){
                ifs >> tmp.SZ >> tmp.RU >> tmp.KM;
                if(!ifs){ break; }
                //ifs >> tmp.PW >> tmp.PB;
                ifs >> tmp.WR >> tmp.BR;
                ifs >> tmp.plays;
                tmp.plays = min(tmp.plays, tmp.max_plays()); // ターン数オーバーへの対応
                for(int t = 0; t < tmp.plays; ++t){
                    ifs >> tmp.play[t];
                }
                if(callback(tmp)){
                    //cerr << tmp.toString() << endl;
                    pdb->push_back(tmp);
                    ++games;
                }
            }
            return games;
        }
        
        template<class accessor_t, typename callback_t>
        int readSGFLog(const std::string& fName, accessor_t *const pdb,
                       const callback_t& callback = [](const auto& game)->bool{ return true; }){
            std::ifstream ifs;
            ifs.open(fName, std::ios::in);
            if(!ifs){ return -1; }
            return readSGFLog(ifs, pdb, callback);
        }
        
        int RankStringtoInt(const std::string& str){
            int n = (int)str[0] - '0';
            if(str[1] != 'd'){
                n = -n;
            }
            return n;
        }
        
        int RuleStringtoInt(const std::string& str){
            if(str[0] == 'j' || str[0] == 'J'){
                return JAPANESE;
            }else{
                return CHINESE;
            }
        }
        
        int PositionStringtoInt(const std::string& str, int l){
            if(str == "tt" || str.size() == 0){
                return Z_PASS;
            }else{
                int ix = str[0] - 'a', iy = l - 1 - (str[1] - 'a');
                int x = IXtoX(ix), y = IYtoY(iy);
                //cerr << str << endl; cerr << Point(x, y) << endl; getchar();
                return XYtoZ(x, y);
            }
        }
    }
    
    template<int L>
    void setOriginalBoard(const SGF::SGFLog& log, EasyBoard<L> *const pbd){
        // 初期状態にセット
        pbd->init();
    }
    
    template<class board_t, class gameLog_t,
    class firstCallback_t, class playCallback_t, class lastCallback_t>
    void iterateGameLog(const gameLog_t& game,
                        const int sym,
                        const firstCallback_t& firstCallback = [](const board_t&)->void{},
                        const playCallback_t& playCallback = [](const board_t&, const int)->int{ return 0; },
                        const lastCallback_t& lastCallback = [](const board_t&)->void{}){
        // 盤の準備
        board_t bd;
        bd.init();
        setOriginalBoard(game, &bd);
        
        firstCallback(bd);
        for(int t = 0, tend = game.plays; t < tend; ++t){
            int ret = playCallback(bd, game.play[t]);
            if(ret == -2){
                return;
            }else if(ret == -1){
                break;
            }
            if(bd.move(bd.getTurnColor(), symmetryZ<board_t::length()>(game.play[t], sym)) != VALID){
                break;
            }
        }
        lastCallback(bd);
    }
}

#endif // GO_SGF_H_
