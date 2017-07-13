// 囲碁
// 合法着手生成

#ifndef GO_GENERATOR_HPP_
#define GO_GENERATOR_HPP_

namespace Go{
	
	template<class move_t, class board_t>
	int genAllMoves(move_t *const mv0, const board_t& bd){
		// 全ての合法着手を生成して合法着手数を返す
        move_t *mv = mv0;
        iterateZ(bd, [&mv](int z)->void{
            if(bd.check(z) == VALID){
                mv->set(z); ++mv;
            }
        }
		mv->setPASS(); ++mv;
		return mv - mv0;
	}
	/*
	template<int COL,class move_t,class board_t>
	int fill_legal_moves_desultorily(move_t *const mv0,const board_t& bd){
		//全ての合法着手を生成して合法着手数を返す
		//盤の端からの通し番号の位置にセットする
		//手が無い箇所のzをZ_RESIGNとしておく
		int cnt=0;
		move_t *mv=mv0;
		for(int x=X_FIRST;x<=board_t::X_LAST;++x){
			for(int y=Y_FIRST;y<=board_t::Y_LAST;++y){
				++mv;
				int z=XYtoZ(x,y);
				if( !bd.template check<COL>(z) ){
					mv->set(z);++cnt;
				}else{
					mv->setResign();
				}
			}
		}
		assert( mv-mv0 == board_t::BOARD_LENGTH * board_t::BOARD_LENGTH +1 - 1 );
		
		if( cnt==0 ){
			mv0->setPASS();++cnt;
		}else{
			mv0->setResign();
		}
		
		for(int i=0;i<board_t::BOARD_LENGTH * board_t::BOARD_LENGTH + 1;++i){
			ASSERT( mv0[i].z==Z_RESIGN || mv0[i].isPASS() || isOnBoard<board_t::BOARD_LENGTH>(ZtoX(mv0[i].z),ZtoY(mv0[i].z))
				, cerr<<i<<" "<<mv0[i].z<<endl; );
		}
		//cerr<<mv0[75].z<<endl;
		
		return cnt;
	}
	
	template<class move_t,class board_t>
	int fill_legal_moves(move_t *const mv0,const board_t& bd,int col){
		//全ての合法着手を生成して合法着手数を返す
		move_t *mv=mv0;
		for(int x=X_FIRST;x<=board_t::X_LAST;++x){
			for(int y=Y_FIRST;y<=board_t::Y_LAST;++y){
				int z=XYtoZ(x,y);
				if( !bd.check(col,z) ){
					mv->set(z);++mv;
				}
			}
		}
		if( mv==mv0 ){
			mv->setPASS();++mv;
		}
		return mv-mv0;
	}
	
	template<class move_t,class board_t>
	int fill_moves_root(move_t *const mv0,const board_t& bd,const int col){
		//ルートノードでの合法着手生成
		move_t *mv=mv0;
		for(int x=X_FIRST;x<=board_t::X_LAST;++x){
			for(int y=Y_FIRST;y<=board_t::Y_LAST;++y){
				int z=XYtoZ(x,y);
				if( !bd.check(col,z) ){
					mv->set(z);++mv;
				}
			}
		}
		if( mv==mv0 && bd.hasAdvantage(col) && bd.last_z!=Z_PASS ){
			mv->setPASS();++mv;
		}
		return mv-mv0;
	}
	*/
	//template<class move_t>
	//void remove(move_t *mv,int i,int j)
	//mv[]
	
}

#endif // GO_GENERATOR_HPP_