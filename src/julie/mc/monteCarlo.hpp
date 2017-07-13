/*
 monteCarlo.hpp
 Katsuki Ohto
 */

#ifndef GO_JULIE_MC_MONTECARLO_HPP_
#define GO_JULIE_MC_MONTECARLO_HPP_

// 囲碁
// モンテカルロ着手決定

namespace Go{
	namespace Julie{
        
        template<class root_t, class board_t>
        int playoutThread(const int threadId,
                          root_t *const proot, const board_t *const pbd,
                          const Color c){
            
            constexpr int L = board_t::length();
            
            // プレイアウトスレッド
            // マルチスレッディングにしない場合は関数として開く
            
            ClockMS clms; // 時計(millisec単位)
            clms.start();
            
            // グローバルに置いてあるこのスレッド用の道具
            auto& tools = threadTools[threadId];
            
            EasyBoard<L> bd; // プレイアウタ空間
            EasyBoard<L> bdSaved; // プレイアウタ空間(保存用)
            
            int threadPlayouts = 0; // このスレッドのプレイアウト回数
            
            proot->unlock();
            
            bdSaved = bd; // プレイアウタ空間(保存用)準備
            
            while(!gState.requestedToStop()){ // 終了判定が出ていない
                
                DERR << "playout " << threadPlayouts << ".";
                
                int i = proot->chooseBestUCB(&tools.dice); // 検討する着手を選ぶ
                int ret;
                int z = proot->child[i].z;
                
                DERR << "move " << z << endl;
                
                PlayoutResult<L> result;
                if(bdSaved.lastZ == Z_PASS && z == Z_PASS){ // 連続パスで即終了
                    result.setScore2(countChineseScore2(bdSaved));
                }else{
                    bd = bdSaved;
                    startSimulation(&result, bd, c, z, policy, &tools);
                }
                
                double sc = result.result(c);
                
                // 結果報告
                proot->lock();
                proot->feed(i, sc);
                proot->unlock();
                
                ++threadPlayouts;
                
                // 終了判定
                if(clms.stop() > proot->limit || threadPlayouts > 100000){ // 制限時間オーバー
                    gState.requestToStop(); // 終了命令
                    return 0;
                }
            }
            
            // 終了判定が出た
            return 0;
        }
        
        template<class board_t>
		uint64_t decideLimitTime(const board_t& bd, Color c){
			// 自分の着手決定限度時間を設定
			uint64_t tmp;
			const auto& tl = bd.limit[toColorIndex(c)];
			
			if(tl.isUnlimited()){ // 持ち時間無制限
				tmp = 10000; // 10秒としておく
			}else if(tl.isNoTime()){
				tmp = 200; // 200ミリ秒としておく
			}else{
				// とりあえず彩の手法
				double C;
                switch(board_t::length()){
					case 9: C  = 8; break;
					case 13: C = 30 - (((double)bd.turn) / 5.0); break;
					case 19: C = 60 - (((double)bd.turn) / 10.0); break;
					default: C = 100; break;
				}
				if(C <= 5){ C = 5; }
				
				tmp = min((uint64_t)(((double)tl.limit_ms) / C), 15000ULL);
				
				if(tl.byoyomi_ms > 0){
					tmp += tl.byoyomi_ms;
				}
			}
			return tmp;
		}
		
		template<class board_t>
		int playWithMCTS(const board_t& bd, Color col, int age, bool ponder){
			// モンテカルロで着手決定
            constexpr int L = board_t::length();
            
            auto *const ptt = getMainTTPtr<L>();
            if(ptt != nullptr){
                ptt->setAge(age);
            }
            
			//必勝形を判定
			int decidedZ = judgeEasyMate(bd, col);
			if(decidedZ != Z_RESIGN){
				return decidedZ;
			}
			
            // ルートノードの処理
			RootNode<L> root;
			root.set(bd, col);
			
			if(root.decidedZ != Z_RESIGN){
				return root.decidedZ;
			}
			
			// 思考時間制限を決める
			uint64_t limit = decideLimitTime(bd,col);
			if(ponder){
				limit *= 6;
			}
			
			CERR << "thinking time limit ... " << limit << " ms" << endl;
			
			root.setLimitTime(limit);
			
#ifdef MULTI_THREADING
            // open threads
            std::thread thr[N_THREADS];
            for (int th = 0; th < N_THREADS; ++th){
                thr[th] = thread(&playoutThread<RootNode<L>, EasyBoard<L>>, th, &root, &bd, col);
            }
            for (int th = 0; th < N_THREADS; ++th){
                thr[th].join();
            }
#else
            // call function
            playoutThread<RootNode<L>, EasyBoard<L>>(0, &root, &bd, col);
            
#endif //MULTI_THREADING
            
            int best = 0;
            if(!ponder){
                best = root.chooseBestLCB(); // 最高評価のもの
            }
			
#ifdef MONITOR
			CERR << "MCP : " << root.trials << " ( old:" << root.oldTrials << " new:" << (root.trials - root.oldTrials)
                << " ) playouts were done." << endl;
			CERR << "MCP : " << getMainTTPtr<L>()->pages() << " nodes were made." << endl;
			
			root.print();
			
#endif //MONITOR
			
            if(!ponder && root.child[best].mean() < WP_RESIGN){
                CERR << "resign." << endl;
                return Z_RESIGN;
            }else{
                return root.child[best].z;
            }
            return 0;
		}
	}
}

#endif // GO_JULIE_MC_MONTECARLO_HPP_