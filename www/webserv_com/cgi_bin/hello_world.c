#include <stdio.h>
#include <unistd.h>


int main(void) {
  printf("Status: 200 OK\r\n");
  printf("Content-type: text/html\r\n");
  printf("Content-Length: 48\r\n\r\n");
  printf("<html><body><h1>Hello, World!</h1></body></html>\r\n");
  write(STDERR_FILENO, "Hello, World!\n", 14);
  write(STDERR_FILENO, "CGI script finished\n", 20);

  return 0;
}
