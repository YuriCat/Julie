/*
 julie.hpp
 Katsuki Ohto
 */
 
#ifndef GO_JULIE_HPP_
#define GO_JULIE_HPP_

#include "julie/julie_go.hpp"

ThreadTools threadTools[N_THREADS];

#include "julie/search/mate.hpp"

#include "julie/policy.hpp"
namespace Go{
    namespace Julie{
        // 方策
        LinearPolicy orgPolicy;
        LinearPolicy policy;
    }
}

#ifndef PLAY_POLICY

#include "julie/mc/monteCarlo.h"
#include "julie/mc/playout.hpp"
#include "julie/mc/monteCarlo.hpp"

#endif

namespace Go{
	namespace Julie{
		
		class Client{ // メイン
		private:
            void boardLengthError()const{
                cerr << "unprepared board length = " << boardLength << endl;
            }
            
		public:
			// 情報
			Color myColor;
			int boardLength;
			
			// 局面情報
			EasyBoard<9> board9;
			EasyBoard<13> board13;
			EasyBoard<19> board19;
			
#ifndef PLAY_POLICY
			int monteCarloAge; // モンテカルロ着手決定に入った回数(1から)
#endif
			XorShift64 dice;
			
			// initializing
			int initMatch();
			int initGame(const int);
			// setting
			int setMyColor(const Color);
			int setHandicapStones();
			int setTimeLimit(const (uint64_t), const (uint64_t));
			// in play
			int recvPlay(const Color, const int, const (uint64_t));
			int play(const Color);
			int play_mc();
			int wait(const Color);
            int saveBoardImage(const Color);
			// board
			int countScore2()const;
			// aftergames
			int closeGame();
			int closeMatch();
			
			Client(){
				boardLength = 0;
				myColor = EMPTY;
			}
			
			~Client(){
				
			}
		};
		
		// 全試合開始時の初期化処理
		int Client::initMatch(){
			// ハッシュ値の割当を行う
			initHash();
			
			// 各スレッドの持ち物を初期化
			dice.srand((uint32_t)time(NULL));
			for(int th = 0; th < N_THREADS; ++th){
				threadTools[th].dice.srand(dice.rand() * (th + 111));
				memset(threadTools[th].buf, 0, sizeof(threadTools[th].buf));
			}
            // 方策パラメータを読み込み
            orgPolicy.fin(DIR_PARAMS_IN + "policy_param.dat");
            policy.fin(DIR_PARAMS_IN + "policy_param.dat");
			
			com_log = fopen((DIR_LOG + "com.log").c_str(), "w");
			
			return 0;
		}
		
		// 1試合開始時の初期化処理
		int Client::initGame(const int b_len){
            
            switch(b_len){
                case 9: case 13: case 19: break;
                default: boardLengthError(); return -1; break;
            }
            
            boardLength = b_len;
            // 一応全盤面を初期化
            board9.init();
            board13.init();
            board19.init();
            
#ifndef PLAY_POLICY
            // 置換表を確保
			switch(boardLength){
				case 9:{
					if(initMainTT<9>() < 0){ return -1; }
				}break;
				case 13:{
					if(initMainTT<13>() < 0){ return -1; }
				}break;
				case 19:{
					if(initMainTT<19>() < 0){ return -1; }
				}break;
                default: boardLengthError(); return -1; break;
			}
			monteCarloAge = 1;
#endif
			return 0;
		}
		// 自分の色の指定
		int Client::setMyColor(const Color c){
			myColor = c;
			return 0;
		}
		
		// 置き石が通達された際の処理
		int Client::setHandicapStones(){
			return 0;
		}
		
		// 制限時間が通達された際の処理
		int Client::setTimeLimit(const uint64_t limit, const uint64_t byoyomi){
			switch(boardLength){
				case 9: board9.setTimeLimit(limit, byoyomi); break;
				case 13: board13.setTimeLimit(limit, byoyomi); break;
				case 19: board19.setTimeLimit(limit, byoyomi); break;
                default: boardLengthError(); return -1; break;
			}
			return 0;
		}
		
