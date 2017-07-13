/*
 go.hpp
 Katsuki Ohto
 */

// 囲碁
// 基本的な定数とデータ構造

#ifndef GO_GO_HPP_
#define GO_GO_HPP_

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <climits>
#include <cfloat>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <string>
#include <bitset>
#include <utility>
#include <tuple>
#include <cstdarg>
#include <random>
#include <array>
#include <vector>
#include <bitset>
#include <queue>
#include <algorithm>
#include <new>
#include <thread>
#include <mutex>
#include <atomic>

#include <strings.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#else

#include <sys/param.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

#endif

#include "settings.h"

#include "../../CppCommon/src/defines.h"
#include "../../CppCommon/src/util/bitOperation.hpp"
#include "../../CppCommon/src/util/bitArray.hpp"
#include "../../CppCommon/src/util/bitSet.hpp"
#include "../../CppCommon/src/util/math.hpp"
#include "../../CppCommon/src/util/xorShift.hpp"
#include "../../CppCommon/src/util/atomic.hpp"
#include "../../CppCommon/src/util/softmaxPolicy.hpp"
#include "../../CppCommon/src/util/lock.hpp"
#include "../../CppCommon/src/util/node.hpp"
#include "../../CppCommon/src/util/accessor.hpp"

namespace Go{

    /**************************ルール**************************/
    
    constexpr int KOMIx2 = RULE_KOMI2;
    enum{
        CHINESE = 0,
        JAPANESE,
    };
    
    /**************************色**************************/
    enum{
        EMPTY = 0,
        BLACK = 1,
        WHITE = 2,
        WALL = 3,
    };
    using Color = int;

    constexpr Color flipColor(Color c)noexcept{ return BLACK + WHITE - c; }
    
    constexpr bool isBlackOrWhite(Color c)noexcept{ return (c == BLACK || c == WHITE); }
    constexpr bool examColor(Color c)noexcept{ return isBlackOrWhite(c); }
    
    constexpr bool isBlack(Color c)noexcept{ return (c == BLACK); }
    constexpr bool isWhite(Color c)noexcept{ return (c == WHITE); }

    const char *const colorString[4] = { "EMPTY", "BLACK", "WHITE", "WALL " };
    
    constexpr int toColorIndex(Color c)noexcept{ return c - 1; }

    constexpr Color eraseWallColor(Color c)noexcept{ return c & 3; } // wallをemptyと同一にする
    
    struct OutColor{
        Color c;
        constexpr OutColor(Color ac):
        c(ac){}
    };

    std::ostream& operator<<(std::ostream& os, const OutColor& arg){
        os << colorString[arg.c]; return os;
    }
    
    template<class callback_t>
    void iterateColor3(const callback_t& callback)noexcept{
        callback(EMPTY);
        callback(BLACK);
        callback(WHITE);
    }
    
    /**************************手番**************************/
    
    constexpr Color toTurnColor(int turn, int handicapped = 0)noexcept{
        return Color(((turn % 2) ^ handicapped) + 1);
    }
    
    /**************************ルール違反**************************/
    
    enum{
        VALID = 0,
        LEGAL = 0,
        DOUBLE = 1,
        SUICIDE = 2,
        KO = 3,
        OUT = 4,
        EYE = 8,
    };
    
    constexpr bool isValid(int validity)noexcept{
        return !(validity & 7);
    }
    
    const char *const ruleString[EYE + 1] = {
        "VALID", "DOUBLE", "SUICIDE", "KO", "OUT", "", "", "", "EYE",
    };
    
    struct OutRule{
        int r;
        constexpr OutRule(int ar):
        r(ar){}
    };
    
    std::ostream& operator<<(std::ostream& os, const OutRule& arg){
        os << ruleString[arg.r]; return os;
    }
    
    /**************************盤上 0 ~ (size - 1) の整数座標**************************/
    
    constexpr int getBoardSize(int l)noexcept{
        return l * l;
    }
    
    // IX, IY, IZ系列
    constexpr int IXIYtoIZ(int ix, int iy, int l)noexcept{
        return ix * l + iy;
    }
    constexpr int IZtoIX(int iz, int l)noexcept{
        return iz / l;
    }
    constexpr int IZtoIY(int iz, int l)noexcept{
        return iz % l;
    }
    template<int L>
    constexpr int IXIYtoIZ(int ix, int iy)noexcept{
        return ix * L + iy;
    }
    template<int L>
    constexpr int IZtoIX(int iz)noexcept{
        return iz / L;
    }
    template<int L>
    constexpr int IZtoIY(int iz)noexcept{
        return iz % L;
    }
    
    static_assert(IXIYtoIZ(IZtoIX(23, 19), IZtoIY(23, 19), 19) == 23,
                  "IXIYtoIZ(19) <-> IZtoIX(19), IZtoIY(19) failed.");
    
    template<int L, class callback_t>
    void iterateIXIY(const callback_t& callback)noexcept{
        for(int ix = 0; ix < L; ++ix){
            for(int iy = 0; iy < L; ++iy){
                callback(ix, iy);
            }
        }
    }
    template<int L, class callback_t>
    void iterateIZ(const callback_t& callback)noexcept{
        for(int iz = 0; iz < getBoardSize(L); ++iz){
            callback(iz);
        }
    }
    
    template<int L>
    bool isOnBoardIXIY(int ix , int iy)noexcept{
        return (0 <= ix < L) && (0 <= iy < L);
    }
    template<int L>
    bool isOnBoardIZ(int iz)noexcept{
        return (0 <= iz < getBoardSize(L));
    }
    
    /**************************盤外あり、配列用の整数座標**************************/

