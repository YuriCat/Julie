# -*- coding: utf-8 -*-
# lstm_policy.py
# Katsuki Ohto

# 碁盤目状に配置した多段LSTMにて着手列を学習

import numpy as np
import chainer
import chainer.functions as F
import chainer.FunctionSet as FunctionSet

import go



class SoftmaxRegressor:
    def __init__(self):
        
        model = FunctionSet(l1_x = F.Linear(go.SIZE, 4 * go.SIZE),
                            l1_h = F.Linear(4 * go.SIZE, 4 * go.SIZE),
                            last = F.Linear(4 * go.SIZE, go.SIZE))
        
        loss = F.softmax_cross_entropy()
        
        return

    def forward_one_step_train(iz, y_, h0, c0, train = True):
        # izに着手がなされた場合(パスで無い)
        x_ = np.zeros((go.SIZE, len(iz)), dtype = np.float32)
        for i in range(len(iz)):
            x_[iz][i] = 1
        x = Variable(x_) # 入力
        h1_in = self.model.l1_x(F.dropout(x, train = train) +  self.model.l1_h(h0))
        c1, h1 = F.lstm(c0, h1_in)
        y = self.model.last(h1)
        l = self.loss(y, Variable(y_))
        return y, h1, c1, l
            
    def forward_one_step_test(iz, h0, c0, train = False):
        # izに着手がなされた場合(パスで無い)
        x_ = np.zeros((go.SIZE, len(iz)), dtype = np.float32)
        for i in range(len(iz)):
            x_[iz][i] = 1
        x = Variable(x_) # 入力
        h1_in = self.model.l1_x(F.dropout(x, train = train) +  self.model.l1_h(h0))
        c1, h1 = F.lstm(c0, h1_in)
        y = self.model.last(h1)
        return y, h1, c1
            
    def forward_train(play, train = True):
        h = Variable(np.zeros(go.SIZE, len(iz)))
        c = Variable(np.zeros(go.SIZE, len(iz)))
        loss = Variable(0)
        for t in len(play) - 1:
            y, h, c, l = forward_one_step_train(play[t], play[t + 1], h, c, train)
            loss += l
        return loss
            
    def forward_test(play, train = False):
        h = Variable(np.zeros(go.SIZE, len(iz)))
        c = Variable(np.zeros(go.SIZE, len(iz)))
        y = None
        for t in len(play) - 1:
            y, h, c = forward_one_step_test(play[t], h, c, train)
        return y
    
    def predict(play):
    
    def calc_loss(x):
