// 囲碁
// 探索スレッド

namespace Go{
    namespace Julie{
        template<class root_t, class board_t>
        int monteCarloThread(const int threadIndex, root_t *const proot, const board_t *const pbd, const Color myColor){
            
            // プレイアウトスレッド
            // マルチスレッディングにしない場合は関数として開く
            
            ClockMS clms; // 時計(millisec単位)
            clms.start();
            
            //グローバルに置いてあるこのスレッド用の道具
            auto& tools = thread_tools[threadIndex];
            
            
            
            ThinkBoard<board_t::length()> board; // プレイアウタ空間
            ThinkBoard<board_t::length()> savedBoard; // プレイアウタ空間(保存用)
            
            int threadNPlayouts = 0; // このスレッドのプレイアウト回数
            
            proot->unlock();
            
            pob_sav.set(*bd, myColor);//プレイアウタ空間(保存用)準備
            
            while(!gState.requestedToStop()){//終了判定が出ていない
                
                DERR << "playout " << threadNPlayouts << ".";
                
                int i = proot->chooseBestUCB(&tools.dice); // 検討する着手を選ぶ
                int ret;
                int z = proot->child[i].z;
                
                //if( z<0 ){CERR<<z<<" th ";}
                
                DERR << "move " << z << endl;
                
                int sc2 = 0;
                if( pob_sav.last_z==Z_PASS && z==Z_PASS ){
                    //連続パスで即終了
                    sc2=pob_sav.count_score_CHINESE();
                }else{
                    memcpy( &pob, &pob_sav, sizeof(Goban<B_LEN>) );
                    
                    if( z!=Z_PASS ){
                        
                        ASSERT( isOnBoard<B_LEN>(ZtoX(z),ZtoY(z)), cerr<<z<<endl; );
                        
                        if( col==BLACK ){
                            int err=pob.template move<BLACK,1>(z);//局面進行、合法性チェック済み
                            --pob.nnp;
                            pob.np[pob.rnp[z]]=pob.np[pob.nnp];//空点リストから削除
                            pob.rnp[pob.np[pob.nnp]]=pob.rnp[z];
                            sc2=po.template start_uct<WHITE>(pob);
                        }else{
                            int err=pob.template move<WHITE,1>(z);//局面進行、合法性チェック済み
                            --pob.nnp;
                            pob.np[pob.rnp[z]]=pob.np[pob.nnp];//空点リストから削除
                            pob.rnp[pob.np[pob.nnp]]=pob.rnp[z];
                            sc2=po.template start_uct<BLACK>(pob);
                        }
                    }else{
                        if( col==BLACK ){
                            pob.template pass<BLACK>();
                            sc2=po.template start_uct<WHITE>(pob);
                        }else{
                            pob.template pass<WHITE>();
                            sc2=po.template start_uct<BLACK>(pob);
                        }
                    }
                }
                
                assert( sc2!=0 );
                
                if( col==BLACK ){
                    if( sc2 > 0 ){
                        ret=1;
                    }else{
                        ret=0;
                    }
                }else{
                    if( sc2 < 0 ){
                        ret=1;
                    }else{
                        ret=0;
                    }
                }
                
                //結果報告
                proot->lock();
                proot->feedResult(i, ret);
                proot->feedAmafResult(pob.kifu, i, ret, pob_sav.tesu, pob.tesu);
                proot->unlock();
                
                ++thread_nplayouts;
                
                if(threadNPlayouts % 4 == 0){
                    //たまに終了判定
                    if(clms.stop() > mcf->limit_time || threadNPlayouts > (( == 9)?40000:10000) ){//制限時間オーバー
                        gState.requestToStop();//終了命令
                        return 0;
                    }
                }
            }
            
            //終了判定が出た
            return 0;
        }
    }
}