    // X, Y, Z系列(盤外を1マス分以上取った位置番号)
    constexpr int BOARD_ARRAY_LENGTH = 32;
    constexpr int X_FIRST = 1;
    constexpr int Y_FIRST = 1;
    constexpr int getXLast(int l)noexcept{ return X_FIRST - 1 + l; }
    constexpr int getYLast(int l)noexcept{ return Y_FIRST - 1 + l; }
    
#define XYtoZ(x, y) ((x) * (BOARD_ARRAY_LENGTH) + (y))
#define ZtoX(z) ((z) / (BOARD_ARRAY_LENGTH))
#define ZtoY(z) ((z) % (BOARD_ARRAY_LENGTH))
    
    template<int L>
    constexpr int DXtoDZ(int dx)noexcept{ return BOARD_ARRAY_LENGTH * dx; }
    template<int L>
    constexpr int DYtoDZ(int dy)noexcept{ return dy; }
    
    constexpr int getZLast(int l)noexcept{
        return XYtoZ(getXLast(l), getYLast(l));
    }
    constexpr int getBoardArraySize(int L){
        return BOARD_ARRAY_LENGTH * (L + 2);
    }
    
    static_assert(XYtoZ(ZtoX(23), ZtoY(23)) == 23, "XYtoZ() <-> ZtoX(), ZtoY() failed.");
    
    // 系列変換
    constexpr int IXtoX(int ix)noexcept{ return ix + X_FIRST; }
    constexpr int IYtoY(int iy)noexcept{ return iy + Y_FIRST; }
    
    constexpr int XtoIX(int x)noexcept{ return x - X_FIRST; }
    constexpr int YtoIY(int y)noexcept{ return y - Y_FIRST; }
    
    constexpr int ZtoIX(int z)noexcept{ return XtoIX(ZtoX(z)); }
    constexpr int ZtoIY(int z)noexcept{ return YtoIY(ZtoY(z)); }
    
    constexpr int IZtoX(int iz, int l)noexcept{ return IXtoX(IZtoIX(iz, l)); }
    constexpr int IZtoY(int iz, int l)noexcept{ return IYtoY(IZtoIY(iz, l)); }
    
    constexpr int ZtoIZ(int z, int l)noexcept{
        return IXIYtoIZ(ZtoIX(z), ZtoIY(z), l);
    }
    constexpr int IZtoZ(int iz, int l)noexcept{
        return XYtoZ(IZtoX(iz, l), IZtoY(iz, l));
    }
    
    template<int L>
    constexpr int ZtoIZ(int z)noexcept{
        return IXIYtoIZ<L>(XtoIX(ZtoX(z)), YtoIY(ZtoY(z)));
    }
    template<int L>
    constexpr int IZtoZ(int iz)noexcept{
        return XYtoZ(IXtoX(IZtoIX<L>(iz)), IYtoY(IZtoIY<L>(iz)));
    }
    
    template<int L>
    constexpr int IXIYtoZ(int ix, int iy)noexcept{
        return XYtoZ(IXtoX(ix), IYtoY(iy));
    }
    
    // 値のチェック
    void assert_z_on(int z, int l){
        int ix = ZtoIX(z), iy = ZtoIY(z);
        ASSERT(0 <= ix && ix < l && 0 <= iy && iy < l, cerr << z << endl;);
    }
    void assert_z(int z, int l){
        int ix = ZtoIX(z), iy = ZtoIY(z);
        ASSERT(-1 <= ix && ix <= l && -1 <= iy && iy <= l, cerr << z << endl;);
    }
    void assert_xy_on(int x, int y, int l){
        int ix = XtoIX(x), iy = YtoIY(y);
        ASSERT(0 <= ix && ix < l && 0 <= iy && iy < l, cerr << x << ", " << y << endl;);
    }
    void assert_xy(int x, int y, int l){
        int ix = XtoIX(x), iy = YtoIY(y);
        ASSERT(-1 <= ix && ix <= l && -1 <= iy && iy <= l, cerr << x << ", " << y << endl;);
    }
    
    constexpr int STRING_SIZE_MAX_9 = 61;
    constexpr int getRenSizeMax(int L){
        return (L == 9) ? STRING_SIZE_MAX_9
        : (4 * L * (L - 1) / 5 + 4);
    }
    /*constexpr int getMCExtractLine(int L){
     return (L==9) ? MC_EXTRACT_LINE_9
     : MC_EXTRACT_LINE_19;
     }*/
    constexpr int getNTurnsMax(int L)noexcept{
        return L * L + 200;
    }
    

    static_assert(ZtoIZ(IZtoZ(23, 19), 19) == 23, "ZtoIZ(19) <-> IZtoZ(19) failed.");
    
    constexpr int Z_PASS = 0;
    constexpr int Z_NONE = -2;
    constexpr int Z_RESIGN = -1;
    
    constexpr int table_dir4[4] = {-32, -1, +1, +32,};
    constexpr int table_dir8[8] = {-33, -32, -31, -1, +1, +31, +32, +33,};
    
    template<int L = 19>
    constexpr int dir4(int i){ return table_dir4[i]; }
    template<int L = 19>
    constexpr int dir8(int i){ return table_dir8[i]; }
    
