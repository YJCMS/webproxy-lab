#include "csapp.h"

void echo(int connfd);

/* 서버의 리스닝 소켓을 열고, 클라이언트 연결 요청 수락, 데이터 주고받는 함수 호출하는 서버 프로그램 */
int main(int argc, char **argv) {
    int listenfd, connfd; //서버 리스닝 소켓, 클라이언트 연결 소켓
    socklen_t clientlen; // 클라이언트 주소 구조체의 크기를 저장하는 변수
    struct sockaddr_storage clientaddr; /* Enough space for any address */
    char client_hostname[MAXLINE], client_port[MAXLINE]; // 클라이언트 호스트명과 포트번호를 저장할 버퍼

    // 인수를 제대로 받지 않았을 때 출력 및 종료
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    // 지정된 포트에서 클라이언트의 연결 요청을 기다리기 위한 리스닝 소켓 생성
    listenfd = Open_listenfd(argv[1]);
    while(1) { // 무한 루프 클라이언트 요청을 계속 처리
        clientlen = sizeof(struct sockaddr_storage); // 클라이언트 주소 구체체의 크기를 설정
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 클라이언트의 연결 요청을 수락 및 연결 소켓 생성
        
        // 연결 클라이언트의 호스트 이름과 포트 번호 확인
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,
                    client_port, MAXLINE, 0); 
        printf("Connected to (%s, %s)\n", client_hostname, client_port); // 호스트 및 포트번호 출력
        echo(connfd); // 클라이언트와 데이터를 주고받는 echo 함수 호출
        Close(connfd); // 클라이언트와의 연결을 종료
    }
    exit(0); // 프로그램 종료
}

/* 클라이언트로부터 데이터를 받아 다시 보내는 함수 */ 
void echo(int connfd) {
    size_t n; // 읽은 바이트 수 저장할 변수
    char buf[MAXLINE];  //데이터를 저장할 버퍼
    rio_t rio; // RIO 구조체, 버퍼링된 I/O를 위한 데이터 구조

    // 클라이언트 연결 소켓을 사용하여 RIO 구조체를 초기화
    Rio_readinitb(&rio, connfd);

    // 클라이언트로부터 데이터를 한 줄씩 읽고, 다시 클라이언트로 보냄
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { // 클라이언트로부터 데이터 읽음
        printf("server received %d bytes\n", (int)n); // 읽은 데이터의 크기 출력
        Rio_writen(connfd, buf, n); // 읽은 데이터를 클라이언트에게 그대로 보냄
    }
}