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
       - [location_directive](#location_directive)
         - [directive_in_location](#directive_in_location)
           - [match](#match)
           - [allow_method_directive](#allow_method_directive)
           - [client_max_body_size_directive](#client_max_body_size_directive)
           - [root_directive](#root_directive)
           - [index_directive](#index_directive)
           - [autoindex_directive](#autoindex_directive)
           - [is_cgi_directive](#is_cgi_directive)
           - [return_directive](#return_directive)

### config
- Required: true
- Multiple: false
- Syntax: `config: server (NEWLINE server)*;`
- 概要: 'config'は、*.confファイル自体を指します。

### server
- Required: true
- Multiple: true
- Syntax: `server: 'server' '{' server_directive+ '}';`
- 概要: 仮想サーバーごとの設定を定義するブロックです。
        ブロックを使った、複数項目の設定を受け付けます。

### server_directive
- Required: true
- Multiple: true
- Syntax:
  ```
  server_directive:
    listen_directive
    | servername_directive
    | location_directive
  ```
- 概要: 仮想サーバーの設定項目としては、この３つが用意されています。

### listen_directive
- Required: true
- Multiple: false
- Syntax: `listen_directive: 'listen' WHITESPACE ((DOMAIN_NAME | IP_ADDR) ':')? (PORT) END_DIRECTIVE`
- 概要: 仮想サーバーが待ち受けるドメイン名またはIPアドレスとポートを指定します。

### servername_directive
- Required: true
- Multiple: false
- Syntax: `'server_name' WHITESPACE ((DOMAIN_NAME | IP_ADDR) WHITESPACE) END_DIRECTIVE`
- 概要: サーバーに関連付けられたドメイン名またはIPアドレスを指定します。

### location_directive
- Required: true
- Multiple: true
- Syntax: `'location' PATH '{' match_directive (directive_in_location)* '}'`;
- 概要: 特定のパスに対する設定を定義します。

### match
- Required: true
- Multiple: false
- Syntax: `match_directive: 'match' WHITESPACE ('prefix' | 'suffix') END_DIRECTIVE;`
- 概要: パスのマッチング方法を指定します。`prefix`は接頭辞マッチング、`suffix`は接尾辞マッチングです。

### directive_in_location
- Required: false
- Multiple: true
- Syntax:
  ```
  directive_in_location:
    allow_method_directive
    | client_max_body_size_directive
    | root_directive
    | index_directive
    | autoindex_directive
    | is_cgi_directive
    | return_directive;
  ```
- 概要: `location`ブロック内の設定要素を表します。


### allow_method_directive
- Required: false
- Multiple: false
- Syntax: `allow_method_directive: 'allow_method' WHITESPACE METHOD (WHITESPACE METHOD)* END_DIRECTIVE;`
- 概要: 許可されるHTTPメソッドを指定します。
注）複数指定された場合、エラー検出できず、最後に現れた項目の値が採用されます。

### client_max_body_size_directive
- Required: false
- Multiple: false
- Syntax: `client_max_body_size_directive: 'client_max_body_size' WHITESPACE NUMBER END_DIRECTIVE;`
- 概要: クライアントからのリクエストボディの最大サイズを指定します。
注）複数指定された場合、エラー検出できず、最後に現れた項目の値が採用されます。

### root_directive
- Required: false
- Multiple: false
- Syntax: `root_directive: 'root' WHITESPACE PATH END_DIRECTIVE;`
- 概要: ドキュメントルートのパスを指定します。
注）複数指定された場合、エラー検出できず、最後に現れた項目の値が採用されます。

### index_directive
- Required: false
- Multiple: false
- Syntax: `index_directive: 'index' WHITESPACE PATH (WHITESPACE PATH)* END_DIRECTIVE;`
- 概要: ディレクトリインデックスとして使用されるファイルを指定します。
注）複数指定された場合、エラー検出できず、最後に現れた項目の値が採用されます。

### autoindex_directive
- Required: false
- Multiple: false
- Syntax: `autoindex_directive: 'autoindex' WHITESPACE ON_OFF END_DIRECTIVE;`
- 概要: ディレクトリリスティングの自動生成を有効化または無効化します。
注）複数指定された場合、エラー検出できず、最後に現れた項目の値が採用されます。

### is_cgi_directive
- Required: false
- Multiple: false
- Syntax: `is_cgi_directive: 'is_cgi' WHITESPACE ON_OFF END_DIRECTIVE;`
- 概要: CGIスクリプトの実行を有効化または無効化します。
注）複数指定された場合、エラー検出できず、最後に現れた項目の値が採用されます。

### return_directive
- Required: false
- Multiple: false
- Syntax: `return_directive: 'return' WHITESPACE URL;`
- 概要: 指定されたURLにリダイレクトします。
注）複数指定された場合、エラー検出できず、最後に現れた項目の値が採用されます。
