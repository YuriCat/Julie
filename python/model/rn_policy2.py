# -*- coding: utf-8 -*-
# rn_policy2.py
# Katsuki Ohto
# https://www.tensorflow.org/versions/0.6.0/tutorials/mnist/pros/index.html
# http://kivantium.hateblo.jp/entry/2015/11/18/233834

import numpy as np
import tensorflow as tf

import go

N_FILTERS = 320

# convolutional neural network
def make(x_placeholder):
    # モデルを作成する関数
    
   # 初期化関数
    def weight_variable(shape):
        initial = tf.truncated_normal(shape, stddev = 0.01, dtype = tf.float32)
        return tf.Variable(initial)

    def bias_variable(shape):
        initial = tf.zeros(shape = shape, dtype = tf.float32)
        return tf.Variable(initial)

    # 畳み込み層
    def conv2d(x, W):
        return tf.nn.conv2d(x, W, strides = [1, 1, 1, 1], padding = 'SAME')

    # 畳み込み層1
    with tf.name_scope('conv1') as scope:
        W_conv1 = weight_variable([5, 5, go.IMAGE_PLAINS, N_FILTERS])
        b_conv1 = bias_variable([N_FILTERS])
        h_conv = tf.nn.relu(conv2d(x_placeholder, W_conv1) + b_conv1)
        #h_conv = tf.nn.relu(tf.nn.conv2d(x_placeholder, W_conv1, strides = [1, 1, 1, 1], padding = 'VALID') + b_conv1)
    
    layers = range(0, 24)
    W_conv = [weight_variable([3, 3, N_FILTERS, N_FILTERS]) for l in layers]
    b_conv = [bias_variable([N_FILTERS]) for l in layers]

    # 畳み込み層2~
    for l in layers:
        with tf.name_scope('conv' + str(l + 2)) as scope:
            h_conv = tf.nn.relu(conv2d(h_conv, W_conv[l]) + b_conv[l]) + h_conv

    # 最終層
    with tf.name_scope('last') as scope:
        W_convl = weight_variable([1, 1, N_FILTERS, 1])
        h_convl = conv2d(h_conv, W_convl)
        h_flat = tf.reshape(h_convl, [-1, go.IMAGE_SIZE])
        b_last = bias_variable([go.IMAGE_SIZE])
        h_last = h_flat + b_last

    return h_last

def normalize(y):
    return tf.nn.softmax(y) # 確率配列を返す

def calc_loss(prob, labels):
    cross_entropy = -tf.reduce_sum(labels * tf.log(tf.clip_by_value(prob, 1e-10, 1.0)))
    # TensorBoardで表示するよう指定
    #tf.scalar_summary("cross_entropy", cross_entropy)
    return cross_entropy

def train(loss, learning_rate):
    #train_step = tf.train.AdamOptimizer(learning_rate).minimize(loss)
    train_step = tf.train.GradientDescentOptimizer(learning_rate).minimize(loss)
    return train_step

def count_correct_num(y, answer):
    correct_prediction = tf.equal(tf.argmax(y, 1), answer)
    return tf.reduce_sum(tf.to_int32(correct_prediction))
