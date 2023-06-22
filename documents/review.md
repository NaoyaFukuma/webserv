#### コンフィグファイルを引数で受け取る
```
./webserv server_config/webserv.conf
```
#### 引数がない場合はデフォルトのパスを使う
```
./webserv
```
#### nginxなど、他のサーバープログラムを実行していないか
```
`execve`を検索
```
#### blockingしない
```
`recv`, `send`に非blockingのフラグを立てている
```
#### pollに基づいてI/Oを実行する
```
`OnReadable`, `OnWritable`関数
```
#### readとwriteでpollingを分けない
```
Epollのイベントマスクが`EPOLLIN|EPOLLOUT`
```
#### errnoはread, writeの後に見ない
```
`errno`を検索
```
#### defaultのerror page
```
curl -i localhost:8080/hoge
```
#### forkはCGIでしか使用しない
```
`fork`を検索
```
#### 静的ファイルを返す
```
curl -i localhost:8080
```
#### ファイルのアップロード
```
http://localhost:8080/public_html/upload.html
```
#### GET, POST, DELETEメソッド
```
curl -i --data-binary './www/webserv_com/public_html/home.html' -X POST http://localhost:8080/cgi_bin/UploadCgi.cgi?filename=home.html
curl -i -X GET http://localhost:8080/upload
curl -i -X DELETE http://localhost:8080/upload/{...}
```
#### 負荷テスト
```
siege -b -t 30S -H http://localhost:8080
```
#### マルチポート
```
curl -i localhost:9090/public_html
```
#### hostが解決できなければ、最初の設定をデフォルトのサーバーとして使う
```
curl -i -H "Host: hoge" 127.0.0.1:8080
```
#### bodyサイズの上限
```
curl -i -H "Content-Length: 10000000000000" 127.0.0.1:8080
```
#### allow methodの設定
```
curl -i localhost:8080/not_allow_get
curl -i -X POST localhost:8080
curl -i -X DELETE localhost:8080
```
#### リダイレクト 
```
http://localhost:8080/return
```
#### root
```
curl -i localhost:8080
```
#### auto index
```
curl -i localhost:8080/upload
```
#### index
```
curl -i localhost:8080
```
#### cgi
```
curl -i --data-binary './www/webserv_com/public_html/home.html' -X POST http://localhost:8080/cgi_bin/UploadCgi.cgi?filename=home.html
curl -i -X GET http://localhost:8080/upload
```

### bonus cookie
```
http://localhost:8080/public_html/Login.html
http://localhost:8080/cgi_bin/Userpage.cgi
```
