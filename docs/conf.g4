grammar configuration;

/*
記号の意味
 `*` 0回以上の繰り返し
 `+` 1回以上の繰り返し
 `?` 0回 or 1回 必須ではない。や、オプションと表現される
 `|` 選択。いずれか一つ。複数該当や該当なしは不可。
*/

NEWLINE: '\n' -> skip;
WHITESPACE: ' ' -> skip;
/*
改行とスペースの取り扱いを明記
トークナイズするときにスキップされる。つまりパーサーには渡らない。
*/


config: server+;

server: 'server' '{' server_directive+ '}';

server_directive:
	listen_directive
	| servername_directive
	| timeout_directive
	| location_directive;

listen_directive: 'listen' ((DOMAIN_NAME | IP_ADDR) ':')? (PORT) END_DIRECTIVE;

servername_directive:
	'server_name' (DOMAIN_NAME | IP_ADDR)+ END_DIRECTIVE;

timeout_directive:
	'timeout' NUMBER END_DIRECTIVE;

location_directive:
	'location' PATH '{' directive_in_location+ '}';

directive_in_location:
  match_directive
	| allow_method_directive
	| client_max_body_size_directive
	| root_directive
	| index_directive
	| is_cgi_directive
	| cgi_path_directive
	| error_page_directive
	| autoindex_directive
	| return_directive;

match_directive: 'match' ('prefix' | 'suffix') END_DIRECTIVE;

allow_method_directive:
	'allow_method' METHOD+ END_DIRECTIVE;

client_max_body_size_directive:
	'client_max_body_size' NUMBER ('B' | 'K' |'M' |'G')?  END_DIRECTIVE;

root_directive: 'root' PATH END_DIRECTIVE;

index_directive:
	'index' PATH END_DIRECTIVE;

error_page_directive: 'error_page' STATUS_CODE+ PATH END_DIRECTIVE;

autoindex_directive:
	'autoindex' ON_OFF END_DIRECTIVE;

is_cgi_directive: 'is_cgi' ON_OFF END_DIRECTIVE;

cgi_path_directive: 'cgi_path' PATH END_DIRECTIVE;

return_directive: 'return' STATUS_CODE? (URL | TEXT)? END_DIRECTIVE;

ON_OFF: 'on' | 'off';
METHOD: 'GET' | 'POST' | 'DELETE';
PATH: ('/' (ALPHABET | NUMBER | '.' | '_' | '-' | '~' | '+' | '%' | '@' | '#' | '$' | '&' | ',' | ';' | '=' | ':' | '|' | '^' | '!' | '*' | '`' | '(' | ')' | '{' | '}' | '[' | ']' | '<' | '>' | '?')*)?;
URL: ('http' | 'https') '://' DOMAIN_NAME ('/');
DOMAIN_NAME: DOMAIN_LABEL ('.' DOMAIN_LABEL)*;
IP_ADDR: NUMBER+ '.' NUMBER+ '.' NUMBER+ '.' NUMBER+;
DOMAIN_LABEL: (ALPHABET | NUMBER)+ | (ALPHABET | NUMBER)+ (ALPHABET | NUMBER | HYPHEN)* (ALPHABET | NUMBER)+;
TEXT: '\'' (ALPHABET | NUMBER | WHITESPACE | ASCII_SYMBOLS)* '\'' END_DIRECTIVE;
ASCII_SYMBOLS: '!' | '#' | '$' | '%' | '&' | '\'' | '(' | ')' | '*' | '+' | ',' | '-' | '.' | '/' | ':' | ';' | '<' | '=' | '>' | '?' | '@' | '[' | '\\' | ']' | '^' | '_' | '`' | '{' | '|' | '}' | '~';
STATUS_CODE: NUMBER;
END_DIRECTIVE: ';';
ALPHABET: 'a' ..'z' | 'A' ..'Z';
NUMBER: ('0' ..'9')+;
HYPHEN: '-';

