# cd ..
# python -m unittest tests/test_go.py
import unittest

import go

# convert text representation of go board to board structure
c2cell = {'X' : go.BLACK, 'O' : go.WHITE, '.' : go.EMPTY, '+' : go.EMPTY}
def s2board(board, s):
    board.clear()
    ls = s.split('\n')
    assert len(ls) == 22
    for y in range(go.LENGTH):
        for x in range(go.LENGTH):
            c = ls[19 - y][x * 2 + 3]
            z = go.XYtoZ(go.PADDING + x, go.PADDING + y)
            board.cell[z] = c2cell[c]
    if ls[21][0:5] == 'White': board.turn = 1
    elif ls[21][0:5] == 'Black': board.turn = 0
    else: raise 'Unknown color'
def s2z(s):
    x = go.xChar.find(s[0])
    y = int(s[1:]) - 1
    return go.XYtoZ(go.PADDING + x, go.PADDING + y)

class TestGo(unittest.TestCase):
    def setUp(self):
        self.board = go.Board(19)
        s2board(self.board,"""
   A B C D E F G H J K L M N O P Q R S T 
19 . X . . . . . . . . . . . . . . . . . 19
18 . O X . O . O . O . . . . . . . . . . 18
17 . O . X X O . O X . . . . X . . . . . 17
16 X X X X O O . O X + . . . . . X X X . 16
15 . O O O O . . O X . . . . . . O X O . 15
14 . . . X X X . . X . . . . . . O O . . 14
13 . O . X O O O O . . . . . . . . . . . 13
12 . X X X X X . . . X . . . . O O X . . 12
11 . X O O X . O O O O . . . . . X O . . 11
10 X O O X X . X X . + . . . . . X O . . 10
 9 . X O O X . X . . . . . . . X O O O . 9 
 8 . . . . O O . . X . . X . . X O O X . 8 
 7 . . X O . O . O . . . . . . O X O X . 7 
 6 . O X X X X O O O O O O O . O X X X . 6 
 5 . X . X O O X X O X . X . . O . . . . 5 
 4 . O X O O X . X O + . . . . . O O X . 4 
 3 . X O O O O O X X O O X . . . O X X . 3 
 2 . X X O X . O X . X O O . . . . O X . 2 
 1 . . . X . O X X X . X . . . . . . . . 1 
   A B C D E F G H J K L M N O P Q R S T 
White to play
"""[1:-1])
        self.eboard = go.Board(19)
        s2board(self.eboard,"""
   A B C D E F G H J K L M N O P Q R S T 
19 O . O . O . O . O O O O X . X O O . O 19
18 . O . O . O . O O X O X X X X X O O . 18
17 O . O . O . O O O X X X X O O O O . O 17
16 . O . O . O O . O X X X . X O O . O O 16
15 O . O . O . O O O X O X X X O O O X X 15
14 . O O O . O O O . O O X X X O X O O X 14
13 O . O . O . O O O O X O O X O X O X X 13
12 O O . O . O X X O O X O O O O X X . X 12
11 X O O . O O X . X O X X O O X X X X X 11
10 X X O O X O X X X X X O O X . X . X . 10
 9 . X X O X X . X . X X O O X X . X X X 9 
 8 X X X X X . X . X X . X O X X X X X . 8 
 7 O O O X . X . X X X X X O O O X . X X 7 
 6 O O . O X X X . X . X X O O X . X X O 6 
 5 . O O O O X . X . X . X O X X X O O O 5 
 4 O O X O X . X . X + X X O O X O O O . 4 
 3 O X X X . X . X . X X X O X X X O . O 3 
 2 O O X X X X X . X . X O O O X . X O . 2 
 1 . O X . X . X X . X X O O X X X X O O 1 
   A B C D E F G H J K L M N O P Q R S T 
Black to play
"""[1:-1])
        self.eboard.captured[0] = 7
        self.eboard.captured[1] = 14
    def test_color(self):
        self.assertEqual(go.BLACK, go.to_turn_color(10));
        self.assertEqual(go.WHITE, go.to_turn_color(21));
        self.assertEqual(go.WHITE, go.flip_color(go.BLACK))
        self.assertEqual(go.BLACK, go.flip_color(go.WHITE))
        self.assertEqual(0, go.to_color_index(go.BLACK))
        self.assertEqual(1, go.to_color_index(go.WHITE))
        self.assertEqual(2, go.to_color_index(go.WALL))
        self.assertEqual(0, go.char_to_ix('A'))
        self.assertEqual(7, go.char_to_ix('H'))
        self.assertEqual(8, go.char_to_ix('J'))
        self.assertEqual(24, go.char_to_ix('Z'))

    def test_z(self):
        for x in range(23):
            for y in range(23):
                z = go.XYtoZ(x, y)
                self.assertEqual(x, go.ZtoX(z))
                self.assertEqual(y, go.ZtoY(z))
                if 2<= x <= 20 and 2 <= y <= 20:
                    self.assertTrue(go.is_on_board_z(z, 19))
                else:
                    self.assertFalse(go.is_on_board_z(z, 19))
    def test_countLibertyAndString(self):
        self.assertEqual((2, 1), self.board.countLibertyAndString(s2z('B19')))
        self.assertEqual((3, 2), self.board.countLibertyAndString(s2z('B18')))
        self.assertEqual((3, 8), self.board.countLibertyAndString(s2z('R11')))
        self.assertEqual((2, 1), self.board.countLibertyAndString(s2z('K5')))

    def test_check(self):
        self.assertEqual(go.VALID, self.board.check(go.WHITE, s2z('C1')))
        self.assertEqual(go.OUT, self.board.check(go.WHITE, go.XYtoZ(1, 19)))
        self.assertEqual(go.DOUBLE, self.board.check(go.WHITE, s2z('E16')))
        self.assertEqual(go.SUICIDE, self.board.check(go.WHITE, s2z('K1')))
        self.assertEqual(go.EYE, self.board.check(go.BLACK, s2z('K1')))
        self.assertEqual(go.VALID, self.board.check(go.WHITE, s2z('E1')))
    def test_count_chinese_score2(self):
        self.assertEqual(-1, self.eboard.count_chinese_score2())
        


if __name__ == '__main__':
    unittest.main()

                