    constexpr bool isOnBoardXY(int x, int y, int l)noexcept{
        return (X_FIRST <= x && x <= getXLast(l))
        && (Y_FIRST <= y && y <= getYLast(l));
    }
    constexpr bool isOnBoardZ(int z, int l)noexcept{
        return isOnBoardXY(ZtoX(z), ZtoY(z), l);
    }
    template<int L>
    constexpr bool isOnBoardXY(int x, int y)noexcept{
        return (X_FIRST <= x && x <= getXLast(L))
        && (Y_FIRST <= y && y <= getYLast(L));
    }
    template<int L>
    constexpr bool isOnBoardZ(int z)noexcept{
        return isOnBoardXY<L>(ZtoX(z), ZtoY(z));
    }
    template<int L>
    constexpr bool isCornerZ(int z)noexcept{
        return (z == ZtoIZ((IXIYtoIZ<L>(0, 0))))
        || (z == ZtoIZ((IXIYtoIZ<L>(0, L - 1))))
        || (z == ZtoIZ((IXIYtoIZ<L>(L - 1, 0))))
        || (z == ZtoIZ((IXIYtoIZ<L>(L - 1, L - 1))));
    }
    template<int L>
    constexpr bool isEdgeXY(int x, int y)noexcept{
        return (x == IXtoX(0))
        || (x == IXtoX(L - 1))
        || (y == IYtoY(0))
        || (y == IYtoY(L - 1));
    }
    template<int L>
    constexpr bool isEdgeZ(int z)noexcept{
        const int x = ZtoX(z);
        const int y = ZtoY(z);
        return (x == IXtoX(0))
        || (x == IXtoX(L - 1))
        || (y == IYtoY(0))
        || (y == IYtoY(L - 1));
    }
    
    template<int L, class callback_t>
    void iterateXY(const callback_t& callback)noexcept{
        for(int x = X_FIRST; x <= getXLast(L); ++x){
            for(int y = Y_FIRST; y <= getYLast(L); ++y){
                callback(x, y);
            }
        }
    }
    template<int L, class callback_t>
    void iterateZ(const callback_t& callback)noexcept{
        for(int z = XYtoZ(X_FIRST, Y_FIRST); z <= XYtoZ(getXLast(L), getYLast(L)); z += DXtoDZ<L>(1)){
            for(int zend = z + DYtoDZ<L>(L); z <= zend; z += DYtoDZ<L>(1)){
                callback(z);
            }
        }
    }
    template<int L, class callback_t>
    void iterateWallZ(const callback_t& callback)noexcept{
        for(int y = Y_FIRST - 1; y <= getYLast(L) + 1; ++y){
            callback(XYtoZ(X_FIRST - 1, y));
        }
        for(int x = X_FIRST; x <= getXLast(L); ++x){
            callback(XYtoZ(x, Y_FIRST - 1));
            callback(XYtoZ(x, getYLast(L) + 1));
        }
        for(int y = Y_FIRST - 1; y <= getYLast(L) + 1; ++y){
            callback(XYtoZ(getXLast(L) + 1, y));
        }
    }
    
    char* XtoSTR(char *str, int x){
        int ix = XtoIX(x);
        char cx = ix + 'A';
        if(cx >= 'I'){ ++cx; }
        *str = cx; str++;
        return str;
    }
    
    char* YtoSTR(char *str, int y){
        int iy = YtoIY(y);
        int sy = iy + 1;
        int sy10 = sy / 10;
        if(sy10){
            *str = (char)(sy10 + '0'); str++;
            sy = sy % 10;
        }
        *str = (char)(sy + '0'); str++;
        return str;
    }
    
    char* ZtoSTR(char *str, int z){
        int x = ZtoX(z), y = ZtoY(z);
        str = XtoSTR(str, x);
        str = YtoSTR(str, y);
        return str;
    }
    
    struct OutZ{
        int z;
        OutZ(int az):z(az){}
    };
    
    std::ostream& operator<<(std::ostream& out, const OutZ& arg){
        if(arg.z == Z_PASS){
            out << "pass";
        }else{
            int x = ZtoX(arg.z), y = ZtoY(arg.z);
            char cx = (x + 'A');
            if(cx >= 'I'){ ++cx; }
            out << cx << y;
        }
        return out;
    }
    
    template<int L>
    int symmetryZ(int z, int sym){
        switch(sym){
            case 0: return z; break; // normal
            case 1: return XYtoZ(X_FIRST + getXLast(L) - ZtoX(z), ZtoY(z)); break; // -x
            case 2: return XYtoZ(ZtoX(z), Y_FIRST + getYLast(L) - ZtoY(z)); break; // -y
            case 3: return XYtoZ(X_FIRST + getXLast(L) - ZtoX(z), Y_FIRST + getYLast(L) - ZtoY(z)); break; // -x,-y
            case 4: return XYtoZ(ZtoY(z), ZtoX(z)); break; // x <-> y
            case 5: return XYtoZ(Y_FIRST + getYLast(L) - ZtoY(z), ZtoX(z)); break;
            case 6: return XYtoZ(ZtoY(z), X_FIRST + getXLast(L) - ZtoX(z)); break;
            case 7: return XYtoZ(Y_FIRST + getYLast(L) - ZtoY(z), X_FIRST + getXLast(L) - ZtoX(z)); break;
            default: UNREACHABLE; break;
        }
    }
    
    /**************************周囲の座標(詰め座標)**************************/
    
    constexpr int invertAroundIndex4I(int ai)noexcept{ return 3 - ai; }
    constexpr int invertAroundIndex8I(int ai)noexcept{ return 7 - ai; }
    
    template<int L, class callback_t>
    void iterateAround4I(int iz, const callback_t& callback)noexcept{
        callback(iz + L);
        callback(iz - 1);
        callback(iz + 1);
        callback(iz + L);
    }

    /**************************周囲の座標**************************/
    
    constexpr int invertAroundIndex4(int ai)noexcept{ return 3 - ai; }
    constexpr int invertAroundIndex8(int ai)noexcept{ return 7 - ai; }
    
    template<class callback_t>
    void iterateAround4(int z, const callback_t& callback)noexcept{
        for(int i = 0; i < 4; ++i){ callback(z + dir4(i)); }
    }
    template<class callback_t>
    void iterateAround4Unrolled(int z, const callback_t& callback)noexcept{
        callback(z + dir4(0));
        callback(z + dir4(1));
        callback(z + dir4(2));
        callback(z + dir4(3));
    }
    template<class callback_t>
    void iterateAround4WithIndex(int z, const callback_t& callback)noexcept{
        for(int i = 0; i < 4; ++i){ callback(i, z + dir4(i)); }
    }
    template<class callback_t>
    void iterateAround4WithIndexUnrolled(int z, const callback_t& callback)noexcept{
        callback(0, z + dir4(0));
        callback(1, z + dir4(1));
        callback(2, z + dir4(2));
        callback(3, z + dir4(3));
    }
    
