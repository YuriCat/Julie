// Julie基本設定
// 設定パターンによっては動かなくなるかもしれないので注意

#ifndef GO_SETTINGS_H_
#define GO_SETTINGS_H_

// プロフィール
#define MY_NAME "Julie"
#define MY_VERSION "160314"
#define MY_COACH "KatsukiOhto"

// 重要な設定

//#define MINIMUM // 本番用
#define MONITOR // 着手決定関連の表示
#define BROADCAST // 試合進行実況
//#define DEBUG // デバッグ出力

#define PLAY_POLICY // 方策関数そのままでプレー

#define PONDER // 相手手番中の先読み

#define RULE_KOMI2 (13)

// 投了する予測勝率ライン
#define WP_RESIGN (0.000001)

// 置換表サイズ
#define TT_SIZE (1000000000)

// 並列化
// スレッド数(ビルド時決定)
// 0以下を設定すると勝手に1になります
#define N_THREADS (1)

// ログ関連

// ログを取って外部ファイルに書き出す
//#define LOGGING_FILE


// メソッドアナライザー使用
// 本番用では自動的にオフ
#define USE_ANALYZER

/**************************以下は直接変更しない**************************/

// スレッド数
#ifdef N_THREADS

#if N_THREADS <= 0
#undef N_THREADS
#define N_THREADS (1)
#endif

// スレッド数として2以上が指定された場合は、マルチスレッドフラグを立てる
#if N_THREADS >= (2)
#define MULTI_THREADING
#endif

#endif // N_THREADS


// 本番用のとき、プレーに無関係なものは全て削除
// ただしバグ落ちにdefineが絡む可能性があるので、強制終了時の出力は消さない
#ifdef MINIMUM

// デバッグ
#ifdef DEBUG
#undef DEBUG
#endif

// 実況
#ifdef BROADCAST
#undef BROADCAST
#endif

// アナライザ
#ifdef USE_ANALYZER
#undef USE_ANALYZER
#endif

#endif // MINIMUM

#endif // GO_SETTINGS_H_