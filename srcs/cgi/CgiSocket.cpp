#include "CgiSocket.hpp"
#include "CgiResponseParser.hpp"
#include "Epoll.hpp"
#include "Http.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Socket.hpp"
#include "define.hpp"
#include "utils.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <wait.h>

// CGI実行を要求したHTTPリクエストとクライアントの登録、そのクライアントのconfigを登録しておく
CgiSocket::CgiSocket(const ConnSocket &http_client_sock,
                     const Request http_request, Response &http_response)
    : ASocket(http_client_sock.GetConfVec()),
      http_client_sock_(http_client_sock), src_http_request_(http_request),
      dest_http_response_(http_response) {
  this->send_buffer_.AddString(http_request.GetBody());
};

// このクラス独自のデストラクタ内の処理は特にないメンバ変数自身のデストラクタが暗黙に呼ばれることに任せる
CgiSocket::~CgiSocket() {
  int status;

  // 子プロセスが終了していない場合に備え、WNOHANGを使う
  int wait_res = waitpid(this->cgi_pid_, &status, WNOHANG);
  switch (wait_res) {
  case 0: // 子プロセスが終了していないのでkill()で終了させて、終了ステータスを回収する
    if (kill(this->cgi_pid_, SIGKILL) < 0) {
      std::cerr << "Keep Running Error: kill" << std::endl;
    }
    if (waitpid(this->cgi_pid_, &status, WNOHANG) < 0) {
      std::cerr << "Keep Running Error: waitpid" << std::endl;
    }
    break;

  case -1: // waitpid()でエラーが発生した
    std::cerr << "Keep Running Error: waitpid" << std::endl;
    break;

  default: // 子プロセスが終了していて、終了ステータスを回収できた
    break;
  }
}

/* retrun value SUCCESS(0) or FAILURE(-1) */
int CgiSocket::OnReadable(Epoll *epoll) {
  // 一度目でバッファ内は全て読み込まれる
  int recv_result = recv_buffer_.ReadSocket(this->GetFd());
  std::cout << "1 recv_result: " << recv_result << std::endl;
  // 二度目はEOFの読み込み-1, それ以外は0(NONBLOCK I/O)
  recv_result = recv_buffer_.ReadSocket(this->GetFd());

  std::cout << "2 recv_result: " << recv_result << std::endl;
  std::cout << "recv_buffer: " << recv_buffer_.GetString() << std::endl;

  if (recv_result == 0) {
    return SUCCESS;
  } else { // EOFの読み込み後にのみparseを行う
    CgiResponseParser cgi_res_parser(*this, this->src_http_request_,
                                     this->dest_http_response_);
    cgi_res_parser.ParseCgiResponse();

    switch (cgi_res_parser.GetParseResult()) {
    case CgiResponseParser::CREATED_HTTP_RESPONSE:
      epoll->Mod(this->http_client_sock_.GetFd(), EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP);
      break;
    case CgiResponseParser::RIDIRECT_TO_LOCAL_CGI:
      epoll->Add(cgi_res_parser.GetRedirectNewCgiSocket(), EPOLLIN | EPOLLOUT | EPOLLET);
      break;
    default:
      break;
    }
    return FAILURE;
  }
}

/* retrun value SUCCESS(0) or FAILURE(-1) */
int CgiSocket::OnWritable(Epoll *epoll) {
  int send_result;
  if (send_buffer_.GetBuffSize()) {
    send_result = send_buffer_.SendSocket(this->GetFd());
  } else {
    send_result = 1; // 送信完了扱いとする
  }

  switch (send_result) {
  case 0: {
    // 送信未完了 EPOLLOUTのイベント登録を維持しつつ、次回のEPOLLOUTを発火を待つ
    last_event_.out_time = time(NULL); // 現在時間に更新
    break;
  }
  case 1: {
    // 送信完了 EPOLLOUTのイベント登録を解除し、EPOLLINのイベント登録を行う
    uint32_t event_mask = EPOLLIN | EPOLLET;
    epoll->Mod(this->GetFd(), event_mask);
    if (shutdown(this->GetFd(), SHUT_WR) < 0) {
      std::cerr << "Keep Running Error: shutdown" << std::endl;
    }
    last_event_.out_time = -1; // タイムアウトを無効化
    break;
  }
  default: { // (-1) エラー
    return FAILURE;
    break;
  }
  }
  return SUCCESS;
}

/*
SUCCESS(0): 次回EPOLLイベントを待つ
FAILURE(-1): EPOLLから解除
なお、EPOLLから削除する際に、
CgiSocketのデストラクタが呼ばれ、適宜子プロセスをkill()、必ずwaitpid()し、
子プロセスの終了を保証しゾンビプロセス孤児プロセスの発生を防ぐ
*/
int CgiSocket::ProcessSocket(Epoll *epoll, void *data) {
  uint32_t event_mask = *(static_cast<uint32_t *>(data));

  if (event_mask & EPOLLIN) {
    std::cout << "FD: " << this->GetFd();
    std::cout << " CgiSocket::ProcessSocket() EPOLLIN" << std::endl;
    if (OnReadable(epoll) == FAILURE) {
      if (this->dest_http_response_.GetProcessStatus() == PROCESSING) {
        this->SetInternalErrorHttpResponse();
        uint32_t event_mask = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
        epoll->Mod(this->http_client_sock_.GetFd(), event_mask);
      }
      return FAILURE;
    }
  }

  if (event_mask & EPOLLOUT) {
    std::cout << "FD: " << this->GetFd()
              << " CgiSocket::ProcessSocket() EPOLLOUT" << std::endl;
    if (OnWritable(epoll) == FAILURE) {
      if (this->dest_http_response_.GetProcessStatus() == PROCESSING) {
        this->SetInternalErrorHttpResponse();
        uint32_t event_mask = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
        epoll->Mod(this->http_client_sock_.GetFd(), event_mask);
      }
      return FAILURE;
    }
  }

  if (event_mask & EPOLLHUP) {
    std::cout << "FD: " << this->GetFd()
              << " CgiSocket::ProcessSocket() EPOLLHUP" << std::endl;
    if (this->dest_http_response_.GetProcessStatus() == PROCESSING) {
      this->SetInternalErrorHttpResponse();
      uint32_t event_mask = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
      epoll->Mod(this->http_client_sock_.GetFd(), event_mask);
    }
    return FAILURE;
  }

  return SUCCESS;
}

void CgiSocket::SetInternalErrorHttpResponse() {
  this->dest_http_response_.SetResponseStatus(500);
  this->dest_http_response_.SetProcessStatus(DONE);
  this->dest_http_response_.SetVersion(
      this->src_http_request_.GetRequestMessage().request_line.version);
}
