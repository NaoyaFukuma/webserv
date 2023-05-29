# 概要
webservで用いる設定ファイルについての解説。
以下の文法規則に違反する場合は、プロセスの実行を中止します。
その際に、logディレクトリ内にerror_log.txtとして違反箇所を出力します。

## 記号の意味
 - `*` 0回以上の繰り返し
 - `+` 1回以上の繰り返し
 - `?` 0回 or 1回
 - `|` 選択。いずれか一つ。複数該当や該当なしは不可。

### 目次
1. [config](#config)
   - [server](#server)
     - [server_directive](#server_directive)
       - [listen_directive](#listen_directive)
       - [servername_directive](#servername_directive)
       - [timeout_directive](#timeout_directive)
       - [location_directive](#location_directive)
         - [directive_in_location](#directive_in_location)
           - [allow_method_directive](#allow_method_directive)
           - [client_max_body_size_directive](#client_max_body_size_directive)
           - [root_directive](#root_directive)
           - [index_directive](#index_directive)
           - [autoindex_directive](#autoindex_directive)
           - [cgi_extension](#cgi_extension)
           - [return_directive](#return_directive)

### config
- Required: true
- Multiple: false
- Syntax: `config: server+;`
- 概要: 'config'は、*.confファイル自体を指します。

### server
- Required: true
- Multiple: true
- Syntax: `server: '{' server_directive+ '}';`
- 概要: 仮想サーバーごとの設定を定義するブロックです。
ブロックを使った、複数項目の設定を受け付けます。
リクエストのHostヘッダーがいずれのserver_nameとも一致しない場合、
.confファイルで最も上に配置されたものにルーティングします。

### server_directive
- Required: true
- Multiple: true
- 項目:
  ```
  server_directive:
    listen_directive
    | servername_directive
    | timeout_directive
    | location_directive
  ```
- 概要: 仮想サーバーの設定項目としては、この4つが用意されています。

### listen_directive
- Required: false
- Multiple: false
- Syntax: `'listen' ((DOMAIN_NAME | IP_ADDR) ':')? (PORT) END_DIRECTIVE`
- 概要: 仮想サーバーが待ち受けるドメイン名またはIPアドレスとポートを指定します。
注）複数指定された場合、エラーとして扱わず、最後に現れた項目の値を採用します。
指定がない場合、”0.0.0.0:80”をlistenします。

### timeout_directive
- Required: false
- Multiple: false
- Syntax: `'timeout' NUMBER END_DIRECTIVE`
- 概要: 秒単位での設定。clientと一定時間通信が発生しない場合、そのclientとの接続を切断する機能があり、この”一定時間”を指定するための項目です。また、”通信”とは、接続確立、リクエストの受信、レスポンスの送信を指します。”一定時間”の起算点の注意点として、送信時に送信バッファの制限により複数回に分かれる場合は、途中までの送信時刻と、リクエスト受信の時刻を別に管理しています。従って、送信バッファの制限によりレスポンスの送信が完了しない状態が続いた場合、例えリクエスト受信によるアクティビティが発生してもタイムアウトにより接続を切断する場合があります。
注）複数指定された場合、エラーとして扱わず、最後に現れた項目の値を採用します。
指定がない場合、60秒をデフォルト設定として採用。

### servername_directive
- Required: true
- Multiple: false
- Syntax: `'server_name' (DOMAIN_NAME | IP_ADDR)+ END_DIRECTIVE`
- 概要: サーバーに関連付けられたドメイン名またはIPアドレスを指定します。
注）複数指定された場合、エラーとして扱わず、最後に現れた項目の値を採用します。

### location_directive
- Required: true
- Multiple: true
- Syntax: `'location' PATH '{' directive_in_location+ '}'`;
- 概要: 特定のパスに対する設定を定義します。
PATH に`/`を指定したものが存在しない場合、エラーとして扱います。
serverへルーティングしたあと、locationへルーティングできないケースが無いことを保証するため。


### directive_in_location
- Required: true
- Multiple: true
- 項目:
  ```
  directive_in_location:
    allow_method_directive
    | client_max_body_size_directive
    | root_directive
    | index_directive
    | cgi_extension
    | error_page_directive
    | autoindex_directive
    | return_directive;
  ```
- 概要: `location`ブロック内の設定要素を表します。

### allow_method_directive
- Required: false
- Multiple: false
- Syntax: `allow_method_directive: 'allow_method' METHOD+ END_DIRECTIVE`
- 概要: 許可されるHTTPメソッドを指定します。
注）複数指定された場合、エラーとして扱わず、最後に現れた項目の値を採用します。
指定がない場合は、GET, POST, DELETEをデフォルト設定として採用。

### client_max_body_size_directive
- Required: false
- Multiple: false
- Syntax: `client_max_body_size_directive: 'client_max_body_size' NUMBER ('B' | 'K' |'M' |'G')?  END_DIRECTIVE`
- 概要: 受信できるリクエストバディの上限。
注）複数指定された場合、エラーとして扱わず、最後に現れた項目の値を採用します。
指定がない場合、１Mをデフォルト設定として採用。
単位の指定がない場合は、B（バイト）単位として解釈。

### root_directive
- Required: true
- Multiple: false
- Syntax: `root_directive: 'root' PATH END_DIRECTIVE`
- 概要: リクエストURIを置換する。
例）
location /images/,  root /data
リクエスト'/images/cat.jpg' -> '/data/images/cat.jpg'
サーバーが稼働している環境のファイルシステムの絶対パスが完成する。
注）複数指定された場合、エラーとして扱わず、最後に現れた項目の値を採用します。

### index_directive
- Required: false
- Multiple: false
- Syntax: `index_directive: 'index' PATH END_DIRECTIVE`
- 概要: ディレクトリインデックスとして使用されるファイルを指定します。
注）複数指定された場合、エラーとして扱わず、最後に現れた項目の値を採用します。
相対パスを指定し、rootディレクティブのPATHを起点にします。

### cgi_extension
- Required: false
- Multiple: false
- Syntax: `cgi_extension: 'cgi_extension' '.' (ALPHABET | NUMBER)* END_DIRECTIVE`
- 概要: cgiスクリプトを特定する際に使用する。
また、このディレクティブがあるlocationディレクティブルーティングされた場合、cgiスクリプトの実行を許容することも包含している。
cgiスクリプトの位置は、location path 直下だけでなく、サブディレクトリ以下も含む。
拡張子の限定を設けずに、CGIスクリプトの実行を許容したい場合、'.'だけを置くことを許容する。
注）複数指定された場合、エラーとして扱わず、最後に現れた項目の値を採用します。

### autoindex_directive
- Required: false
- Multiple: false
- Syntax: `autoindex_directive: 'autoindex' ON_OFF END_DIRECTIVE`
- 概要: ディレクトリリスティングの自動生成を有効化または無効化します。
注）複数指定された場合、エラーとして扱わず、最後に現れた項目の値を採用します。
指定がなければOFFをデフォルトの設定として採用。

### error_page_directive
- Required: false
- Multiple: false
- Syntax: `error_page_directive: 'error_page' STATUS_CODE+ PATH END_DIRECTIVE`
- 概要:
注）複数指定された場合、エラーとして扱わず、最後に現れた項目の値を採用します。
指定がなければデフォルトのエラーコードとメッセージを出力します。

### return_directive
- Required: false
- Multiple: false
- Syntax: `return_directive: 'return' STATUS_CODE? (URL | TEXT)? END_DIRECTIVE`
- 概要: 指定されたURLにリダイレクトします。
注）複数指定された場合、エラーとして扱わず、最初に現れた項目の値を採用します。
以下のパターンを許容します。
```
  return CODE URL
  return CODE TEXT
  return CODE
  return URL
  ```
