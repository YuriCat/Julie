// 盤面情報

namespace Go{
    
    template<int L>
    struct Cell{
        // 基本的な情報
        Color c;
        int stringNumber; // 所属する連の番号
        BitArray64<16, 4> aroundStringNumber; // 周囲のマスが所属する連の番号
        BitArray64<8, 8> cfgDistance[BOARD_ARRAY_SIZE_ / 8]; // common fate graph の距離
        
        // 重要な情報
        
        // 周囲の情報
        int liberty; // 呼吸点の数
        uint32_t pat3x3_; // 3x3パターンの完全ハッシュ値(直前の手を考慮)
        
        // 戦略的情報
        int capture[2]; // 黒、白が石を置いたときに取れる個数
        
        bool isEmpty()const noexcept{ return (c == EMPTY); }
        Color color()const noexcept{ return c; }
        
        void init(int z)noexcept{
            stringNumber = -1;
            aroundStringNumber.clear();
            if(isOnBoardZ<L>(z)){
                c = EMPTY;
                int i5z = IXIYtoIZ<5>(ItoSmallerBoardI<L, 5>(ZtoIX(z)),
                                      ItoSmallerBoardI<L, 5>(ZtoIY(z)));
                pat3x3_ = i5z * 9 * ipow(3, 8);
                
                // 盤端の特殊な処理
                if(isEdgeZ<L>(z)){
                    if(isCornerZ<L>(z)){
                        liberty = 2;
                    }else{
                        liberty = 3;
                    }
                }else{
                    liberty = 4;
                }
            }else{ // 盤外
                c = WALL;
                pat3x3_ = 0;
                liberty = 0;
            }
        }
        void move(Color ac)noexcept{ c = ac; }
        void remove()noexcept{ c = EMPTY; }
        void moveAround(Color c, int ai)noexcept{
            // 石が置かれた場合の3x3keyの更新
            pat3x3_ += c * ipow(3, ai);
        }
        void remove3x3(Color c, int ai)noexcept{
            // 石が取り除かれた場合の3x3keyの更新
            pat3x3_ -= c * ipow(3, ai);
        }
        void pass3x3(int ai)noexcept{
            // 直前の手の影響が消える
            pat3x3_ -= ai * ipow(3, 8);
        }
    };
    
    template<int L>
    struct String{
        Color c;
        int size; // 連の個数
        int liberty; // 呼吸点の数
        int pseudoLiberty; // 重複を許した呼吸点の数
        int kyusho; // 打たれたら取られる場所
        BitBoard<L> stone; // 連を構成する石の集合
        BitBoard<L> libStone; // 呼吸点の集合
        int dead; // 死んでいる
    };
    
	template<int L>
    class JulieBoard{
    public:
        
        Cell& cell(int z){ return cell_[z]; }
        Cell& cell(int x, int y){ return cell_[XYtoZ(x, y)]; }
        const Cell& cell(int z)const{ return cell_[z]; }
        const Cell& cell(int x, int y)const{ return cell_[XYtoZ(x, y)]; }
        
        Color color(int z)const{ return cell_[z].color(); }
        Color color(int x, int y)const{ return cell_[XYtoZ(x, y)].color(); }
        bool isEmpty(int z)const{ return cell.isEmpty(); }
        
        double prob(Color c, int z)const{ return cell_[z].prob_[toColorIndex[c]]; }
        double setProb(Color c, int z, double p){ cell_[z].prob_[toColorIndex[c]] = p; }
        
        String& string(int istr){ return string_[istr]; }
        const String& string(int istr)const{ return string_[istr]; }
        
        int stringNumber(int z)const{ return cell(z).stringNumber; }
        
    private:
		constexpr static int BOARD_ARRAY_SIZE_ = getBoardArraySize(L);
        
    public:
        constexpr static int X_FIRST_ = X_FIRST;
        constexpr static int Y_FIRST_ = Y_FIRST;
		constexpr static int X_LAST_ = getXLast(L);
		constexpr static int Y_LAST_ = getYLast(L);
        
        constexpr static int length()noexcept{ return L; }
        
        // 探索用パラメータ
        int threadIndex;
        int depth;
        XorShift64 dice;
        
        // 盤の情報
        int handicap; // 置石個数
		int turn; // これまでの手数(置き石は含まない)
		Color color;
		Cell cell_[BOARD_ARRAY_SIZE_];
		int caputured[2];
		int koZ, lastZ; // 直前の手(パス含む)
		Color lastColor; // 最後に打った(パス含まず)プレーヤー
        
        BitBoard colorBits_[3]; // 空, 白, 黒
        
        // 連の情報
        String string_[BOARD_ARRAY_SIZE_]; // 連
        BitBoard stringBits_; // 連番号ビット集合
        int NStrings_[2]; // 各色の連の数
        
        // 持ち時間
        TimeLimit limit[2];
        TimeLimit orgLimit[2]; // 最初の時間
		
		// 盤面ハッシュ値
		// 手番をfreeとするため、実用上は手番情報をその場で与えて使う
		u64 hash_stone_[8];
        
        // 着手確率テーブル(Z_PASSにパスの記録を入れる)
        double prob_[2][BOARD_ARRAY_SIZE_];
        
