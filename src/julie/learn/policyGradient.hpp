/*
 policyGradient.hpp
 Katsuki Ohto
 */

#ifndef GO_JULIE_LEARN_POLICYGRADIENT_HPP_
#define GO_JULIE_LEARN_POLICYGRADIENT_HPP_

#include "../julie_go.hpp"
#include "../policy.hpp"

namespace Go{
    // 試合ログから方策勾配を用いた教師有り学習で着手方策関数を棋譜の着手が選ばれやすいように近づける
    template<int B_LEN, class gameLog_t, class policy_t, class learner_t, class tools_t>
    int learnGamePolicyGradient(const gameLog_t& game,
                                const BitSet32 flags,
                                const policy_t& policy,
                                const int symmetry,
                                learner_t *const plearner,
                                tools_t *const ptools){
        
        iterateGameLog<EasyBoard<B_LEN>>
        (game, symmetry,
         [](const auto& bd)->void{}, // first callback
         [&](const auto& bd, const int mvz0)->int{ // play callback
             const int mvz = symmetryZ<B_LEN>(mvz0, symmetry);
             constexpr int ph = 0;
             
             auto *const buf = ptools->buf;
             const int NMoves = genAllMoves(buf, bd);
             
             int index = -1;
             for(int m = 0; m < NMoves; ++m){
                 if(buf[m].toZ() == mvz){
                     index = m;
                     break;
                 }
             }
             
             //cerr << bd;
             //cerr << mvz << endl;
             //cerr << Point(mvz)<< endl;
             
             if(index == -1){ // unfound
                 
                 //cerr << bd;
                 //cerr << mvz << endl;
                 //cerr << Point(mvz)<< endl;
                 //cerr << OutRule(bd.check(bd.getTurnColor(), mvz)) << endl;
                 //getchar();
                 
                 if(flags.test(1)){ // feed feature value
                     plearner->feedUnfoundFeatureValue(ph);
                 }
                 if(flags.test(2)){ // test
                     plearner->feedObjValue(index, ph);
                 }
             }else{ // found
                 double score[B_LEN * B_LEN + 1];
                 calcPolicyScoreSlow<1>(score, buf, NMoves, bd.getTurnColor(), bd, policy);
                 if(flags.test(1)){ // feed feature value
                     plearner->feedFeatureValue(ph);
                 }
                 if(flags.test(0)){ // learn
                     plearner->feedSupervisedActionIndex(index, ph);
                     plearner->updateParams();
                 }
                 if(flags.test(2)){ // test
                     plearner->feedObjValue(index, ph);
                 }
             }
             return 0;
         },
         [](const auto& bd)->void{}); // last callback
        return 0;
    }
}

#endif // GO_JULIE_LEARN_POLICYGRADIENT_HPP_