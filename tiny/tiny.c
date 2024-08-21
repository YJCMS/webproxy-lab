/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

/* HTTP 트랜잭션을 처리하는 함수 */
void doit(int fd) {
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE); // 요청 라인 읽어들임
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  if (strcasecmp(method, "GET") * strcasecmp(method, "HEAD")) { // GET / HEAD 요청이 아니면 에러 메세지(stcasecmp 함수 문자열 비교해서 같으면 0 리턴)
    clienterror(fd, method, "501", "Not Implemented",
                  "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs);
  if (stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found", 
                  "Tiny couldnt find this file");
    return;
  }

  if (is_static) { /* Serve static content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { // 보통파일인지 읽기권한 가졌는지 검증
      clienterror(fd, filename, "403", "Forbidden",
                   "Tiny couldn't read the file");
      return;
    }

    serve_static(fd, filename, sbuf.st_size, method); // 정적 컨텐츠 클라이언트에게 제공

  } else { /* Server dynamic content */
      if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { // 파일이 실행가능하진 검증
        clienterror(fd, filename, "403", "Forbidden",
                     "Tiny couldn't run the CGI program");
        return;
      }
      serve_dynamic(fd, filename, cgiargs); // 동적 컨텐츠 클라이언트에게 제공
  }
}

/* 에러메세지를 클라이언트에 보냄 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
  char buf[MAXLINE], body[MAXBUF];
  
  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, shortmsg);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs) {
  char *ptr;
  
  /* Static content */
  if(!strstr(uri, "cgi-bin")) { 
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if(uri[strlen(uri)-1] == '/') {
      strcat(filename, "home.html");
    }
    return 1;
  /* Dynamic content */
  } else {
    ptr = index(uri, '?');

    if(ptr) {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    } else {
        strcpy(cgiargs, "");
    }

    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize, char *method) {
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  // 응답 헤더
  sprintf(buf, "%sSever: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  // 응답을 클라이언트에 보냄
  Rio_writen(fd, buf, strlen(buf));
  printf("Response header:\n");
  printf("%s", buf);

  // HTTP HEAD
  if (strcasecmp(method, "HEAD") == 0) {
      return;
  } 

  /* Send respense body to client */
  srcfd = Open(filename, O_RDONLY, 0); // O_RDONLY 읽기 권한으로 filename을 불러옴
  srcp = (char *)Malloc(filesize); //  문제 11.9 - malloc 이용 
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 기존 코드 - 파일의 메모리 그대로 가상메모리 매핑
  Rio_readn(srcfd, srcp, filesize); // 문제 11.9 - 파일의 데이터를 메모리로 읽어와서 srcp에 매핑
  Close(srcfd);
  Rio_writen(fd, srcp, filesize); // 해당 메모리에 있는 파일 내용을 fd로 보냄
  // Munmap(srcp, filesize); // 기존 코드 - Munmap으로 메모리 해제
  free(srcp); // 문제 11.9 - malloc 사용 -> free 필수

}

/* get_filetype - Derive file type from filename */
void get_filetype(char *filename, char *filetype) {
  if(strstr(filename, ".html")) {
    strcpy(filetype, "text/html");
  } else if(strstr(filename, ".gif")) {
      strcpy(filetype, "image/gif");
  } else if(strstr(filename, ".png")) {
      strcpy(filetype, "image/png");
  } else if(strstr(filename, ".jpg")) {
      strcpy(filetype, "image/jpeg");
  } else if(strstr(filename, ".mp4")) {
      strcpy(filetype, "video/mp4");
  } else {
      strcpy(filetype, "text/plain");
  }
}

void serve_dynamic(int fd, char *filename, char *cgiargs) {
  char buf[MAXLINE], *emptylist[] = { NULL };

  /* Return first partof HTTP response */
  sprintf(buf, "HTTP/1.0 200 Ok\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if(Fork() == 0) {
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO); // Redirect stdout to client
    Execve(filename, emptylist, environ); // Run CGI program
  }
  Wait(NULL); // parent wait for and reaps child
}