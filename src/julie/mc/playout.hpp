/*
 playout.hpp
 Katsuki Ohto
 */

#ifndef GO_JULIE_MC_PLAYOUT_HPP_
#define GO_JULIE_MC_PLAYOUT_HPP_

#include "../julie_go.hpp"
#include "monteCarlo.h"

namespace Go{
	namespace Julie{
        
        // プレイアウトの結果
        struct PlayResult{
            //MoveInfo mv;
            //double p;
            int z;
            void set(int az)noexcept{
                z = az;
            }
        };
        
        template<int L>
        struct PlayoutResult{
            int sc2; double blackResult;
            PlayResult play[getNTurnsMax(L)];
            
            double result(Color c)const noexcept{
                return isBlack(c) ? blackResult : (1 - blackResult);
            }
            void setScore2(int asc2)noexcept{
                sc2 = asc2;
                blackResult = sigmoid(sc2, 1);
            }
        };
		
		template<class result_t, class board_t, class policy_t, class tools_t>
		void doSimulation(result_t *const presult, board_t& bd,
                          const policy_t& pol, tools_t *const ptools){
            // 局面からシミュレーション開始
            for(;;){
                Color c = bd.getTurnColor();
                int z = playWithPolicy(c, bd, pol, ptools);
                if(bd.lastZ == Z_PASS && z == Z_PASS){
                    // 連続パスで終了
                    goto GAME_END;
                }else{
                    presult->play[bd.getTurn()].set(z);
                    bd.move(c, z);
                }
            }
		GAME_END: // 試合終了
			int sc2 = countChineseScore2(bd);
            presult->setScore2(sc2);
		}
        
        template<class result_t, class board_t, class policy_t, class tools_t>
        void startSimulation(result_t *const presult, board_t& bd,
                             const Color c, const int z,
                             const policy_t& pol, tools_t *const ptools){
            // 局面+着手からシミュレーション開始
            if(bd.lastZ == Z_PASS && z == Z_PASS){ // 連続パスで即終了
                int sc2 = countChineseScore2(bd);
                presult->setScore2(sc2);
            }else{ // 試合継続
                presult->play[bd.getTurn()].set(z);
                bd.move(c, z);
                doSimulation(presult, bd, pol, ptools);
            }
        }
        
        
		/*
		template<class node_t, class board_t>
		void doUCT(node_t& node, board_t& bd){
			// 現局面が登録されている事を前提とする
			
            int sc2;
			
			// どの着手を検討するか、方策関数とノードに登録されている結果から選ぶ
            iterateZ(bd, )
			
			int id=pnode->chooseBestUCB();
			//決まった
			
			const auto& child = pnode->child[id];
			
			if(pch->isPASS() ){
				if( bd.last_z==Z_PASS ){//連続パスで試合終了
					sc2=bd.count_score_CHINESE();
					goto GAME_END;
				}else{
					bd.template pass<COL>();
				}
			}else{
				//局面更新
				
				bd.template move<COL>(pch->z);
				--bd.nnp;
				bd.np[bd.rnp[pch->z]]=bd.np[bd.nnp];//空点リストから削除
				bd.rnp[bd.np[bd.nnp]]=bd.rnp[pch->z];
			}
			
			if( bd.tesu > board_t::N_TE_MAX ){
				//手数オーバー
				sc2=bd.getTmpScore2();
				
			}else if( pch->NTrials < board_t::MC_EXTRACT_LINE ){
				//プレイアウト
				sc2=play(bd,flipColor(COL));
			}else{
				//下位ノードへ
				bool found;
				const uint64_t next_hash=bd.hash_bd^(flipColor(COL)-1);
				auto *const pnext_node=getMainTTPtr<board_t::BOARD_LENGTH>()->read(next_hash,&found);
				
				if( pnext_node!=nullptr ){
					if( !found ){//次ノードがまだない
						//ノード作成
						pnext_node->template set<flipColor(COL)>( bd, next_hash, age );
						
						DERR<<"registed "<<std::hex<<next_hash<<std::dec<<endl;
					}
					if( COL==BLACK ){
						sc2=play_uct<WHITE>(depth+1,bd,pnext_node);
					}else{
						sc2=play_uct<BLACK>(depth+1,bd,pnext_node);
					}
				}else{
					//次ノードが作成不可能
					//以降をプレイアウトで開く
					sc2=play(bd,flipColor(COL));
				}
			}
			
		GAME_END:
            pnode->lock();
            pnode->feed(*presult);
            pnode->unlock();
		}
		*/
	}
}

#endif // GO_JULIE_MC_PLAYOUT_HPP_
