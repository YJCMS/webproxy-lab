#include "csapp.h"

int main(int argc, char **argv) {
    int clientfd; // 서버와 연결할 클라이언트 소켓 디스크럽터
    char *host, *port, buf[MAXLINE]; // 서버의 호스트명, 포트번호, 입출력 버퍼
    rio_t rio; // RIO 구조체, 버퍼링된 I/O를 위한 데이터 구조

    // 명령행 인수가 올바르게 제공되지 않은 경우 출력 후 종료
    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1]; // 첫번째 인수로 전달되 호스트명을 저장
    port = argv[2]; // 두번째 인수로 전달된 포트번호를 저장

    // 서버에 연결하기 위한 클라이언트 소켓을 생성(호스트와 포트로 서버연결)
    clientfd = Open_clientfd(host, port);
    // 클라이언트 소켓을 RIO 구조체로 초기화(버퍼링된 I/O를 사용하기 위해
    Rio_readinitb(&rio, clientfd);

    // 표준입력으로부터 데이터를 읽어 서버에 전송하고, 서버로부터 응답을 출력
    while (Fgets(buf, MAXLINE, stdin) != NULL) { // 표준입력에서 한 줄씩 읽음
        Rio_writen(clientfd, buf, strlen(buf)); // 읽은 데이터를 서버에 전송
        Rio_readlineb(&rio, buf, MAXLINE); // 서버로부터 응답을 한 줄 읽음
        Fputs(buf, stdout); // 서버응답을 표준 출력으로 출력
    }

    // 클라이언트 소켓을 닫고 프로그램 종료
    Close(clientfd);
    exit(0);
}