#ifndef UECDA_STRUCTURE_MINLOGIO_HPP_
#define UECDA_STRUCTURE_MINLOGIO_HPP_

#ifdef LOGGING

//sgfファイル読み込み

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

namespace UECda{
    
    //入力
    const int N_COMMANDS = 13;
    std::string command[N_COMMANDS] = {
        "//",
        "/*",
        "*/"
        "player",
        "game",
        "score",
        "seat",
        "class",
        "dealt",
        "changed",
        "original",
        "play",
        "result",
    };
    
    std::map<std::string, int> cmdMap;
    
    bool isCommand(const std::string& str){
        /*for(int i=0;i<12;++i){
         if(str==command[i]){return true;}
         }*/
        if(cmdMap.find(str) != cmdMap.end()){ return true; }
        return false;
    }
    
    int StringsMtoCards(std::vector<std::string>& vec, Cards *const dst){
        *dst = CARDS_NULL;
        for(auto itr = vec.begin(); itr != vec.end(); ++itr){
            IntCard ic = StringMtoIntCard(*itr);
            if(ic < 0){ return -1; }
            addIntCard(dst, ic);
        }
        return 0;
    }
    
    int StringQueueMtoCards(std::queue<std::string>& q, Cards *const dst){
        *dst = CARDS_NULL;
        const std::string& str = q.front();
        if(isCommand(str)){
            cerr<<"com"<<endl; return -1;
        }
        if(str != "{"){
            cerr<<"not {"<<endl; return -1;
        }
        q.pop();
        while(q.front() != "}"){
            if(!q.size()){
                cerr << "not q size" << endl; return -1;
            }
            const std::string& str = q.front();
            if(isCommand(str)){
                cerr << "com" << endl; return -1;
            }
            IntCard ic = StringMtoIntCard(str);
            //DERR<<str<<" "<<OutIntCard(ic)<<endl;
            if(ic < 0){
                cerr << "bad intcard" << endl; return -1;
            }
            addIntCard(dst, ic);
            q.pop();
        }
        q.pop();
        return 0;
    }
    
    int StringMtoMoveTime(const std::string& str, Move *const dstMv, uint64_t *const dstTime){
        Move mv;
        
        const std::string dlm = " []\n";
        
        std::vector<std::string> v;
        boost::algorithm::split(v, str, boost::is_any_of(dlm), boost::algorithm::token_compress_on);
        
        DERR << v[0] << "," << v[1] << endl;
        
        if(v.size() <= 0){
            DERR << "k" << endl;
            return -1;
        }
        
        mv = StringMtoMove(v[0]);
        if(mv.isILLEGAL()){
            DERR << "illegal move" << endl;
            return -1;
        }
        
        *dstMv = mv;
        
        if(v.size() <= 1){
            DERR << "kk" << endl;
            return 0;
        }
        
        *dstTime = boost::lexical_cast<(uint64_t)>(v[1]);
        
        return 0;
    }
    
    void initCommandMap(){
        for(int i = 0; i < N_COMMANDS; ++i){
            cmdMap[command[i]] = i;
        }
    }
    
#define Foo() DERR << "unexpected command : " << q.front() << endl; goto NEXT;
#define ToI(str) boost::lexical_cast<(uint64_t)>(str)
    
