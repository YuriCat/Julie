/*
 monteCarlo.h
 Katsuki Ohto
 */

// 囲碁
// モンテカルロ関連ヘッダ

#ifndef GO_JULIE_MC_MONTECARLO_H_
#define GO_JULIE_MC_MONTECARLO_H_

#include "../julie_go.hpp"

namespace Go{
    namespace Julie{
        
        constexpr int MC_LEAF_ORIGINAL_TRIALS = 2; // 先局面での着手初期トライ回数
        constexpr int MC_ROOT_ORIGINAL_TRIALS = 2; // ルート局面での着手初期トライ回数
        //constexpr int MC_ROOT_MIN_TRIALS = 8; // ルートでの最低トライ回数
        
        template<class node_t>
        struct Child{
            //一般
            int z;
            uint32_t trials;
            float score;
            uint32_t trialsAMAF;
            float scoreAMAF;
            
            float polScore; // 方策点
            node_t *pnext;
            uint64_t info; // 0~1 mateColor
            
            void init()noexcept{
                trials = MC_LEAF_ORIGINAL_TRIALS;
                score = MC_LEAF_ORIGINAL_TRIALS * 0.5;
                trialsAMAF = MC_LEAF_ORIGINAL_TRIALS;
                scoreAMAF = MC_LEAF_ORIGINAL_TRIALS * 0.5;
                pnext = nullptr;
                info = 0;
            }
            void set(int az)noexcept{
                init();
                z = az;
            }
            void feed(double sc)noexcept{
                trials += 1;
                score += sc;
            }
            void feedAMAF(double sc)noexcept{
                trialsAMAF += 1;
                scoreAMAF += sc;
            }
            bool any()const noexcept{ return (z >= Z_PASS); }
            bool isPass()const noexcept{ return (z == Z_PASS); }
            
            double mean()const noexcept{ return score / (double)trials; }
            double meanAMAF()const noexcept{ return scoreAMAF / (double)trialsAMAF; }
        };
        
        template<int L>
        struct Node : public AtomicNode<NodeLock<uint64_t>, 1>{
            // 一般
            uint32_t trials;
            uint32_t trialsAMAF;
            int childs;
            int age;
            int turn;
            
            Child<Node> child[L * L + 1];
            
            void clear()noexcept{
                trials = 0;
                trialsAMAF = 0;
                childs = 0;
            }
            
            template<class board_t>
            void set(const board_t& bd, uint64_t ahash, int ag){
                clear();
                // 飛び飛びで合法手生成
                childs = genAllMovesWithoutEye(child, bd.getTurnColor(), bd);
                trials = childs * MC_LEAF_ORIGINAL_TRIALS;
                turn = bd.turn;
            }
            
            template<class result_t>
            void feed(int i, const result_t& res){
                child[i].feed(res.result(getTurnColor()));
            }
            
            int chooseBestUCB()const{
                constexpr double C = 0.2;
                constexpr double K = 100;
                constexpr double M = 3;
                
                constexpr double W1 = 1 / 0.9;
                constexpr double W2 = 1 / 20000;
                
                const double beta = (L == 9) ? sqrt(K / (K + M * trials))
                : (trialsAMAF / (trialsAMAF + trials * (W1 + W2 * trialsAMAF)));
                
                int best = -1;
                double bestUCB = -9999;
                
                if (isPluged()){ // 候補着手が詰まっている
                    for (int i = 0; i < getNChilds(); ++i){
                        const auto& ch = child[i];
                        ASSERT(ch.trials > 0, cerr << getNChilds() << endl;);
                        double ucb = ch.mean() + C * sqrt(log(trials) / ch.trials);
                        double rave = ch.meanAMAF() + C * sqrt(log(trialsAMAF) / ch.trialsAMAF);
                        double ucbRave = (1 - beta) * ucb + beta * rave;
                        
                        if (ucb > bestUCB){
                            best = i;
                            bestUCB = ucb;
                        }
                    }
                    ASSERT(0 <= best && best < getNChilds(),);
                    ASSERT(child[best].isPass() || isOnBoardZ<L>(child[best].z),
                           cerr << best << " " << child[best].z << endl;);
                }
                else{
                    for (int i = 0; i < L * L + 1; ++i){
                        const auto& ch = child[i];
                        if (ch.any()){
                            ASSERT(ch.trials > 0, cerr << getNChilds() << endl;);
                            double ucb = ch.mean() + C * sqrt(log(trials) / ch.trials);
                            double rave = ch.meanAMAF() + C * sqrt(log(trialsAMAF) / ch.trialsAMAF);
                            double ucbRave = (1 - beta) * ucb + beta * rave;
                            
                            if (ucbRave > bestUCB){
                                best = i;
                                bestUCB = ucbRave;
                            }
                        }
                    }
                    ASSERT(0 <= best && best < L * L + 1,);
                    ASSERT(child[best].isPass() || isOnBoardZ<L>(child[best].z),
                           cerr << best << " " << child[best].z << endl;);
                }
                return best;
            }
            