        template<class callback_t>
        void iterateStringStone(int istr, const callback_t& callback){
            /*int z = string(istr).firstZ;
            int lz = string(istr).lastZ;
            while(1){
                callback(z);
                if(z == lz){ break; }
                z = cell(z).next;
            }*/
            iterate(string(istr).stone, [&callback](int z)->void{
                callback(z);
            });
        }
		
		void init()noexcept{
            // init cells
            for(int z = 0; z < BOARD_ARRAY_SIZE_; ++z){
                cell_[z].init(z);
            }
			caputured[0] = caputured[1] = 0;
			koZ = Z_PASS;
			lastZ = Z_NONE;
			lastColor = WHITE;
			turn = 0;
			initHashx8<L>(hash_stone_);
		}
        
        // time limit
        void setTimeLimit(uint64_t lim, uint64_t by){
            for(int i = 0; i < 2; ++i){
                limit[i].set(lim, by);
                orgLimit[i].set(lim, by);
            }
		}
        
        void feedUsedTime(Color c, uint64_t ut){
            limit[toColorIndex(c)].feed(ut);
        }
		void removeString(Color c, int istr){
            iterateStringStone(string(istr), [this, c](int z)->void{
                cell(z).remove();
                iterateAround8WithIndex(z, [this](int ai, int az)->void{
                    cell(az).remove3x3(c, invertAroundIndex8(ai)); // 3x3ハッシュを更新
                });
            });
		}
        int getTurn()const noexcept{
            return turn;
        }
        Color getTurnColor()const noexcept{
            return toTurnColor(turn , (handicap > 0));
        }
		
		int check(const Color myColor, const int z)const{
            // okならVALID、badならそれ以外
            // パスは常に可能なので入らない
            ASSERT(z != Z_PASS,);
            
			if(!isEmpty(z)){ return DOUBLE; } // 空でない
			if(z == koZ){ return KO; } // コウ
            
            const Color oppColor = flipColor(myColor);
            
            // 自分が石を打ったときに所属する呼吸点が0でないかどうか調べる
            bool ok = false;
            int buddies = 0;
            int walls = 0;
            for(int i = 0; i < 4; ++i){
                int tz = z + dir4(i);
                Color tc = color(tz);
                if(tc == EMPTY){ ok = true; } // 周囲に空点があれば呼吸点は1以上
                if(tc == WALL){ walls += 1; }
                if(tc == myColor){ // 同じ色の石
                    int sn = stringNumber(tz);
                    const auto& str = string(sn);
                    if(str.liberty > 1){ // 複数の呼吸点がある
                        ok = true;
                    }
                    ++buddies;
                }
            }
            
            if(!ok){ return SUICIDE; } // 自殺手
            if(walls + buddies == 4){ return EYE; } // 眼
			return VALID;
		}
		
		int move(const Color col, const int z){
            // 合法であることを前提とする
			
			if(z == Z_PASS){
				if(koZ != Z_PASS){
					// コウのハッシュ値を戻しておく
					procKoHashx8<L>(hash_stone_, koZ);
				}
				koZ = Z_PASS; lastZ = z; ++turn; return 0;
			} // パス
			ASSERT(cell(z).isSpace(),); // 空である必要
			
			Color oppColor = flipColor(col);
			
			// 呼吸点が0になった石を取る操作
            int newStringNum = z; // 最小の連番号を連番号とする
            BitArray64<16, 4> aroundMyStrings(0);
            iteratAround4WithIndex([oppColor, &newStringNum, &aroundMyStrings, &this](int i, int tz)->void{
                Color tc = color(az);
                if(tc == oppColor){
                    int sn = stringNumber(tz);
                    if(string(sn).liberty == 1){ // 呼吸点が0になる
                        removeString(oppColor, sn);
                    }
                }else if(tc == myColor){
                    int sn = stringNumber(tz);
                    newStringNum = min(newStringNum, sn); // 更新後の連番号
                    aroundMyStrings.push_front(sn);
                }
            });
			
			// 石を置く
			cell(z).putStone(col);
			lastZ = z;
			++turn;
			
            // 連の更新
            if(newStringNum == z){ // 新しい連が自分中心になる
                
            }
            iterateAny(aroundMyStrings, [newStringNum, &this](int sn)->void{
                mergeString()
            });
            
			// ハッシュ値更新
			
			if(koZ != Z_PASS){
				// コウのハッシュ値を戻しておく
				procKoHashx8<L>(hash_stone_, koZ);
			}
            procHashx8<L>(hash_stone_, col, z);
				
			// コウかどうかを判断
			int dame, ishi;
			count_dame(z, &dame, &ishi);
			
			if(take_sum == 1 && stringSize(stringNumber(z)) == 1 && dame == 1){
				koZ = ko_kamo;
				if(ko_kamo != Z_PASS){
					// コウのハッシュ値
					procKoHashx8<L>(hash_stone_, ko_kamo);
				}
			}else{
				koZ = Z_PASS;
			}
			
			return 0;
		}
		
		
		bool exam()const{ // validator
			return true;
		}
	};
}