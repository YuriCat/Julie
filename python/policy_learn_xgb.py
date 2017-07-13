# -*- coding: utf-8 -*-
# policy_learn_xgb.py
# Katsuki Ohto

import sys
import numpy as np
from sklearn.metrics import confusion_matrix

import xgboost as xgb

import go

# gradient boosting tree にて指し手予測

N_PHASES = 2 # 先後別々

if __name__ == "__main__":

    args = sys.argv
 
    # データのロードと設定
    move_logs = go.load_move_log(args[1])

    loaded_data_num = len(shot_logs)

    print("number of loaded move-logs = %d" % loaded_data_num)
    
    # 場合分けのそれぞれのパターン数を求める
    # training data, test dataを作る
    phase_index_vector = [[] for _ in range(N_PHASES)]
    for i, sl in enumerate(shot_logs):
        phase_index_vector[sl['turn']].append(i)
    
    phase_data_num = [len(v) for v in phase_index_vector]
    data_num = np.sum(phase_data_num)

    print(phase_data_num)
    
    for ph in range(N_PHASES):
        np.random.shuffle(phase_index_vector[ph]) # データシャッフル

    # 入力データ
    board_vector = [np.empty((phase_data_num[ph], 2 + dc.N_STONES * 4), dtype = np.float32) for ph in range(N_PHASES)]
    move_vector = [np.zeros(phase_data_num[ph], dtype = np.float32) for ph in range(N_PHASES)]

    for ph in range(N_PHASES):
        for i in range(phase_data_num[ph]):
            sl = move_logs[phase_index_vector[ph][i]]
            sc = sl['escore']
        
            sc_index = dc.StoIDX(sc)
            move_vector[ph][i] = sc_index

    print("number of used shot-logs = %d" % data_num)

    test_rate = 0.2
    test_num = []
    train_num = []
    
    for ph in range(N_PHASES):
        test_num.append(int(phase_data_num[ph] * test_rate))
        train_num.append(phase_data_num[ph] - test_num[ph])
    
    classifier = [xgb.XGBClassifier() for _ in range(N_PHASES)]
    
    for ph in range(N_PHASES):
        classifier[ph].fit(board_vector[ph][0 : train_num[ph]],
                           score_vector[ph][0 : train_num[ph]])

        #print(classifier.predict(board_vector_all[train_num : data_num]))

        cm = confusion_matrix(score_vector[train_num[ph] : phase_data_num[ph]],
                      classifier[ph].predict(board_vector[train_num[ph] : phase_data_num[ph]]))

        print(cm)

        print("phase %d accuracy = %f" % (ph, (np.sum(np.diag(cm)) / float(np.sum(cm)))))