            bool isPluged()const noexcept{ return true; }
            int getNChilds()const noexcept{ return childs; }
            int getAge()const noexcept{ return age; }
            int getTurn()const noexcept{ return turn; }
            Color getTurnColor()const noexcept{ return toTurnColor(getTurn()); }
        };
        
        template<int L, int RM, int _MEMORY>
        class MainTransPositionTable : public NodeTranspositionTable<Node<L>, (_MEMORY / sizeof(Node<L>)), RM>{
            // メイン置換表
        private:
            using base_t = NodeTranspositionTable<Node<L>, (_MEMORY / sizeof(Node<L>)), RM>;
            
        public:
            int oldestAge; // まだ消えていない最も古い年代
            int lastAge; // 最も新しく登録されたと思われる年代
            
            void init()noexcept{
                base_t::init();
                oldestAge = 1;
                lastAge = 0;
            }
            
            void setAge(int a)noexcept{
                lastAge = a;
            }
            
            void clearByTurn(int t)noexcept{
                // 手数が一定以前のものを消す
                int del = base_t::collect([t](const auto& node)->bool{
                    return (node.getTurn() <= t);
                });
                CERR << del << " pages were cleared by turn." << endl;
                }
                
                void clearOldest()noexcept{
                    // 最も昔に登録されたものを消す
                    if(oldestAge <= lastAge){
                        int oa = oldestAge;
                        int del = base_t::collect([oa](const auto& node)->bool{
                            return (node.getAge() <= oa);
                        });
                        CERR << del << " pages were cleared by age." << endl;
                        ++oldestAge;
                    }
                }
                };
                
                // 置換表実体 & 初期化
                MainTransPositionTable<9, 6, TT_SIZE> *MainTT9 = nullptr;
                MainTransPositionTable<13, 6, TT_SIZE> *MainTT13 = nullptr;
                MainTransPositionTable<19, 6, TT_SIZE> *MainTT19 = nullptr;
                
                template<int L>
                MainTransPositionTable<L, 6, TT_SIZE>* getMainTTPtr();
                
                template<int L>int initMainTT();
                template<int L>int closeMainTT();
                
                template<>MainTransPositionTable<9, 6, TT_SIZE>* getMainTTPtr<9>(){
                    return MainTT9;
                }
                template<>MainTransPositionTable<13, 6, TT_SIZE>* getMainTTPtr<13>(){
                    return MainTT13;
                }
                template<>MainTransPositionTable<19, 6, TT_SIZE>* getMainTTPtr<19>(){
                    return MainTT19;
                }
                
                template<>int initMainTT<9>(){
                    if(MainTT9 == nullptr){
                        MainTransPositionTable<9, 6, TT_SIZE> *tmp = new(MainTransPositionTable<9, 6, TT_SIZE>);
                        if(tmp == nullptr){ return -1; }
                        tmp->init();
                        MainTT9 = tmp;
                    }
                    return 0;
                }
                
                template<>int initMainTT<13>(){
                    if(MainTT13 == nullptr){
                        MainTransPositionTable<13, 6, TT_SIZE> *tmp = new(MainTransPositionTable<13, 6, TT_SIZE>);
                        if( tmp == nullptr ){ return -1; }
                        tmp->init();
                        MainTT13 = tmp;
                    }
                    return 0;
                }
                
                template<>int initMainTT<19>(){
                    if(MainTT19 == nullptr){
                        MainTransPositionTable<19, 6, TT_SIZE> *tmp = new(MainTransPositionTable<19, 6, TT_SIZE>);
                        if(tmp == nullptr){ return -1; }
                        tmp->init();
                        MainTT19 = tmp;
                    }
                    return 0;
                }
                
