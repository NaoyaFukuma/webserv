/* client */
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define RESPONSE                                                               \
  "HTTP/1.1 404 Not Found\r\nDate: Mon, 10 Apr 2023 12:00:00 GMT\r\n\
Server: Apache/2.4.41 (Ubuntu)\r\n\
Content-Type: text/html; charset=UTF-8\r\n\
Content-Length: 146\r\n\
Connection: keep-alive\r\n\
\r\n\
<!DOCTYPE html>\r\n\
<html lang=\"en\">\r\n\
<head>\r\n\
    <meta charset=\"UTF-8\">\r\n\
    <title>404 Not Found</title>\r\n\
</head>\r\n\
<body>\r\n\
    <h1>404 Not Found</h1>\r\n\
    <p>The requested URL was not found on this server.</p>\r\n\
</body>\r\n\
</html>\r\n\
\r\n"
#define RESPONSE_SIZE 411

using namespace std;

int main() {
  int client_fd =
      socket(AF_INET, SOCK_STREAM, 0); // IPv4, TCP, default protocol
  if (client_fd == -1) {               // error
    return 1;
  }
  struct sockaddr_in server_addr;                       // IPv4
  server_addr.sin_family = AF_INET;                     // IPv4
  server_addr.sin_port = htons(8080);                   // port
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost
  char buf[1024];
  int bytes_read;
  if (connect(client_fd, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) == -1) {
    return 1;
  }
  std::cout << "connected" << std::endl;
  send(client_fd, RESPONSE, RESPONSE_SIZE, 0);
  bytes_read = recv(client_fd, buf, RESPONSE_SIZE, 0);
  write(STDOUT_FILENO, buf, bytes_read);

  close(client_fd); // close the socket
  return 0;
}