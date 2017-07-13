/*
 mate.hpp
 Katsuki Ohto
*/

// 囲碁
// 勝敗判定

namespace Go{
	namespace Julie{
		
		template<class board_t>
		int judgeEasyMate(const board_t& bd, const Color col){
			//簡単な詰みを判定
			if(bd.lastZ == Z_PASS){
                int sc2 = countChineseScore2(bd);
				if((col == BLACK && sc2 > 0)
                   || (col != BLACK && sc2 < 0)){ // win by pass
					return Z_PASS;
				}
			}
			return Z_RESIGN;
		}
	}
}