                template<>int closeMainTT<9>(){
                    if(MainTT9 == nullptr){ return -1; }
                    delete(MainTT9);
                    MainTT9 = nullptr;
                    return 0;
                }
                
                template<>int closeMainTT<13>(){
                    if(MainTT13 == nullptr){ return -1; }
                    delete(MainTT13);
                    MainTT13 = nullptr;
                    return 0;
                }
                
                template<>int closeMainTT<19>(){
                    if(MainTT19 == nullptr){ return -1; }
                    delete(MainTT19);
                    MainTT19 = nullptr;
                    return 0;
                }
                
                template<int L>
                struct RootChild{
                    // ルート専用
                    int z;
                    uint32_t flag;
                    uint32_t oldTrials;
                    std::atomic<uint32_t> trials;
                    double score;
                    double score2; // 2乗和
                    double linPolScore; // 線形の方策点
                    double cnnPolScore; // CNNによる方策点
                    
                    Node<L> *pnext; // 次局面
                    
                    static bool GreaterMean(const RootChild& left, const RootChild& right){
                        return left.mean() > right.mean();
                    }
                    
                    bool any()const noexcept{ return (z != Z_RESIGN); }
                    
                    void init()noexcept{
                        flag = 1U;
                        trials = MC_ROOT_ORIGINAL_TRIALS;
                        score = MC_ROOT_ORIGINAL_TRIALS * 0.5;
                        score2 = MC_ROOT_ORIGINAL_TRIALS * 0.5; // 全て0or1のとき
                    }
                    
                    void set(const int az)noexcept{
                        // 着手をセットし、初期化
                        init();
                        z = az;
                    }
                    void setPass(){
                        init();
                        z = Z_PASS;
                    }
                    void feed(const double sc)noexcept{
                        ++trials;
                        score += sc;
                        score2 += sc * sc;
                    }
                    
                    template<class child_t>
                    void importChild(const child_t& ach)noexcept{
                        // 通常のchild型から読み込む
                        z = ach.z;
                        flag = 1U;
                        oldTrials = ach.trials;
                        trials = ach.trials;
                        score = ach.mean() * ach.trials;
                        score2 = score;
                    }
                    
                    bool isPass()const noexcept{ return (z == Z_PASS); }
                    
                    double mean()const noexcept{
                        return score / trials;
                    }
                    double var()const noexcept{
                        return score2 / trials - pow(mean(), 2);
                    }
                };
                
                template<int L>
                class RootNode{
                private:
                    using node_t = Node<L>;
                    using rootChild_t = RootChild<L>;
                    
                public:
                    rootChild_t child[L * L + 1]; // 候補着手
                    int childs;
                    uint32_t oldTrials;
                    uint32_t trials;
                    double score; // 得点総計
                    Color col;
                    uint64_t limit; // 制限時間
                    int decidedZ;
                    
                    SpinLock<uint32_t> lock_;
                    
                    void unlock()noexcept{ lock_.unlock(); }
                    void lock()noexcept{ lock_.lock(); }
                    
                    bool isPluged()const noexcept{ return true; }
                    int getNChilds()const noexcept{ return childs; }
                    bool isWhite()const noexcept{ return (col == WHITE); }
                    