    template<class callback_t>
    void iterateAround4(int x, int y, const callback_t& callback)noexcept{
        callback(x - 1, y);
        callback(x, y - 1);
        callback(x, y + 1);
        callback(x + 1, y);
    }
    template<class callback_t>
    void iterateAround4WithIndex(int x, int y, const callback_t& callback)noexcept{
        callback(0, x - 1, y);
        callback(1, x, y - 1);
        callback(2, x, y + 1);
        callback(3, x + 1, y);
    }
    template<class callback_t>
    void iterateAround8(int z, const callback_t& callback)noexcept{
        for(int i = 0; i < 8; ++i){ callback(z + dir8(i)); }
    }
    template<class callback_t>
    void iterateAround8Unrolled(int z, const callback_t& callback)noexcept{
        callback(z + dir8(0));
        callback(z + dir8(1));
        callback(z + dir8(2));
        callback(z + dir8(3));
        callback(z + dir8(4));
        callback(z + dir8(5));
        callback(z + dir8(6));
        callback(z + dir8(7));
    }
    template<class callback_t>
    void iterateAround8WithIndex(int z, const callback_t& callback)noexcept{
        for(int i = 0; i < 8; ++i){ callback(i, z + dir8(i)); }
    }
    template<class callback_t>
    void iterateAround8WithIndexUnrolled(int z, const callback_t& callback)noexcept{
        callback(0, z + dir8(0));
        callback(1, z + dir8(1));
        callback(2, z + dir8(2));
        callback(3, z + dir8(3));
        callback(4, z + dir8(4));
        callback(5, z + dir8(5));
        callback(6, z + dir8(6));
        callback(7, z + dir8(7));
    }
    template<class callback_t>
    void iterateAround8(int x, int y, const callback_t& callback)noexcept{
        callback(x - 1, y - 1);
        callback(x - 1, y);
        callback(x - 1, y + 1);
        callback(x, y - 1);
        callback(x, y + 1);
        callback(x + 1, y - 1);
        callback(x + 1, y);
        callback(x + 1, y + 1);
    }
    template<class callback_t>
    void iterateAround8WithIndex(int x, int y, const callback_t& callback)noexcept{
        callback(0, x - 1, y - 1);
        callback(1, x - 1, y);
        callback(2, x - 1, y + 1);
        callback(3, x, y - 1);
        callback(4, x, y + 1);
        callback(5, x + 1, y - 1);
        callback(6, x + 1, y);
        callback(7, x + 1, y + 1);
    }
    
    /**************************座標アルゴリズム**************************/
    
    template<int L>
    constexpr int calcDistanceToEndIXIY(int ix, int iy)noexcept{
        return min(min(ix, (L - 1 - ix)), min(iy, (L - 1 - iy)));
    }
    
    template<int L, int SL>
    constexpr int ItoSmallerBoardI(int i)noexcept{
        return (i < L / 2) ?
        ((i > SL / 2) ? (SL / 2) : i) :
        ((L - 1 - i > SL / 2) ? (SL - 1 - SL / 2) : (SL - 1 - (L - 1 - i)));
    }
    
    static_assert(ItoSmallerBoardI<19, 5>(1) == 1, "ItoSmallerBoardI<19, 5>(1) -> 1 failed");
    static_assert(ItoSmallerBoardI<19, 5>(2) == 2, "ItoSmallerBoardI<19, 5>(2) -> 2 failed");
    static_assert(ItoSmallerBoardI<19, 5>(3) == 2, "ItoSmallerBoardI<19, 5>(3) -> 2 failed");
    static_assert(ItoSmallerBoardI<19, 5>(15) == 2, "ItoSmallerBoardI<19, 5>(15) -> 2 failed");
    static_assert(ItoSmallerBoardI<19, 5>(16) == 2, "ItoSmallerBoardI<19, 5>(16) -> 2 failed");
    static_assert(ItoSmallerBoardI<19, 5>(17) == 3, "ItoSmallerBoardI<19, 5>(17) -> 3 failed");
    
    /**************************座標（2次元）**************************/
    
    struct Point{
        int x, y;
        
        void operator=(const Point& p)noexcept{
            x = p.x; y = p.y;
        }
        constexpr bool isPass()const noexcept{ return (x == 0 && y == 0); }
        
        constexpr Point():x(), y(){}
        
        constexpr Point(const Point& p)
        :x(p.x), y(p.y){}
        
        constexpr Point(const int ax, const int ay)
        :x(ax), y(ay){}
        
        constexpr Point(const int az)
        :x(ZtoX(az)), y(ZtoY(az)){}
        
        void setPass()noexcept{
            x = 0; y = 0;
        }
    };
    
    std::ostream& operator<<(std::ostream& out, const Point& arg){
        char str[16];
        char *tmp = str;
        tmp = XtoSTR(tmp, arg.x);
        tmp = YtoSTR(tmp, arg.y);
        *tmp = '\0';
        out << str;
        return out;
    }
    
    constexpr int PtoZ(const Point& p)noexcept{
        return (p.isPass() ? Z_PASS : XYtoZ(p.x, p.y));
    }

    /**************************制限時間**************************/

    struct TimeLimit{
        // ミリ秒単位
        // もし秒単位でのみ扱われる試合なら、注意
        
        constexpr static uint64_t UNLIMITED_ = 0xffffffffffffffff;
        uint64_t limit_ms; // 持ち時間ミリ秒
        uint64_t byoyomi_ms; // 秒読みミリ秒

