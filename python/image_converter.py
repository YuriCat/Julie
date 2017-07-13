# -*- coding: utf-8 -*-
# image_converter.py
# Katsuki Ohto
# 2016/4/1

# 文字列の画像を学習時のnumpy形式に保存

import sys

import numpy as np
import go

BATCH = 256

def convert_to_np(ipath, opath, n):
    file_name = ipath + str(n) + ".dat"
    images, labels = go.load_image_moves(file_name, BATCH)
    
    image_name = opath + "_np_img" + str(n)
    label_name = opath + "_np_lbl" + str(n)
    
    np.savez_compressed(image_name, images)
    np.savez_compressed(label_name, labels)

if __name__ == "__main__":
    args = sys.argv
    ipath = args[1]
    opath = args[2]
    nst = int(args[3])
    ned = int(args[4])
    
    print(args)
    
    for i in range(nst, ned, 1):
        convert_to_np(ipath, opath, i)
