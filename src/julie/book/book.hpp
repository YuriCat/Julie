//囲碁
//定石
namespace Go{
	
	
	
	const unsigned int BK_SIZE_INDEX=6;// 定跡ファイルのインデックスの1つのエントリーの大きさ
	const unsigned int BK_SIZE_HEADER=9;// 定跡ファイルのデータエントリーにあるヘッダーのサイズ
	const unsigned int BK_SIZE_MOVE=4;// 定跡ファイルの指し手のサイズ
	const unsigned int BK_MAX_MOVE=32; //一局面に保存されている着手の最大数
	
	struct BookMove{
		uint16 z,freq;
	};
	
	struct BookEntry{
		uint64_t hash;
		BookMove mv[BK_MAX_MOVE];
	};
	
	class BookTT{
		
	};
	
	class OpeningBook{
		
	private:
		FILE *pf_book;
		int size;
	public:
		int on(){
			pf_book=fopen( "kanon_book.bin", "r" );
			if( pf_book == nullptr ){return -1;}
			return 0;
		}
		
		int onWriting(){
			pf_book=fopen( "kanon_book.bin", "r" );
			if( pf_book == nullptr ){return -1;}
			return 0;
		}
		
		int off(){
			if( pf_book != nullptr ){
				fclose( pf_book );
			}
		}
		
		template<class move_t,class board_t>
		void read(move_t *const mv,const board_t& bd,int col){
			uint64_t hash=bd.getFrontHash(col);
			const unsigned char *p;
			unsigned int position, size_section, size, u;
			int ibook_section, moves;
			unsigned short s;
			
			ibook_section = (int)( (unsigned int)hash & (unsigned int)( NUM_SECTION-1 ) );//位置
			
			//位置指示子をずらす
			if ( fseek( pf_book, BK_SIZE_INDEX*ibook_section, SEEK_SET ) == EOF ){//終端まで行った
				CERR<<"Book::read : failed to seek index."<<endl;
				return -2;
			}
			//索引を読む
			if ( fread( &position, sizeof(int), 1, pf_book ) != 1 )
			{
				str_error = str_io_error;
				return -2;
			}
			if ( fread( &s, sizeof(unsigned short), 1, pf_book ) != 1 )
			{
				str_error = str_io_error;
				return -2;
			}
			
			size_section = (unsigned int)s;
			if ( size_section > MAX_SIZE_SECTION )
			{
				str_error = str_book_error;
				return -2;
			}
			
			if ( fseek( pf_book, (long)position, SEEK_SET ) == EOF )
			{
				str_error = str_io_error;
				return -2;
			}
			if ( fread( book_section, sizeof(unsigned char), (size_t)size_section,
					   pf_book ) != (size_t)size_section )
			{
				str_error = str_io_error;
				return -2;
			}
		}
		
		OpeningBook()
		:pf_book(nullptr)
		{}
		
		~OpeningBook(){
			off();
		}
	};
	
	
}