# レビュー用資料


## レビュー (コンフィグ)
#### コンフィグファイル受け取れる?

`./webserv server_config/webserv.conf`

#### デフォルトコンフィグファイル読み取ろうとする?

`./webserv`

## レビュー

### 複数ポート対応

`curl -i localhost:8080`

`curl -i localhost:9090/public_html`

### 1つのポートに複数のバーチャルサーバー, ドメイン名(Hostヘッダー)による分類

`curl -i -H "Host: webserv.com" 127.0.0.1:8080`

`curl -i -H "Host: localhost" 127.0.0.1:8080`

### 複数のホスト これがよくわからん

`curl -v -X GET http://localhost:80/index.html`

`curl -v -X GET http://127.0.0.2:80/index.html`

### デフォルトエラーページ

`curl -i -H "Host: hoge" 127.0.0.1:8080`

### 設定したエラーページ

`curl -i -H "Host: webserv.com" 127.0.0.1:8080/hoge`

### CGIが動くか

[RFC 3875 - The Common Gateway Interface (CGI) Version 1.1 日本語訳](https://tex2e.github.io/rfc-translater/html/rfc3875.html)

#### Document-Response

ブラウザから
`http://localhost:8080/cgi_bin/hello_world.cgi`

#### Local-Redirect

`curl -i -H "Host: webserv.com" 127.0.0.1:8080/cgi_bin/LocalRedirectStatic.cgi`

#### Client-Redirect

`curl -i -H "Host: webserv.com" 127.0.0.1:8080/cgi_bin/ClientRedirect.cgi`

### 静的ファイルのサーブ

`curl -i -H "Host: webserv.com" 127.0.0.1:8080`

### ファイルアップロード

`curl -i --data-binary '@www/webserv_com/public_html/home.html' -X POST http://localhost:8080/cgi_bin/UploadCgi.cgi?filename=home.html`

### ファイル削除

`curl -i -X DELETE http://localhost:8080/upload/{...}`

### client_body_size の制限

`curl -i -H "Content-Length: 10000000000000" localhost:8080`

### 許可したHTTPメソッドのみリソースへのアクセスが可能

```
curl -i localhost:8080/not_allow_get
curl -i -X POST localhost:8080
curl -i -X DELETE localhost:8080
```

### root で指定したディレクトリからファイルを探す

`curl -i http://localhost:8080/public_html/epoll_explanation.html`

### autoindex の on / off

on: `curl -i http://localhost:8080/upload`

off: `curl -i http://localhost:9090`

### index ファイル

`curl -i http://localhost:8080`

### `return` リダイレクティブ

`curl -i http://localhost:8080/return`

### 複数のCGIスクリプトを動かす (ボーナス)
ドキュメントだけ、クライアントリダイレクト、ローカルリダイレクト、post によるアップロードなどに対応


## ベンチマーク

`siege -b http://localhost:8080 --time=30S` でベンチマークを実行する｡

オプションの意味
- `-b`: ベンチマークモード
- `--time=30S` 30秒間ベンチマークを実行する｡

ちなみにデフォルトではsiegeは25クライアント並行で動かす

### 結果の読み方（実行環境によって変動）

```
        "transactions":                        22533,
        "availability":                       100.00,
        "elapsed_time":                        29.08,
        "data_transferred":                    39.37,
        "response_time":                        0.03,
        "transaction_rate":                   774.86,
        "throughput":                           1.35,
        "concurrency":                         24.95,
        "successful_transactions":             22533,
        "failed_transactions":                     0,
        "longest_transaction":                  0.08,
        "shortest_transaction":                 0.01
```

- Transactions: ベンチマークで送信されたリクエスト数
- Availability: レスポンスが返ってきて､なおかつレスポンスコード400や500ではないレスポンスの送信したリクエスト数に対する割合
- Elapsed time: ベンチマークを実行した時間
- Data transferred: ヘッダーを含むsiegeの各クライアントに送信されたデータ量の総和
- Response time: レスポンスが返ってくるまでの平均時間
- Transaction rate: サーバーが1秒間に処理出来る平均リクエスト数
- Throughput: サーバーがsiegeの各クライアントに1秒間に送信する平均データ量
- Concurrency: 同時に接続されたコネクション数の平均｡ サーバーの性能が悪いほどこの数値はあがる｡
- Successful transactions: ステータスコードが400より下(つまり正常)のレスポンス数
- Failed transactions: 400以上のステータスまたはソケットに関してエラーが発生した数
- Longest transaction: レスポンスを返すのに要した時間の最大
- Shortest transaction: レスポンスを返すのに要した時間の最小
