# othello
サーバー・クライアント通信を利用したオセロ 

## 遊び方
- それぞれコンパイル
- serverを実行
    - コマンドは ./progname ipアドレス 適当なポート番号
    - ipアドレスは127.0.0.1でOK
- clientを別窓で2つ実行
    - コマンドは ./progname ipアドレス 適当なポート番号
    - serverで指定したipアドレス、ポートと同じ物を指定
- clientの方にメッセージが出るので、それに従って石を打つ

## 注意事項
終了処理がまだ未完成
