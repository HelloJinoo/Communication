#include <stdlib.h>
#include <stdio.h>
#include <WinSock2.h>

#pragma comment(lib, "WS2_32.LIB")

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define PORT 4578
#define PACKET_SIZE 1024
#define SERVER_IP "192.168.0.45"
void main() {
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	SOCKET hSocket;
	hSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN tAddr;
	tAddr.sin_family = AF_INET;
	tAddr.sin_port = htons(PORT);
	tAddr.sin_addr.s_addr = inet_addr(SERVER_IP);


	char local_name[1024]; 
	struct hostent *host_ptr = NULL; 
	memset(local_name, 0, 1024);
	gethostname(local_name, 1024);	//gethosname(저장할 배열 , 길이) : 현재 컴퓨터의 local_name가져오기
	host_ptr = gethostbyname(local_name);	//gethostbyname(이름) : 이름으로 컴퓨터의 정보 가져와서 저장
	printf("Client IP: %u.%u.%u.%u\n",
		(unsigned char)host_ptr->h_addr_list[0][0],
		(unsigned char)host_ptr->h_addr_list[0][1],
		(unsigned char)host_ptr->h_addr_list[0][2],
		(unsigned char)host_ptr->h_addr_list[0][3]);
	
	
	/*client측에서는 bind대신 connect함수를사용하여 socket설정 및 서버와 연결*/
	connect(hSocket, (SOCKADDR*)&tAddr, sizeof(tAddr));
	printf("Server connected.\n");


	while (1) {
		char cMsg[1024];
		memset(cMsg, 0, 1024);

		printf("보낼 메세지 일력 : ");
		scanf("%[^\n]", cMsg);
		send(hSocket, cMsg, strlen(cMsg), 0);
		char cBuffer[PACKET_SIZE];
		memset(cBuffer, 0, 1024);
		recv(hSocket, cBuffer, PACKET_SIZE, 0);
		printf("Recv Msg : %s\n", cBuffer);
	}
	closesocket(hSocket);
	WSACleanup();
	system("pause");



}