        void clear()noexcept{
            limit_ms = 0ULL; byoyomi_ms = 0ULL;
        }
        
        void set(uint64_t aleft, uint64_t aby = 0ULL)noexcept{
            limit_ms = aleft;
            byoyomi_ms = aby;
        }
        void setUnlimited()noexcept{
            limit_ms = UNLIMITED_;
            byoyomi_ms = 0ULL;
        }
        
        void feed(uint64_t aused)noexcept{ // 使用した時間を引く
            if(isUnlimited()){ // 時間無制限
                //何もしない
            }else if(aused >= limit_ms){ // 制限時間を使い切った
                limit_ms = 0ULL;
            }else{
                limit_ms -= aused;
            }
        }
        bool isUnlimited()const noexcept{ // 時間無制限
            return (limit_ms == UNLIMITED_);
        }
        bool isNoTime()const noexcept{ // ノータイム(出来る限り早く出すべき時)
            return false;
        }
    };
    
    /**************************盤面ハッシュ値**************************/
    
    constexpr int BOARD_ARRAY_SIZE_MAX = getBoardArraySize(19);
    
    // ハッシュテーブル
    uint64_t hash_table_stone[BOARD_ARRAY_SIZE_MAX][4];
    uint64_t hash_table_ko[BOARD_ARRAY_SIZE_MAX];
    
    // ハッシュ関連の初期化
    void initHash()noexcept{
        
        constexpr uint32_t seed = 0x0e4a0fe9;
        XorShift64 dice;
        
        dice.srand(seed);
        
        for(int z = 0; z < BOARD_ARRAY_SIZE_MAX; ++z){
            hash_table_stone[z][EMPTY] = 0ULL;
            hash_table_stone[z][BLACK] = dice.rand();
            hash_table_stone[z][WHITE] = dice.rand();
            hash_table_stone[z][WALL] = dice.rand();
            hash_table_ko[z] = dice.rand();
        }
    }
    
    template<int L>
    void initHashx8(uint64_t *const phash){
        for(int i = 0; i < 8; ++i){
            phash[i] = 0ULL;
        }
    }
    
    template<int L, class hash_table_t>
    void procHashx8(uint64_t *const phash, Color c, int z,
                    const hash_table_t& hash_table){
        // 8方向のハッシュ値を同時に更新する
        int x(ZtoX(z)), y(ZtoY(z));
        
        constexpr int X_LAST = getXLast(L);
        constexpr int Y_LAST = getYLast(L);
        
        phash[0] ^= hash_table[z][c]; // normal
        phash[1] ^= hash_table[XYtoZ(X_FIRST + X_LAST - x, y)][c]; // -x
        phash[2] ^= hash_table[XYtoZ(x, Y_FIRST+Y_LAST - y)][c]; // -y
        phash[3] ^= hash_table[XYtoZ(X_FIRST + X_LAST - x, Y_FIRST + Y_LAST - y)][c]; // -x,-y
        phash[4] ^= hash_table[XYtoZ(y, x)][c]; // x <-> y
        phash[5] ^= hash_table[XYtoZ(Y_FIRST + Y_LAST - y, x)][c];
        phash[6] ^= hash_table[XYtoZ(y, X_FIRST + X_LAST - x)][c];
        phash[7] ^= hash_table[XYtoZ(Y_FIRST + Y_LAST - y, X_FIRST + X_LAST - x)][c];
        
        ASSERT(hash_table[z][c],); // normal
        ASSERT(hash_table[XYtoZ(X_FIRST + X_LAST - x, y)][c],); // -x
        ASSERT(hash_table[XYtoZ(x, Y_FIRST+Y_LAST - y)][c],); // -y
        ASSERT(hash_table[XYtoZ(X_FIRST + X_LAST - x, Y_FIRST + Y_LAST - y)][c],); // -x,-y
        ASSERT(hash_table[XYtoZ(y, x)][c],); // x <-> y
        ASSERT(hash_table[XYtoZ(Y_FIRST + Y_LAST - y, x)][c],);
        ASSERT(hash_table[XYtoZ(y, X_FIRST + X_LAST - x)][c],);
        ASSERT(hash_table[XYtoZ(Y_FIRST + Y_LAST - y, X_FIRST + X_LAST - x)][c],);
    }
    template<int L, class hash_table_t>
    void unprocHashx8(uint64_t *const phash, Color c, int z,
                      const hash_table_t& hash_table){
        procHashx8<L>(phash, c, z, hash_table);
    }
    
    
    template<int L>
    void procKoHashx8(uint64_t *const phash, int z){
        // 8方向のコウのハッシュ値を同時に更新する
        int x(ZtoX(z)), y(ZtoY(z));
        
        constexpr int X_LAST = getXLast(L);
        constexpr int Y_LAST = getYLast(L);
        
        phash[0] ^= hash_table_ko[z]; // normal
        phash[1] ^= hash_table_ko[XYtoZ(X_FIRST + X_LAST - x, y)]; // -x
        phash[2] ^= hash_table_ko[XYtoZ(x, Y_FIRST + Y_LAST - y)]; // -y
        phash[3] ^= hash_table_ko[XYtoZ(X_FIRST + X_LAST - x, Y_FIRST + Y_LAST - y)]; // -x, -y
        phash[4] ^= hash_table_ko[XYtoZ(y, x)]; // x <-> y
        phash[5] ^= hash_table_ko[XYtoZ(Y_FIRST + Y_LAST - y, x)];
        phash[6] ^= hash_table_ko[XYtoZ(y, X_FIRST + X_LAST - x)];
        phash[7] ^= hash_table_ko[XYtoZ(Y_FIRST + Y_LAST - y, X_FIRST + X_LAST - x)];
    }
    
    uint64_t pickFrontByHashx8(const uint64_t *const phash){
        // 8方向のハッシュ値のうち代表1つを選ぶ
        return min(
                   min(min(phash[0], phash[1]),
                       min(phash[2], phash[3])),
                   min(min(phash[4], phash[5]),
                       min(phash[6], phash[7]))
                   );
    }

