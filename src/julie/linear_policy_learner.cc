// 囲碁
// 方策関数を方策勾配を用いた教師有り学習で学習

#include "julie_go.hpp"
#include "../sgf/sgf.h"
#include "policy.hpp"
#include "learn/policyGradient.hpp"

enum{
    MODE_FLAG_TEST = 1,
    MODE_FLAG_SHUFFLE = 8,
    MODE_FLAG_ANALYZE = 16,
};

namespace LearningSettings{
    // 学習用パラメータの設定
    constexpr double temperature = 1;
    double learningRate = 0.00005;
    double attenuatingRate = 0.05;
    double L1Rate = 0;
    double L2Rate = 0.0000001;
    int batchSize = 8192;
    int iterations = 200;
}

int learn(int mode, int itr, int batch,
          const std::vector<std::string>& logFileNames,
          const std::string& baseParamFile){
    
    using namespace Go;
    using namespace Go::SGF;
    
    constexpr int L = 19;
    
    double testRate = (mode & MODE_FLAG_TEST) ? 0.77 : 0;
    
    initHash();
    
    ThreadTools tools;
    
    std::mt19937 mt((uint32_t)time(NULL));
    tools.dice.srand(mt());
    LinearPolicy *const ppolicy = new LinearPolicy();
    LinearPolicyLearner *const plearner = new LinearPolicyLearner();
    
    LinearPolicy& policy = *ppolicy;
    LinearPolicyLearner& learner = *plearner;
    
    policy.setLearner(&learner);
    learner.setPolicy(&policy);
    
    // ログをファイルから読み込み
    RandomAccessor<SGFLog> db;
    
    //std::string logFileName = DIR_SGFLOG + "sgflog.dat";
    //std::string logFileName = DIR_SGFLOG + "sgflog_mini.dat";
    std::string logFileName = logFileNames.at(0);
    
    cerr << "started reading sgflog file." << endl;
    int games = readSGFLog(logFileName, &db, [&](const auto& gm)->bool{
        return true;
    });
    if(games < 0){
        cerr << "failed to read sgflog file." << endl;
        return -1;
    }
    cerr << games << " games were imported." << endl;
    
    // 学習とテストの試合数決定
    const double learnGameRate = games / (games + pow((double)games, testRate));
    
    const int learnGames = min((int)(games * learnGameRate), games);
    const int testGames = games - learnGames;
    
    // ログのアクセス順を並べ替え
    db.initOrder();
    if(mode & MODE_FLAG_SHUFFLE){
        db.shuffle(0, db.size(), mt);
    }
    
    // 特徴要素を解析して、学習の度合いを決める
    learner.setLearnParam(1, 0, 0, 0, 1);
    policy.setTemperature(learner.temperature());
    
    if(mode & MODE_FLAG_ANALYZE){
        cerr << "started analyzing feature." << endl;
        
        BitSet32 flag(0);
        flag.set(1);
        
        // 学習データ
        learner.initFeatureValue();
        iterateRandomly(db, 0, learnGames, [&](const auto& game)->void{
            for(int sym = 0; sym < 8; ++sym){
                learnGamePolicyGradient<L>(game, flag, policy, sym, &learner, &tools); // 特徴要素解析
            }
        });
        learner.closeFeatureValue();
        learner.foutFeatureSurvey(DIR_PARAMS_OUT + "policy_feature_survey.dat");
        cerr << "training record : " << learner.toRecordString() << endl;
        
        // テストデータ
        learner.initFeatureValue();
        iterateRandomly(db, learnGames, games, [&](const auto& game)->void{
            for(int sym = 0; sym < 8; ++sym){
                learnGamePolicyGradient<L>(game, flag, policy, sym, &learner, &tools); // 特徴要素解析
            }
        });
        learner.closeFeatureValue();
        cerr << "test     record : " << learner.toRecordString() << endl;
    }else{
        cerr << "already finished analyzing feature." << endl;
    }
    
    // 学習開始
    cerr << "started learning." << endl;
    learner.finFeatureSurvey(DIR_PARAMS_OUT + "policy_feature_survey.dat");
    if(baseParamFile != ""){
        policy.fin(baseParamFile);
    }
    
    for (int j = 0; j < itr; ++j){
        
        cerr << "iteration " << j << endl;
        learner.setLearnParam(LearningSettings::temperature,
                              LearningSettings::learningRate * exp(-j * LearningSettings::attenuatingRate),
                              LearningSettings::L1Rate * exp(-j * LearningSettings::attenuatingRate),
                              LearningSettings::L2Rate * exp(-j * LearningSettings::attenuatingRate),
                              LearningSettings::batchSize);
        
        db.shuffle(0, learnGames, mt);
        
        BitSet32 flag(0);
        flag.set(0);
        flag.set(2);
        
        learner.initObjValue();
        iterateRandomly(db, 0, learnGames, [&](const auto& game)->void{
            const int sym = tools.dice.rand() % 8;
            learnGamePolicyGradient<L>(game, flag, policy, sym, &learner, &tools); // 学習
        });
        
        policy.fout(DIR_PARAMS_OUT + "policy_param.dat");
        foutComment(policy, DIR_PARAMS_OUT + "policy_comment.txt");
        
        cerr << "Training : " << learner.toObjValueString() << endl;
        
        if(testGames > 0){ // テスト
            flag.reset(0);
            learner.initObjValue();
            iterateRandomly(db, learnGames, games, [&](const auto& game)->void{
                for(int sym = 0; sym < 8; ++sym){
                    learnGamePolicyGradient<L>(game, flag, policy, sym, &learner, &tools); // テスト
                }
            });
            cerr << "Test     : " << learner.toObjValueString() << endl;
        }
    }
    cerr << "finished learning." << endl;
    
    return 0;
    
}

int main(int argc, char* argv[]){
    
    // 基本的な初期化
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    
    int mode = MODE_FLAG_SHUFFLE | MODE_FLAG_ANALYZE;
    int itr = 50;
    int batch = 8192;
    
    std::vector<std::string> logFileNames;
    std::string baseParamFile = "";
    
    for(int c = 1; c < argc; ++c){
        if(!strcmp(argv[c], "-b")){
            batch = atoi(argv[c + 1]);
        }else if(!strcmp(argv[c], "-i")){
            itr = atoi(argv[c + 1]);
        }else if(!strcmp(argv[c], "-l")){
            logFileNames.emplace_back(std::string(argv[c + 1]));
        }else if(!strcmp(argv[c], "-na")){
            mode &= ~MODE_FLAG_ANALYZE;
        }else if(!strcmp(argv[c], "-t")){
            mode |= MODE_FLAG_TEST;
        }else if(!strcmp(argv[c], "-o")){
            Go::DIR_PARAMS_OUT = std::string(argv[c + 1]);
        }else if(!strcmp(argv[c], "-b")){
            baseParamFile = std::string(argv[c + 1]);
        }
    }
    
    int ret = learn(mode, itr, batch, logFileNames, baseParamFile);
    
    return ret;
}