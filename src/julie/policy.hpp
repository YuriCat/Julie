/*
 policy.hpp
 Katsuki Ohto
 */

#ifndef GO_JULIE_POLICY_HPP_
#define GO_JULIE_POLICY_HPP_

#include "julie_go.hpp"

namespace Go{
    
    /**************************着手方策**************************/
    
    namespace PolicySpace{
        enum{
            POL_PASS = 0,
            POL_DIST_WALL,
            POL_2x2,
            POL_3x3,
            FEA_ALL,
        };
        
        constexpr int numTable[FEA_ALL] = {
            2, // POL_PASS
            10 * 20, // POL_DIST_WALL
            0, //ipow(3, 4) * // POL_2x2
            2 * 25 * 9 * ipow(3, 8), // POL_3x3
            
        };
        constexpr int FEA_NUM(unsigned int fea){
            return numTable[fea];
        }
        constexpr int FEA_IDX(unsigned int fea){
            return (fea == 0) ? 0 : (FEA_IDX(fea - 1) + FEA_NUM(fea - 1));
        }
        constexpr int FEA_NUM_ALL = FEA_IDX(FEA_ALL);
        
#define LINEOUT(feature, str) {out << str << endl; int base = FEA_IDX(feature); for (int i = 0; i < FEA_NUM(feature); ++i){ os(base + i); }out << endl;}
        
#define LINEOUTX(feature, str, x) { out << str << endl; int base = FEA_IDX(feature); int num = FEA_NUM(feature);\
        for (int i = 0;;){os(base + i); ++i; if(i >= num)break; if(i % (x) == 0){ out << endl; }} out << endl; }
            
            int commentToPolicyParam(std::ostream& out, const double param[FEA_NUM_ALL]){
                auto os = [&out, param](int idx)->void{ out << param[idx] << " "; };
                
                out << "****** POLICY ******" << endl;
                LINEOUT(POL_PASS, "PASS");
                LINEOUTX(POL_DIST_WALL, "DIST_WALL", 10);
                //LINEOUT(,);
                return 0;
            }
#undef LINEOUT
        }
            
        using LinearPolicy = SoftmaxPolicy<PolicySpace::FEA_NUM_ALL, 1>;
        using LinearPolicyLearner = SoftmaxPolicyLearner<LinearPolicy>;
        
        int foutComment(const LinearPolicy& pol, const std::string& fName){
            std::ofstream ofs(fName, std::ios::out);
            return PolicySpace::commentToPolicyParam(ofs, pol.param_);
        }
            
#define Foo(i) {s += pol.param(i); pol.template feedFeatureScore<M>((i), 1.0);}
            
#define FooX(i, x) {s += pol.param(i) * (x); pol.template feedFeatureScore<M>((i), (x));}
            
#define FooC(p, i) {ASSERT(FEA_IDX(p) <= i && i < FEA_IDX((p) + 1),\
            cerr << "FooC() : illegal index " << (i) << " in " << FEA_IDX(p) << "~" << (FEA_IDX((p) + 1) - 1);\
            cerr << " (pi = " << (p) << ")" << endl; );\
            s += pol.param(i); pol.template feedFeatureScore<M>((i), 1.0);}
            