    /**************************簡単な局面表現**************************/
    
    template<int L>
    class EasyBoard{
    private:
        constexpr static int BOARD_LENGTH_ = L;
        constexpr static int BOARD_ARRAY_SIZE_ = getBoardArraySize(L);
        
    public:
        constexpr static int X_FIRST_ = X_FIRST;
        constexpr static int Y_FIRST_ = Y_FIRST;
        constexpr static int X_LAST_ = getXLast(L);
        constexpr static int Y_LAST_ = getYLast(L);
        
        constexpr static int length()noexcept{ return BOARD_LENGTH_; }
        
        int turn, lastZ, koZ, handicap;
        Color lastColor;
        int captured[2];
        TimeLimit limit[2];
        TimeLimit orgLimit[2]; // 最初の時間
        
        uint64_t hash_stone_[8]; // 盤面ハッシュ値
        
        Color color(int z)const{ assert_z(z, length()); return cell_[z]; }
        Color color(int x, int y)const{ assert_xy(x, y, length()); return cell_[XYtoZ(x, y)]; }
        
        int getTurn()const noexcept{
            return turn;
        }
        Color getTurnColor()const noexcept{
            return toTurnColor(turn , (handicap > 0));
        }
        
        template<class callback_t>
        void iterateXY(const callback_t& callback)noexcept{
            for(int x = X_FIRST_; x <= X_LAST_; ++x){
                for(int y = Y_FIRST_; y <= Y_LAST_; ++y){
                    callback(x, y);
                }
            }
        }
        template<class callback_t>
        void iterateXY(const callback_t& callback)const noexcept{
            for(int x = X_FIRST_; x <= X_LAST_; ++x){
                for(int y = Y_FIRST_; y <= Y_LAST_; ++y){
                    callback(x, y);
                }
            }
        }
        template<class callback_t>
        void iterateZ(const callback_t& callback)noexcept{
            for(int x = X_FIRST_; x <= X_LAST_; ++x){
                for(int y = Y_FIRST_; y <= Y_LAST_; ++y){
                    callback(XYtoZ(x, y));
                }
            }
        }
        template<class callback_t>
        void iterateZ(const callback_t& callback)const noexcept{
            for(int x = X_FIRST_; x <= X_LAST_; ++x){
                for(int y = Y_FIRST_; y <= Y_LAST_; ++y){
                    callback(XYtoZ(x, y));
                }
            }
        }
        void initInfo()noexcept{
            turn = 0;
            handicap = 0;
            for(int ci = 0; ci < 2; ++ci){
                captured[ci] = 0;
                limit[ci].clear();
                orgLimit[ci].clear();
            }
            koZ = 0;
            lastZ = Z_NONE;
            initHashx8<L>(hash_stone_);
        }
        void init()noexcept{
            // init cells
            for(int z = 0; z < BOARD_ARRAY_SIZE_; ++z){
                cell_[z] = WALL;
            }
            for(int x = X_FIRST_; x <= X_LAST_; ++x){
                for(int y = Y_FIRST_; y <= Y_LAST_; ++y){
                    cell_[XYtoZ(x, y)] = EMPTY;
                }
            }
            lastColor = WHITE;
            initInfo();
        }
        void setTimeLimit(Color c, uint64_t at, uint64_t aby)noexcept{
            limit[toColorIndex(c)].set(at, aby);
            orgLimit[toColorIndex(c)].set(at, aby);
        }
        void setTimeLimit(uint64_t at, uint64_t aby)noexcept{
            for(int c = 0; c < 2; ++c){
                limit[c].set(at, aby);
                orgLimit[c].set(at, aby);
            }
        }
        void feedUsedTime(Color c, uint64_t at){
            limit[toColorIndex(c)].feed(at);
        }
        
        template<class stone_list>
        void init(const stone_list& ah)noexcept{
            // init cells
            for(int z = 0; z < BOARD_ARRAY_SIZE_; ++z){
                cell_[z] = WALL;
            }
            for(int x = X_FIRST_; x <= X_LAST_; ++x){
                for(int y = Y_FIRST_; y <= Y_LAST_; ++y){
                    cell_[XYtoZ(x, y)] = EMPTY;
                }
            }
            for(auto z : ah){
                move(BLACK, z);
            }
            lastColor = BLACK;
            initInfo();
        }
        void initNoWall()noexcept{
            // init cells
            for(int z = 0; z < BOARD_ARRAY_SIZE_; ++z){
                cell_[z] = EMPTY;
            }
            lastColor = WHITE;
            initInfo();
        }
        int remove(const Color col, const int z){
            // 連となっている石を全て取り除く
            if(color(z) != col){ return -1; }
            cell_[z] = EMPTY;
            procHashx8<L>(hash_stone_, col, z, hash_table_stone);
            iterateAround4(z, [this, col](int tz)->void{
                if(color(tz) == col){ remove(col, tz); }
            });
            return 0;
        }
        int check(const Color col, const int z)const{
            if(z == Z_PASS){ // パス
                return VALID;
            }
            if(!isOnBoardZ<length()>(z)){ // 盤外
                return OUT;
            }
            if(color(z) != EMPTY){ // 二重置き
                return DOUBLE;
            }
            if(z == koZ){ return KO; } // コウ
            struct AroundInfo{
                Color c;
                int lib, str;
            };
            
            AroundInfo around[4];
            const Color oppColor = flipColor(col);
            
            // 4方向のダメの数と石数
            int empty = 0; // 空白
            int wall = 0; // 盤外
            int aliveBuddies = 0;
            int capture = 0; // 取る石の数
            int koCandidate = 0; // コウかもしれない場所
            
            for(int i = 0; i < 4; ++i){
                const int tz = z + dir4(i);
                const Color c = color(tz);
                if(c == EMPTY){ ++empty; }
                if(c == WALL){ ++wall; }
                
                around[i].c = c;
                
                if(!examColor(c)){
                    // 盤外や空点には呼吸点は無いし連でもない
                    around[i].lib = 0;
                    around[i].str = 0;
                    continue;
                }
                int tlib, tstr;
                // 呼吸点と連のサイズを数える
                countLibertyAndString(tz, &tlib, &tstr);
                around[i].lib = tlib;
                around[i].str = tstr;
                
                if(c == oppColor && tlib == 1){
                    // 元々呼吸点が1つなので、取ることが出来る
                    // 取った場合には、コウになる可能性がある
                    capture += tstr; koCandidate = tz;
                }
                if(c == col && tlib >= 2){
                    // 呼吸点が2つ以上あるので、同じ色の生きた石である
                    ++aliveBuddies;
                }
            }
            
            if(capture == 0 && empty == 0 && aliveBuddies == 0){
                // 石を取るわけでも、周りに空点もなく、生きた味方とつながるわけでもないので自殺手
                return SUICIDE; // 自殺手
            }
            if(wall + aliveBuddies == 4){
                // 周囲が味方で囲われているので眼
                return EYE; // 眼
            }
            return VALID;
        }
        
