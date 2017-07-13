# -*- coding: utf-8 -*-
# policy_test.py
# Katsuki Ohto

import sys

import numpy as np
import tensorflow as tf

import go
#import model.cnn_policy as mdl
import model.rn_policy as mdl


def test_inverse():
    # 白番と黒番で同じ着手確率が返るか