		// 入力や相手のプレーを待っている際の処理
		int Client::wait(const Color c){
#ifndef PLAY_POLICY
			// 置換表が溢れているときは掃除してから先読み
            int z = Z_PASS;
			switch(boardLength){
                case 9:{
                    if(MainTT9 != nullptr){
                        MainTT9->clearByTurn(board9.getTurn());
                        while(MainTT9->isFilledOver(0.5)){ // 50%以上
                            MainTT9->clearOldest();
                        }
                        z = playWithMCTS(board9, c, monteCarloAge, true);
                    }
                }break;
                case 13:{
                    if(MainTT13 != nullptr){
                        MainTT13->clearByTurn(board13.getTurn());
                        while(MainTT13->isFilledOver(0.5)){ // 50%以上
                            MainTT13->clearOldest();
                        }
                        z = playWithMCTS(board13, c, monteCarloAge, true);
                    }
                }break;
                case 19:{
                    if(MainTT19 != nullptr){
                        MainTT19->clearByTurn(board19.getTurn());
                        while(MainTT19->isFilledOver(0.5)){ // 50%以上
                            MainTT19->clearOldest();
                        }
                        z = playWithMCTS(board19, c, monteCarloAge, true);
                    }
                }break;
                default: boardLengthError(); return -1; break;
			}
			++monteCarloAge;
#endif // PLAY_POLICY
			return 0;
		}
		
		// 着手決定を求められた際の処理
		int Client::play(Color c){
			// 自分の色判明
			if(myColor == EMPTY){
				setMyColor(c);
			}
            
			CERR << "board_length = " << boardLength << endl;
            
            int z = Z_PASS;
			
#ifndef PLAY_POLICY
			// モンテカルロ着手決定
			switch(boardLength){
				case 9:{
					z = playWithMCTS(board9, c, monteCarloAge, false);
				}break;
				case 13:{
					z = playWithMCTS(board13, c, monteCarloAge, false);
				}break;
				case 19:{
					z = playWithMCTS(board19, c, monteCarloAge, false);
				}break;
                default: boardLengthError(); return -1; break;
			}
			++monteCarloAge;
#else
            // 方策関数による着手決定
            switch(boardLength){
                case 9: z = playWithBestPolicy(c, board9, orgPolicy, &threadTools[0]); break;
                case 13: z = playWithBestPolicy(c, board13, orgPolicy, &threadTools[0]); break;
                case 19: z = playWithBestPolicy(c, board19, orgPolicy, &threadTools[0]); break;
                default: boardLengthError(); return -1; break;
            }
#endif // PLAY_POLICY
			out_ss();
			
			return z;
		}
		
		// プレーの受け取り
		int Client::recvPlay(const Color c, const int z, const uint64_t time){
			if(z == Z_RESIGN){
				return 0;
			}
			switch(boardLength){
				case 9:{
                    int validity = board9.move(c, z);
					if(!isValid(validity)){
						cerr << "move was illegal. (" << ruleString[validity] << ")" << endl;
					}
					board9.feedUsedTime(c, time);
					CERR << board9;
				}break;
				case 13:{
                    int validity = board13.move(c, z);
					if(!isValid(validity)){
						cerr << "move was illegal. (" << ruleString[validity] << ")" << endl;
					}
					board13.feedUsedTime(c, time);
					CERR << board13;
				}break;
				case 19:{
                    int validity = board19.move(c, z);
					if(!isValid(validity)){
						cerr << "move was illegal. (" << ruleString[validity] << ")" << endl;
					}
					board19.feedUsedTime(c, time);
					CERR << board19;
				}break;
                default: boardLengthError(); return -1; break;
			}
			
			out_ss();
			
			return 0;
		}
		
		int Client::countScore2()const{
			int ans = 0;
			switch(boardLength){
                case 9: ans = Go::countChineseScore2(board9); break;
				case 13: ans = Go::countChineseScore2(board13); break;
				case 19: ans = Go::countChineseScore2(board19); break;
                default: boardLengthError(); return -1; break;
			}
			return ans;
		}
		
		// １ゲーム終了後の処理
		int Client::closeGame(){
            
#ifndef PLAY_POLICY
            switch(boardLength){
                case 9: closeMainTT<9>(); break;
                case 13: closeMainTT<13>(); break;
                case 19: closeMainTT<19>(); break;
                default: boardLengthError(); return -1; break;
            }
#endif
            
			return 0;
		}
		
		// 全ゲーム終了後の処理
		int Client::closeMatch(){
			return 0;
		}
	}
}

#endif // GO_JULIE_HPP_