        template<int FORCE = _NO>
        int move(const Color col, const int z){
            
            //int validity = check(col, z);
            //if(validity != VALID){ return validity; } // 非合法
            //cerr << ZtoIX(z) << ", " << ZtoIY(z) << endl;
            if(z == Z_PASS){ // パス
                if(koZ != Z_PASS){
                    // コウのハッシュ値を戻しておく
                    procKoHashx8<L>(hash_stone_, koZ);
                }
                koZ = Z_PASS; lastZ = z; ++turn; return VALID;
            }
            if(!isOnBoardZ<length()>(z)){ // 盤外
                //cerr << "bangai! " << z << endl; getchar();
                return OUT;
            }
            if(color(z) != EMPTY){ // 二重置き
                return DOUBLE;
            }
            if(z == koZ){ // コウ
                return KO;
            }
            
            struct AroundInfo{
                Color c;
                int z;
                int lib, str;
            };
            
            AroundInfo around[4]; // 周辺のマスの連の情報
            
            const Color oppColor = flipColor(col);
            
            // 4方向のダメの数と石数
            int empty = 0; // 空白
            int wall = 0; // 盤外
            int aliveBuddies = 0;
            int capture = 0; // 取る石の数
            int koCandidate = 0; // コウかもしれない場所
            
            for(int i = 0; i < 4; ++i){
                const int tz = z + dir4(i);
                const Color c = color(tz);
                if(c == EMPTY){ ++empty; }
                if(c == WALL){ ++wall; }
                
                around[i].c = c;
                around[i].z = tz;
                
                if(!examColor(c)){
                    // 盤外や空点には呼吸点は無いし連でもない
                    around[i].lib = 0;
                    around[i].str = 0;
                    continue;
                }
                int tlib, tstr;
                // 呼吸点と連のサイズを数える
                countLibertyAndString(tz, &tlib, &tstr);
                around[i].lib = tlib;
                around[i].str = tstr;
                
                if(c == oppColor && tlib == 1){
                    // 元々呼吸点が1つなので、取ることが出来る
                    // 取った場合には、コウになる可能性がある
                    capture += tstr; koCandidate = tz;
                }
                if(c == col && tlib >= 2){
                    // 呼吸点が2つ以上あるので、同じ色の生きた石である
                    ++aliveBuddies;
                }
            }
            
            if(capture == 0 && empty == 0 && aliveBuddies == 0){
                // 石を取るわけでも、周りに空点もなく、生きた味方とつながるわけでもないので自殺手
                return SUICIDE; // 自殺手
            }
            if(FORCE == _NO && wall + aliveBuddies == 4){
                // 周囲が味方で囲われているので眼
                return EYE; // 眼
            }
            
            // 石を取る操作
            for(int i = 0; i < 4; ++i){
                const int tz = around[i].z;
                if(color(tz) == oppColor){ // 既に消した石を再び消さないように毎回チェック
                    if(around[i].lib == 1){ // 呼吸点が1つなので、今置く石で取られる
                        remove(oppColor, tz);
                        captured[toColorIndex(col)] += around[i].str;
                    }
                }
            }
            
            // 石を置く
            cell_[z] = col;
            lastZ = z;
            
            // ハッシュ値更新
            if(koZ != Z_PASS){
                // コウのハッシュ値を戻しておく
                procKoHashx8<L>(hash_stone_, koZ);
            }
            procHashx8<L>(hash_stone_, col, z, hash_table_stone);
            
            // コウかどうかを判断
            int lib, str;
            countLibertyAndString(z, &lib, &str);
            
            if(capture == 1 && lib == 1 && str == 1){
                // 石を1つ取って、サイズ1、呼吸点1の連が出来るならば、コウになる
                koZ = koCandidate;
                if(koCandidate != Z_PASS){
                    // コウのハッシュ値
                    procKoHashx8<L>(hash_stone_, koCandidate);
                }
            }else{
                koZ = Z_PASS;
            }
            ++turn;
            return VALID;
        }
        int move(int z)noexcept{
            return move(getTurnColor(), z);
        }
        
