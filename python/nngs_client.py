# -*- coding: utf-8 -*-
# nngs_client.py
# Katsuki Ohto

import sys, socket, time
from contextlib import closing

import numpy as np

import go
import policy_client as pc

DEFAULT_HOST = '127.0.0.1'
DEFAULT_PORT = 9696

def send_msg(sock, msg):
    print("Client << %s" % msg)
    buf = (msg + '\0').encode('ascii')
    print(buf)
    sock.send(buf)

def recv_msg(sock, buf_size):
    msg = sock.recv(buf_size)
    print(msg)
    return msg.decode('ascii', 'ignore')

class Engine:
    def __init__(self, l, time_limit):
        self.board = go.Board(l)
        self.policy = None
        self.org_time_limit = time_limit

    def init_all(self, arg_list):
        print('Engine.init_all()')
        self.policy = pc.PolicyClient(arg_list[0]) # モデル読み込み
        return 0

    def init_game(self, arg_list):
        print('Engine.init_game()')
        self.board.clear()
        self.time_limit = self.org_time_limit
        return 0

    def close_game(self, arg_list):
        print('Engine.close_game()')
        return 0

    def close_all(self, arg_list):
        print('Engine.close_all()')
        return -1

    def play(self, my_color):
        print('Engine.play()')
        
        print("remained time = %s" % self.time_limit)
        tstart = time.time()

        """vmoves = 0
        for z in range(1, go.IMAGE_SIZE):
            validity = self.board.checkRoot(my_color, z)
            if validity == go.VALID: # 合法である
                vmoves = 1
                break
        if vmoves == 0:
            print("no valid move.")
            self.time_limit -= time.time() - tstart
            return go.PASS"""
        
        sc2 = self.board.count_chinese_score2()
        print("current chinese score x 2 = %d" % sc2)
        if self.board.lastZ == go.PASS:
            #if self.board.turn >= 200:
            #    return go.PASS
            if my_color == go.BLACK and sc2 > 0:
                self.time_limit -= time.time() - tstart
                return go.PASS
            elif my_color == go.WHITE and sc2 < 0:
                self.time_limit -= time.time() - tstart
                return go.PASS
        
        img = go.to_image(self.board, my_color)
        
        amp_img = go.amplify_image8(img)
        prob = self.policy.calc_amp8(amp_img)
        
        print(go.colorString[my_color])
        for ix in range(self.board.length):
            print([int(prob[go.XYtoZ(ix + self.board.padding, iy + self.board.padding)] * 100) for iy in range(self.board.length)])
        
        vlist = []
        for z in range(1, go.IMAGE_SIZE):
            validity = self.board.checkRoot(my_color, z)
            if validity == go.VALID: # 合法である
                vlist.append((z, prob[z]))
        if len(vlist) == 0:
            print("no valid move.")
            self.time_limit -= time.time() - tstart
            return go.PASS
        
        np.random.shuffle(vlist) # 同一局面を避けるために先にランダムに並べ替え
        sorted_vlist = sorted(vlist, key = lambda x : -x[1])
        self.time_limit -= time.time() - tstart
        return sorted_vlist[0][0]

    def recv_play(self, c, z):
        print('Engine.recv_play()')
        self.board.move(c, z, force = True)
        print(self.board.to_string())
        return 0

def to_NNGS_move_string(z, l):
    print("z = %d" % z)
    if z == go.RESIGN or z == go.PASS:
        return "pass"
    else:
        x = go.ZtoX(z)
        y = go.ZtoY(z)
        padding = go.PADDING + (go.MAX_LENGTH - l) // 2
        return go.xChar[x - padding] + str(y - padding + 1)