                    template<class board_t>
                    void set(const board_t& bd, Color acol){
                        const uint64_t hash = node_t::knitIdentityValue(bd.hash_stone_[0], toColorIndex(acol));
                        bool found = false;
                        
                        auto *const ptt = getMainTTPtr<L>();
                        node_t *pnode;
                        if(ptt != nullptr){
                            pnode = ptt->read(hash, hash, &found);
                        }else{
                            pnode = nullptr;
                        }
                        
                        oldTrials = 0;
                        
                        decidedZ = Z_RESIGN;
                        
                        if(pnode != nullptr && found){
                            //すでに現局面が登録されているので、インポートする
                            CERR << "imported!" << endl;
                            
                            if(pnode->isPluged()){
                                childs = pnode->getNChilds();
                                for(int i = 0; i < getNChilds(); ++i){
                                    const auto& ch = pnode->child[i];
                                    if(ch.any()){
                                        child[i].importChild(ch);
                                        trials += ch.trials;
                                        oldTrials += ch.trials;
                                        score += ch.score;
                                    }
                                }
                            }else{
                                int cnt = 0;
                                for(int i = 0; i < L * L; ++i){
                                    const auto& ch = pnode->child[i];
                                    if(ch.any()){
                                        child[cnt].importChild(ch);
                                        trials += ch.trials;
                                        oldTrials += ch.trials;
                                        score += ch.score;
                                        ++cnt;
                                    }
                                }
                                childs = cnt;
                            }
                            if(getNChilds() > 1 || !child[0].isPass()){ // パス以外ができるとき、パスを追加
                                child[childs].set(Z_PASS);
                                trials += MC_ROOT_ORIGINAL_TRIALS;
                                score += MC_ROOT_ORIGINAL_TRIALS * 0.5;
                                ++childs;
                            }
                        }else{
                            // 局面から候補着手を調べ、セットする
                            childs = genAllMoves(child, acol, bd);
                            CERR << "settled " << childs << " childs." << endl;
                            trials = MC_ROOT_ORIGINAL_TRIALS * childs;
                            score = trials * 0.5; // 半分勝ったと仮定
                        }
                        
                        if(getNChilds() == 0){ // 合法着手なし
                            decidedZ = Z_PASS; return;
                        }else if(getNChilds() == 0){ // 合法着手1つ
                            decidedZ = child[0].z; return;
                        }
                        
                        col = acol;
                        unlock();
                    }
                    
                    void setLimitTime(uint64_t alimit){
                        limit = alimit;
                    }
                    
                    template<class dice_t>
                    int chooseBestUCB(dice_t *const dice)const{
                        constexpr double C = 0.6;
                        
                        int best = -1;
                        double bestUCB = -9999;
                        
                        ASSERT(isPluged(),);
                        for (int i = 0; i< getNChilds(); ++i){
                            const auto& ch = child[i];
                            double ucb = ch.mean() + C * sqrt(log(trials) / ch.trials);
                            if(ucb > bestUCB){
                                best = i;
                                bestUCB = ucb;
                            }
                        }
                        ASSERT(0 <= best && best < getNChilds(),
                               cerr << getNChilds() << " " << best << endl;);
                        ASSERT(child[best].isPass() || isOnBoardZ<L>(child[best].z),
                               cerr << best << " " << child[best].z << endl;);
                        return best;
                    }
                    
                    void feed(int i, double sc){
                        child[i].feed(sc);
                        trials += 1;
                        score += sc;
                    }
                    
                    int chooseBestLCB()const{
                        constexpr double C = 0.1;
                        
                        int best = -1;
                        double bestLCB = -9999;
                        
                        // LCB最大のものを選ぶ
                        ASSERT(isPluged(),);
                        for (int i = 0; i < getNChilds(); ++i){
                            const auto& ch = child[i];
                            double lcb = ch.mean() - C * sqrt(log(trials) / ch.trials);
                            if(lcb > bestLCB){
                                best = i;
                                bestLCB = lcb;
                            }
                        }
                        ASSERT(0 <= best && best < getNChilds(),
                               cerr << getNChilds() << " " << best << endl;);
                        ASSERT(child[best].isPass() || isOnBoardZ<L>(child[best].z),
                               cerr << best << " " << child[best].z << endl;);
                        return best;
                    }
                    
#ifdef MONITOR
                    void print()const{
                        //モンテカルロ解析結果表示
                        for(int c = 0; c < getNChilds(); ++c){
                            
                            const int rew = (int)(child[c].mean() * 10000);
                            const double sem = sqrt(child[c].var()) * 10000;
                            const int rewZone[2] = {rew - (int)sem , rew + (int)sem};
                            
                            //まず調整済み評価点を表示
                            CERR << OutZ(child[c].z) << " : " << rew << " ( " << rewZone[0] << " ~ " << rewZone[1] << " ) ";
                            
                            //プレイアウトの結果を表示。これらの値は未調整。
                            CERR << child[c].trials << " trials. ";
                            CERR << endl;
                        }
                    }
#endif //MONITOR
                };
                
                }
                }
                
#endif // GO_JULIE_MC_MONTECARLO_H_
                