        int countLiberty(const int z)const{
            int lib, str;
            countLibertyAndString(z, &lib, &str);
            return lib;
        }
        void countLibertyAndString(const int z, int *const plib, int *const pstr)const{
            std::bitset<BOARD_ARRAY_SIZE_> checkedFlag;
            checkedFlag.reset();
            *plib = 0;
            *pstr = 0;
            countLibertyAndStringSub(&checkedFlag, color(z), z, plib, pstr);
        }
    private:
        void countLibertyAndStringSub(std::bitset<BOARD_ARRAY_SIZE_>*const pflag,
                                      const Color col, const int z,
                                      int *const plib, int *const pstr)const{
            pflag->set(z);
            ++(*pstr);
            for(int i = 0; i < 4; ++i){
                const int tmpZ = z + dir4(i);
                if(pflag->test(tmpZ)){ continue; } // チェック済
                if(color(tmpZ) == EMPTY){
                    pflag->set(tmpZ);
                    ++(*plib); // 隣が空なので呼吸点
                }else if(color(tmpZ) == col){
                    // 隣が同じ色の石ならば連になるので、さらに探索
                    countLibertyAndStringSub(pflag, col, tmpZ, plib, pstr);
                }
            }
        }
        int cell_[BOARD_ARRAY_SIZE_];
    };
    
    // 出力
    template<int L>
    std::ostream& operator<<(std::ostream& out, const EasyBoard<L>& arg){
        const char *const str[6] = { ".", "●", "○", ".", "■", "□", };
        using board_t = EasyBoard<L>;
        for(int y = board_t::Y_LAST_; y >= board_t::Y_FIRST_; --y){
            int iy = YtoIY(y);
            if(y < 10){
                out << (iy + 1) << ' ';
            }else{
                out << (iy + 1);
            }
            for(int x = board_t::X_FIRST_; x <= board_t::X_LAST_; ++x){
                int z = XYtoZ(x,y);
                if(z == arg.lastZ){
                    out << ' ' << str[arg.color(z) + 3];
                }else{
                    out << ' ' << str[arg.color(z)];
                }
            }
            out << endl;
        }
        out << "  ";
        for(int x = board_t::X_FIRST_; x <= board_t::X_LAST_; ++x){
            char cx = (XtoIX(x) + 'A');
            if(cx >= 'I'){ ++cx; }
            out << ' ' << cx;
        }
        out << endl;
        out << "Turn : " << arg.turn << "  Turn-Color : " << OutColor(arg.getTurnColor()) << "  Ko : " ;
        if(arg.koZ == Z_PASS){
            out << "none";
        }else{
            out << Point(arg.koZ);
        }
        out << endl;
        out << "handicap : " << arg.handicap << endl;
        return out;
    }
    
    /**************************簡単な着手表現**************************/
    
    struct Move{
        int z;
        
        void set(int az)noexcept{ z = az; }
        void setPass()noexcept{ z = Z_PASS; }
        bool isPass()const noexcept{ return (z == Z_PASS); }
        int toZ()const noexcept{ return z; }
    };
    
    /**************************情報を持った着手表現**************************/
    
    struct MoveInfo{
        int z;
        
        void set(int az)noexcept{ z = az; }
        void setPass()noexcept{ z = Z_PASS; }
        bool isPass()const noexcept{ return (z == Z_PASS); }
        int toZ()const noexcept{ return z; }
    };
    
    /**************************基本のアルゴリズム**************************/
    
    template<class board_t, class callback_t>
    void iterateXY(const board_t& bd, const callback_t& callback)noexcept{
        for(int x = board_t::X_FIRST_; x <= board_t::X_LAST_; ++x){
            for(int y = board_t::Y_FIRST_; y <= board_t::Y_LAST_; ++y){
                callback(x, y);
            }
        }
    }
    template<class board_t, class callback_t>
    void iterateZ(const board_t& bd, const callback_t& callback)noexcept{
        for(int x = board_t::X_FIRST_; x <= board_t::X_LAST_; ++x){
            for(int y = board_t::Y_FIRST_; y <= board_t::Y_LAST_; ++y){
                callback(XYtoZ(x, y));
            }
        }
    }

    template<class board_t>
    int countChineseScore2(const board_t& bd)noexcept{
        int score = 0;
        int kind[3]; // empty, black, white
        kind[BLACK] = kind[WHITE] = 0;
        iterateZ(bd, [&score, &kind, &bd](const int z)->void{
            const Color c = bd.color(z);
            ++kind[c];
            if(c != EMPTY){ return; }
            int mk[4];
            mk[BLACK] = mk[WHITE] = 0;
            iterateAround4(z, [&mk, &bd](const int tz)->void{
                ++mk[bd.color(tz)];
            });
            if(mk[BLACK] && (mk[WHITE] == 0)){
                ++score;
            }else if(mk[WHITE] && (mk[BLACK] == 0)){
                --score;
            }
        });
        score += kind[BLACK] - kind[WHITE];
        return score * 2 - KOMIx2;
    }
    
    template<class move_t, class board_t>
    int genAllMoves(move_t *const mv0, const Color c, const board_t& bd){
        // 全ての合法着手を生成して合法着手数を返す
        move_t *mv = mv0;
        iterateZ(bd, [&](int z)->void{
            if(isValid(bd.check(c, z))){
                mv->set(z); ++mv;
            }
        });
        mv->setPass(); ++mv;
        return mv - mv0;
    }
    template<class move_t, class board_t>
    int genAllMovesWithoutEye(move_t *const mv0, const Color c, const board_t& bd){
        // 眼以外の全ての合法着手を生成して合法着手数を返す
        move_t *mv = mv0;
        iterateZ(bd, [&](int z)->void{
            if(bd.check(c, z) == VALID){
                mv->set(z); ++mv;
            }
        });
        mv->setPass(); ++mv;
        return mv - mv0;
    }
    
    template<class move_t, class board_t>
    int genAllMoves(move_t *const mv0, const board_t& bd){
        return genAllMoves(mv0, bd.getTurnColor(), bd);
    }
    
    /**************************初期化**************************/
    
    struct GeneralInitializer{
        GeneralInitializer(){
            initHash();
        }
    };
    
    GeneralInitializer genIni;
}

#endif // GO_GO_HPP_