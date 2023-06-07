#include <stdio.h>
#include <unistd.h>


int main(void) {
  printf("Status: 200 OK\r\n");
  printf("Content-type: text/html\r\n");
  printf("Content-Length: 48\r\n\r\n");
  printf("<html><body><h1>Hello, World!</h1></body></html>\r\n");
  return 0;
}