def nngs_main(length, my_color, host = DEFAULT_HOST, port = DEFAULT_PORT, my_id = "julie", ops_id = "noName"):
    
    buf_size = 1024
    
    engine = Engine(length, 30 * 60)
    ingame = False
    end = False
    
    host = socket.gethostbyname_ex(host)[0]
    
    #model_path = "/Users/ohto/documents/data/go/tfmodel/rn170317_b/tf_policy_cnn.ckpt"
    model_path = "/Users/ohto/documents/data/go/tfmodel/rn170405/tf_policy_cnn.ckpt"
    
    engine.init_all([model_path])

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    with closing(sock):
        sock.connect((host, port))
        send_msg(sock, my_id + '\n') # ログイン
        while not end:
            rmsg_str = recv_msg(sock, buf_size)
            if len(rmsg_str) == 0:
                print("disconnected...")
                end = True
            rmsg_list = str(rmsg_str).split('\n')
            for command in rmsg_list:
                print("Server >> %s" % command)
                # メッセージが複雑なのでこちらで分岐する
                if command.find("No Name Go Server") >= 0:
                    send_msg(sock, "set client TRUE\n") # シンプルな通信モードに
                if command.find("Set client to be True") >= 0:
                    if my_color == go.BLACK and not ingame:
                        ingame = True
                        engine.init_game([])
                        send_msg(sock, "match " + ops_id + " B " + str(length) + " 30 0\n") # 黒番の場合に、試合を申し込む
                if command.find("Match [" + str(length) + "x" + str(length) + "]") >= 0:
                    if my_color == go.WHITE and not ingame:
                        ingame = True
                        engine.init_game([])
                        send_msg(sock, "match " + ops_id + " W " + str(length) + " 30 0\n") # 白番の場合に、試合を受ける
                if my_color == go.BLACK and command.find("accepted.") >= 0: # 白が応じたので初手を送る
                    move_z = engine.play(my_color);
                    engine.recv_play(my_color, move_z);
                    send_msg(sock, to_NNGS_move_string(move_z, length) + "\n") # 手を送る
                if command.find("Illegal") >= 0:
                    break
                if command.find("You can check your score") >= 0: # Passが連続した場合に来る(PASSした後の相手からのPASSは来ない）
                    send_msg(sock, "done\n");
                    end = True
                if command.find("9 {Game") >= 0 and command.find("resigns.") >= 0:
                    if command.find(my_id + " vs " + ops_id) >= 0:
                        end = True # game end
                    if command.find(ops_id + " vs " + my_id) >= 0:
                        end = True # game end
                if command.find("forfeits on time") >= 0 and command.find(my_id) >= 0 and command.find(ops_id) >= 0:
                    #どちらかの時間切れ
                    end = True
                if command.find("{" + ops_id + " has disconnected}") >= 0:
                    end = True # 通信切断
                if command.find("(" + go.colorChar[go.flip_color(my_color)]+ "): ") >= 0:
                    # 相手の通常の手を受信
                    index = command.find("(" + go.colorChar[go.flip_color(my_color)]+ "): ") + 5
                    tstr = command[index :]
                    
                    move_z = go.RESIGN
                    if tstr.find("Pass") >= 0:
                        move_z = go.PASS
                    else:
                        padding = go.PADDING + (go.MAX_LENGTH - engine.board.length) // 2
                        x = go.char_to_ix(tstr[0]) + padding
                        y = int(tstr[1 :]) - 1 + padding
                        move_z = go.XYtoZ(x, y)

                    engine.recv_play(go.flip_color(my_color), move_z)
                    move_z = engine.play(my_color);
                    engine.recv_play(my_color, move_z)
                                                                                  
                    send_msg(sock, to_NNGS_move_string(move_z, length) + "\n") # 手を送る
    return

if __name__ == '__main__':
    # length, color, host, port, my_id, opp_id
    my_color = go.BLACK
    if sys.argv[2] == 'W':
        my_color = go.WHITE
    nngs_main(length = int(sys.argv[1]),
              my_color = my_color,
              host = sys.argv[3],
              port = int(sys.argv[4]),
              my_id = sys.argv[5],
              ops_id = sys.argv[6])