#define FooXC(p, i, x) {ASSERT(FEA_IDX(p) <= i && i < FEA_IDX((p) + 1),\
            cerr << "FooXC() : illegal index " << (i) << " in " << FEA_IDX(p) << "~" << (FEA_IDX((p) + 1) - 1);\
            cerr << " (pi = " << (p) << ")" << endl; );\
            s += pol.param(i) * (x); pol.template feedFeatureScore<M>((i), (x));}
            
            template<int M = 0, class move_t, class board_t, class policy_t>
            int calcPolicyScoreSlow(double *const dst,
                                    move_t *const buf,
                                    const int NMoves,
                                    const Color myColor,
                                    const board_t& bd,
                                    const policy_t& pol){
                
                using namespace PolicySpace;
                
                pol.template initCalculatingScore<M>(NMoves);
                
                if(!M || dst != nullptr){
                    dst[0] = 0;
                }
                
                constexpr int L = board_t::length();
                
                const Color oppColor = flipColor(myColor);
                const int ci = toColorIndex(myColor);
                
                for(int m = 0; m < NMoves; ++m){
                    
                    pol.template initCalculatingCandidateScore<M>();
                    
                    double s = 0;
                    int i;
                    const move_t& mv = buf[m];
                    const int z = mv.toZ();
                    const int x = ZtoX(z);
                    const int y = ZtoY(z);
                    const int iz = ZtoIZ<L>(z);
                    const int ix = IZtoIX<L>(iz);
                    const int iy = IZtoIY<L>(iz);
                    
                    { // pass
                        constexpr int base = FEA_IDX(POL_PASS);
                        if(mv.isPass()){
                            i = base + ci;
                            FooC(POL_PASS, i);
                        }
                    }
                    { // distance to wall
                        constexpr int base = FEA_IDX(POL_DIST_WALL);
                        if(!mv.isPass()){
                            i = base + min(19, bd.turn / 15) * 10 + calcDistanceToEndIXIY<L>(ix, iy);
                            FooC(POL_DIST_WALL, i);
                        }
                    }
                    
                    
                    { // 3x3 pettern
                        constexpr int base = FEA_IDX(POL_3x3);
                        
                        if(!mv.isPass()){
                            
                            i = base + toColorIndex(myColor) * 25 * 9 * ipow(3, 8);
                            
                            // 5路盤での位置インデックス
                            int i5z = IXIYtoIZ<5>(ItoSmallerBoardI<L, 5>(ix), ItoSmallerBoardI<L, 5>(iy));
                            i += i5z * 9 * ipow(3, 8);
                            
                            iterateAround8WithIndexUnrolled(z, [&i, &bd](int ti, int tz)->void{
                                if(tz == bd.lastZ){
                                    i += (1 + ti) * ipow(3, 8);
                                }
                                Color tc3 = eraseWallColor(bd.color(tz));
                                i += ipow(3, ti) * tc3;
                            });
                            FooC(POL_3x3, i);
                        }
                    }
                    FASSERT(s,);
                    double exps = exp(s / pol.temperature());
                    FASSERT(exps, cerr << "s = " << s << " T = " << pol.temperature() << endl;);
                    
                    pol.template feedCandidateScore<M>(exps);
                    
                    if(!M || dst != nullptr){
                        dst[m + 1] = dst[m] + exps;
                    }
                }
                return 0;
            }
            
#undef Foo
#undef FooX
#undef FooC
#undef FooXC
            
            template<class move_t, class board_t, class policy_t, class dice_t>
            int playWithPolicy(move_t *const buf, const int NMoves, const Color myColor, const board_t& bd, const policy_t& pol, dice_t *const pdice){
                double score[NMoves + 1];
                calcPolicyScoreSlow<0>(score, buf, NMoves, myColor, bd, pol);
                double r = pdice->drand() * score[NMoves];
                return sortedDAsearch(score, 0, NMoves, r);
            }
            template<class board_t, class policy_t, class tools_t>
            int playWithPolicy(const Color myColor, const board_t& bd, const policy_t& pol, tools_t *const ptools){
                const int NMoves = genAllMovesWithoutEye(ptools->buf, myColor, bd);
                return ptools->buf[playWithPolicy(ptools->buf, NMoves, myColor, bd, pol, &ptools->dice)].toZ();
            }
            
            template<class move_t, class board_t, class policy_t, class dice_t>
            int playWithBestPolicy(move_t *const buf, const int NMoves, const Color myColor, const board_t& bd, const policy_t& pol, dice_t *const pdice){
                double score[NMoves + 1];
                calcPolicyScoreSlow<0>(score, buf, NMoves, myColor, bd, pol);
                int bestIndex[NMoves];
                bestIndex[0] = -1;
                int NBestMoves = 0;
                double bestScore = -DBL_MAX;
                for(int m = 0; m < NMoves; ++m){
                    double s = score[m + 1] - score[m];
                    if(s > bestScore){
                        bestIndex[0] = m;
                        bestScore = s;
                        NBestMoves = 1;
                    }else if(s == bestScore){
                        bestIndex[NBestMoves] = m;
                        ++NBestMoves;
                    }
                }
                if(NBestMoves <= 1){
                    return bestIndex[0];
                }else{
                    return bestIndex[pdice->rand() % NBestMoves];
                }
            }
            
            template<class board_t, class policy_t, class tools_t>
            int playWithBestPolicy(const Color myColor, const board_t& bd, const policy_t& pol, tools_t *const ptools){
                const int NMoves = genAllMovesWithoutEye(ptools->buf, myColor, bd);
                return ptools->buf[playWithBestPolicy(ptools->buf, NMoves, myColor, bd, pol, &ptools->dice)].toZ();
            }
            }
#endif // GO_JULIE_POLICY_HPP_