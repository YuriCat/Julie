# -*- coding: utf-8 -*-
# value_learn_cnn.py
# Katsuki Ohto
# http://kivantium.hateblo.jp/entry/2015/11/18/233834

import __future__
import sys
import time

import numpy as np
import tensorflow as tf

import go

import model.rn_policy as G
import model.cnn_value as D

N_PHASES = 1
#BATCH_SIZE = 8192
#N_TRAIN_BATCHES = 1000
BATCH_SIZE = 256
N_TRAIN_BATCHES = 276500
#N_TRAIN_BATCHES = 34000
N_TEST_BATCHES = 512
ex, images, moves))

def train_discriminator(D_model_path):
    # generatorの生成結果を用いてdiscriminatorを学習
    with tf.Graph().as_default():
        x_placeholder = tf.placeholder("float", shape = (None, go.IMAGE_LENGTH, go.IMAGE_LENGTH, go.IMAGE_PLAINS))
        y_placeholder = tf.placeholder("float", shape = (None, 1))
        learning_rate_placeholder = tf.placeholder("float", shape = ())
        
        forward_op = [D.make(x_placeholder) for ph in range(N_PHASES)]
        loss_op = [D.calc_loss(forward_op[ph]) for ph in range(N_PHASES)]
        train_op = [D.train(loss_op[ph], learning_rate_placeholder) for ph in range(N_PHASES)]

        saver = tf.train.Saver()
        sess = tf.Session()
        sess.run(tf.global_variables_initializer())
        # パラメータ読み込み
        if D_model_path != "none":
            saver.restore(sess, D_model_path)

        learning_rate = 0.0001
        y_positive = np.ones((BATCH_SIZE, 1))
        y_negative = -np.ones((BATCH_SIZE, 1))
    
        for e in range(mepoch):
            # training phase
            for ph in range(N_PHASES):
                # 正例ロード
                tstart = time.time()
                index = np.random.randint(N_TRAIN_BATCHES)
                images, moves = go.load_image_moves_np(data_path, index)
                #print("loading %f sec" % (time.time() - tstart))
                tstart = time.time()
                for i in range(0, BATCH_SIZE, batch_size):
                    sess.run(train_op[ph], feed_dict={
                             x_placeholder : images[i : (i + batch_size)],
                             label_placeholder : moves[i : (i + batch_size)],
                             learning_rate_placeholder : learning_rate})
                # 負例作成
                for i in range(0, batch_size):
                    sess.run()
    
            #print("training %f sec" % (time.time() - tstart))
            learning_rate *= 0.999999
            if e % 256 != 0:
                for i in range(0, BATCH_SIZE, batch_size):
                    sess.run(train_op[ph], feed_dict={
                             x_placeholder : images[i : (i + batch_size)],
                             y_placeholder : moves[i : (i + batch_size)],
                         learning_rate_placeholder : learning_rate})
                continue


if __name__ == "__main__":

    args = sys.argv
 
    # 設定
    mepoch = int(args[2])
    batch_size = int(args[3])
    mdl_file = args[4]
    data_path = args[1]
        
    with tf.Graph().as_default():
        x_placeholder = tf.placeholder("float", shape = (None, go.IMAGE_LENGTH, go.IMAGE_LENGTH, go.IMAGE_PLAINS))
        y_placeholder = tf.placeholder("float", shape = (None, 2))
        moves_placeholder = tf.placeholder("int64", shape = (None))
        learning_rate_placeholder = tf.placeholder("float", shape = ())
        
        # モデル作成
        y_op = [mdl.make(x_placeholder) for ph in range(N_PHASES)]
        # 正規化
        prob_op = [mdl.normalize(y_op[ph]) for ph in range(N_PHASES)]
        # loss計算
        loss_op = [mdl.calc_loss(prob_op[ph], label_placeholder) for ph in range(N_PHASES)]
        # training()を呼び出して訓練
        train_op = [mdl.train(loss_op[ph], learning_rate_placeholder) for ph in range(N_PHASES)]
        # 精度の計算
        test_op = [mdl.count_correct_num(y_op[ph], moves_placeholder) for ph in range(N_PHASES)]
        
        # 保存の準備
        saver = tf.train.Saver()
        sess = tf.Session()
        sess.run(tf.global_variables_initializer())
        # パラメータ読み込み
        if mdl_file != "none":
            saver.restore(sess, mdl_file)
        
        # TensorBoardで表示する値の設定
        #summary_op = tf.merge_all_summaries()
        #summary_writer = tf.train.SummaryWriter(FLAGS.train_dir, sess.graph_def)
        
        # テストはいつも同じデータなので先にロードしておく
        tstart = time.time()
        #test_images, test_moves = go.load_image_moves(data_path + str(N_TRAIN_BATCHES) + ".dat", BATCH_SIZE)
        test_images, test_moves = go.load_image_moves_np(data_path, N_TRAIN_BATCHES)
        for i in range(1, N_TEST_BATCHES, 1):
            test_image, test_move = go.load_image_moves_np(data_path, N_TRAIN_BATCHES + i)
            test_images = np.r_[test_images, test_image]
            test_moves = np.r_[test_moves, test_move]
        print("loading test-data  %f sec" % (time.time() - tstart))
        print(test_moves)
        test_answers = np.argmax(test_moves, axis = 1)
        learning_rate = 0.0001
        
        for e in range(mepoch):
            
            # training phase
            for ph in range(N_PHASES):
                # データロード
                tstart = time.time()
                index = np.random.randint(N_TRAIN_BATCHES)
                images, moves = go.load_image_moves_np(data_path, index)
                
                #print("loading %f sec" % (time.time() - tstart))
                
                tstart = time.time()
                #print(index)
                for i in range(0, BATCH_SIZE, batch_size):
                    # トレーニング
                    # feed_dictでplaceholderに入れるデータを指定する
                    sess.run(train_op[ph], feed_dict={
                             x_placeholder : images[i : (i + batch_size)],
                             label_placeholder : moves[i : (i + batch_size)],
                             learning_rate_placeholder : learning_rate})
                
                #print("training %f sec" % (time.time() - tstart))
            learning_rate *= 0.999999
            if e % 2048 != 0:
                continue
            # test phase
            for ph in range(N_PHASES):
                correct_num = 0
                for i in range(0, BATCH_SIZE, batch_size):
                    correct_num += sess.run(test_op[ph],
                                            feed_dict = {
                                            x_placeholder : test_images[i : (i + batch_size)],
                                            moves_placeholder : test_answers[i : (i + batch_size)]})
                print("[%d samples] test accuracy %g" % (BATCH_SIZE * (e + 1), correct_num / float(BATCH_SIZE)))
            
            save_path = saver.save(sess, "tf_policy_cnn.ckpt")