    int readSGF(std::ifstream& ifs, log_t *const plog){
        
        using std::string;
        using std::vector;
        using std::queue;
        
        initCommandMap();
        
        gameLog_t gLog;
        BitArray32<4, N> infoClass;
        BitArray32<4, N> infoSeat;
        BitArray32<4, N> infoNewClass;
        std::map<int, changeLog_t> cLogMap;
        Move lastMove;
        
        BitSet32 flag1G;
        BitSet32 flagAllG;
        
        int t;
        
        queue<string> q;
        string str;
        
        //ここから読み込みループ
        cerr << "start reading log..." << endl;
        while(1){
            
            while(q.size() < 1000){
                // コマンドを行ごとに読み込み
                if(!getline(ifs, str,'\n')){
                    q.push("//");
                    break;
                }
                
                vector<string> v;
                boost::algorithm::split(v, str, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
                
                for(auto itr = v.begin(); itr != v.end(); ++itr){
                    
                    //DERR<<"front "<<*itr<<endl;
                    if(itr->size() > 0){
                        q.push(*itr);
                    }
                }
            }
            
            string cmd = q.front();
            q.pop();
            
            if(cmd == "//"){ // 全試合終了
                break;
            }
            else if(cmd == "/*"){//game開始合図
                gLog.init();
                infoClass.clear();
                infoSeat.clear();
                infoNewClass.clear();
                cLogMap.clear();
                lastMove = MOVE_PASS;
                flag1G.init();
            }
            else if(cmd == "*/"){//game終了合図
                //ログとして必要なデータが揃っているか確認
                //if(!flag1G.test(0)){}//ゲーム番号不明
                //if( !flag1G.test(0) ){}//累積スコア不明
                if(!lastMove.isPASS()){
                    gLog.setTerminated();
                }
                gLog.infoClass() = infoClass;
                gLog.infoSeat() = infoSeat;
                gLog.infoNewClass() = infoNewClass;
                pmLog->push_game(gLog);
            }
            else if(cmd == "match"){
                const string& str=q.front();
                if(isCommand(str)){Foo();}
                q.pop();
                flagAllG.set(0);
            }
            else if(cmd == "player"){
                for(int i = 0; i < N; ++i){
                    const string& str = q.front();
                    if(isCommand(str)){ Foo(); }
                    DERR << "pname : " << str << endl;
                    pmLog->setPlayer(i, str);
                    q.pop();
                }
                flagAllG.set(1);
            }
            else if(cmd == "game"){
                const string& str = q.front();
                if(isCommand(str)){ Foo(); }
                int gn = boost::lexical_cast<int>(str);
                //cerr << "game " << gn << endl;
                q.pop();
                flag1G.set(0);
            }
            else if(cmd == "score"){
                DERR << "score-data" << endl;
                for(int i = 0; i < N; ++i){
                    const string& str = q.front();
                    if(isCommand(str)){ Foo(); }
                    q.pop();
                }
                flag1G.set(1);
            }
            else if( cmd=="class" ){
                for(int i=0;i<N;++i){
                    const string& str=q.front();
                    if(isCommand(str)){Foo();}
                    infoClass.replace(i, ToI(str));
                    q.pop();
                }
                flag1G.set(2);
            }
            else if(cmd == "seat"){
                for(int i = 0; i < N; ++i){
                    const string& str = q.front();
                    if( isCommand(str) ){ Foo(); }
                    infoSeat.replace(i, ToI(str));
                    q.pop();
                }
                flag1G.set(3);
            }
            else if(cmd == "dealt"){
                for(int i = 0; i < N; ++i){
                    Cards c;
                    if(StringQueueMtoCards(q, &c) < 0){ Foo(); }
                    gLog.setDealtCards(i, c);
                    DERR << OutCards(c) << endl;
                }
                flag1G.set(4);
            }
            else if(cmd == "changed"){
                bool anyChange = false;
                BitArray32<4, N> infoClassPlayer = invert(infoClass);
                
                for(int i = 0; i < N; ++i){
                    Cards c;
                    if(StringQueueMtoCards(q, &c)<0){ Foo(); }
                    if(anyCards(c)){
                        anyChange = true;
                        changeLog_t cLog(i, infoClassPlayer[getChangePartnerClass(infoClass[i])], c);
                        
                        cLogMap[-infoClass[i]] = cLog;
                    }
                }
                if(!anyChange){
                    gLog.setInitGame();
                    DERR << "init game." << endl;
                }else{
                    for(auto c : cLogMap){
                        gLog.push_change(c.second);
                    }
                }
                flag1G.set(5);
            }
            else if(cmd == "original"){
                for(int i = 0; i < N; ++i){
                    Cards c;
                    if(StringQueueMtoCards(q, &c) < 0){ Foo(); }
                    gLog.setOrgCards(i, c);
                }
                flag1G.set(6);
            }
            else if(cmd == "play"){
                while(1){
                    Move mv; uint64_t time;
                    const string& str = q.front();
                    if(isCommand(str)){ Foo(); }
                    if(StringMtoMoveTime(str, &mv, &time) < 0){ Foo(); }
                    playLog_t pLog(mv, time);
                    gLog.push_play(pLog);
                    lastMove = mv;
                    q.pop();
                }
                flag1G.set(7);
            }
            else if(cmd == "result"){
                for(int i = 0; i < N; ++i){
                    const string& str = q.front();
                    if(isCommand(str)){ Foo(); }
                    infoNewClass.replace(i, ToI(str));
                    q.pop();
                }
                flag1G.set(8);
            }
            else{
                //いかなるコマンドでもないので読み飛ばし
                q.pop();
            }
        NEXT:;
        }
    END:;
        
        cerr << pmLog->games() << " games were loaded." << endl;
        
        //これで全試合が読み込めたはず。
        return 0;
    }
#undef ToI
#undef Foo
    
    template<log_t>
    int readSGF(const std::string& fName, log_t *const plog){
        std::ifstream ifs;
        ifs.open(fName, std::ios::in);
        if(!ifs){
            cerr << "UECda::readMatchLogFile() : no log file." << endl;
            return -1;
        }
        return readSGF(ifs, plog);
    }
}

#endif // LOGGING
#endif // UECDA_STRUCTURE_MINLOGIO_HPP_
