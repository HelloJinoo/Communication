#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <WinSock2.h>

#pragma comment(lib,"WS2_32.LIB") //Socket library를 자동으로 설정

#define PORT 4578	//포트번호
#define PACKET_SIZE 1024	//패킷사이즈
#define RW_READ 0x03
#define RW_WRITE 0x06

unsigned short ReverseByteOrder(unsigned short value);
unsigned short CreateCRC16(unsigned char *buff, size_t len);

void main() {
	
	unsigned short reg_map[150]; unsigned char recv_buff[8];
	unsigned char *send_buff;
	memset(reg_map, 0x00, sizeof(short) * 150); unsigned short address;
	reg_map[6] = 0xFE;


	WSADATA wsaData;	//윈도우의 소켓 초기화 정보를 저장하기위한 구조체
	WSAStartup(MAKEWORD(2, 2), &wsaData);	//( 소켓버전, WSADATA 구조체 주소) -> 윈도우즈에 어느 소켓을 활용할 것인지 알려줌
											//MAKEWORD() -> word타입으로 만듬

	/*소켓 생성*/
	SOCKET hListen;
	hListen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);  // PF_INET : ipv4를 사용한다 
														  // SOCK_STREAM : 연결지향형 소켓을 만들겠다.
														  // IPPROTO_TCP : protocol로 TCP를 사용하겠다.

	/*소켓 설정*/
	SOCKADDR_IN tListenAddr;
	tListenAddr.sin_family = AF_INET;	//반드시 AF_INET	
	tListenAddr.sin_port = htons(PORT);	//포트번호 설정 , host to network short -> 빅엔디안 방식으로 데이터를 변환
	tListenAddr.sin_addr.s_addr = htonl(INADDR_ANY);	

	/*바인드 */
	bind(hListen, (SOCKADDR*)&tListenAddr, sizeof(tListenAddr)); //소켓에 위에 설정한 주소정보를 묶어주고  , bind(소켓, 소켓 구성요소 구조체의 주소, 그 구조체의 크기)
	
	/*Listen (client를 기다림)*/
	listen(hListen, SOMAXCONN);	//hListen소켓을 접속대기상태로 만들어줌 
								//SOMAXCONN은 한꺼번에 요청 가능한 최대 접속승인 수를 의미
	char local_name[1024];
	struct hostent *host_ptr = NULL;
	memset(local_name, 0, 1024);
	gethostname(local_name, 1024);		//gethosname(저장할 배열 , 길이) : 현재 컴퓨터의 local_name가져오기
	host_ptr = gethostbyname(local_name);	//gethostbyname(이름) : 이름으로 컴퓨터의 정보 가져와서 저장
	printf("Server IP: %u.%u.%u.%u\n",
		(unsigned char)host_ptr->h_addr_list[0][0],
		(unsigned char)host_ptr->h_addr_list[0][1],
		(unsigned char)host_ptr->h_addr_list[0][2],
		(unsigned char)host_ptr->h_addr_list[0][3]);
	printf("Port Number is %d\n", PORT);
	printf("Wait client connection!\n");

	/*-----------------Client 측 소켓 생성 및 정보를 담을 구조체 생성 및 값 할당*/
	SOCKADDR_IN tClientAddr;
	int iClientSize = sizeof(tClientAddr);
	
	/*Accept (client와 연결) */
	SOCKET hClient = accept(hListen, (SOCKADDR*)&tClientAddr, &iClientSize);	//접속 요청 수락 (동기화된 방식 : 요청을 마무리하기 전까지 대기상태)
																				//(server 소켓 , client소켓 , 크기 )
	printf("Client connected.\n");

		/*modbus protocol*/
	{
		unsigned char recv_buff[8];
		unsigned char *send_buff;
		unsigned short address;
		unsigned short value;
		unsigned short len;
		unsigned short i;
		unsigned short crc;
		recv(hClient, (char*)recv_buff, 8, 0);		//recv_buff[0] = id , recv_buff[1] = 읽기인지 쓰기인지?
													//recv_buff[2] = ? , recv_buff[3] = ?
													//recv_buff[4] = ? , recv_buff[5] = ?
													//recv_buff[6] = ? , recv_buff[7] = ?

		if (recv_buff[6] == reg_map[6] || recv_buff[0] == 0xFF) {
			if (recv_buff[1] == RW_READ) {
				memcpy(&address, &recv_buff[2], 2); //주소를 가져와서 address에 저장
				address = ReverseByteOrder(address); //주소를 byteordering  
				memcpy(&len, &recv_buff[4], 2);
				len = ReverseByteOrder(len);
				send_buff = (unsigned char*)malloc(3 + len * 2 + 2); //보낼 크기만큼 send_buff 할당
				 // ID r/w function code, length, register no to read, CRC2bytes
				send_buff[0] = recv_buff[0];
				send_buff[1] = RW_READ;
				send_buff[2] = (unsigned char)len * 2;

				for (i = 0; i < len; i++)
				{
					value = reg_map[address + i]; //start address ~ to length
					value = ReverseByteOrder(value);
					memcpy(&send_buff[3 + i * 2], &value, 2);// copy to send buffer
				}				crc = CreateCRC16(send_buff, 3+len*2);
				memcpy( &send_buff[3+len*2], &crc, 2);
				send(hClient, (char*)send_buff, 3+len*2 + 2, 0); //response sending

				free(send_buff); //보낸 후 할당한 send_buff메모리 반납
				send_buff = NULL;
			}
		}
	}



	/*데이터 수신*/
	while (1) {
		char cBuffer[PACKET_SIZE];	//Client측 정보를 수신하기위해 정의해두는 패킷크기만큼의 버퍼
		memset(cBuffer, 0, 1024);
		recv(hClient, cBuffer, PACKET_SIZE, 0);	//recv(data를 보내는 소켓, 수신정보를 담을 배열주소 , 배열의 크기 , flag)
		printf("Recv Msg : %s\n", cBuffer); //전달받은 데이터 출력
		char cMsg[] = "Server Send : ok!";
		send(hClient, cMsg, strlen(cMsg), 0); //server -> client 메세지를 보내는 함수 ( 연결된 socket , 메세지 , 길이 , flag)
	}
	closesocket(hClient);
	closesocket(hListen);


	WSACleanup();		//다 끝이나고 지워주는,소멸자 느낌 
	system("pause");

}


unsigned short ReverseByteOrder(unsigned short value)
{
	unsigned short ret = 0;
	((char*)&ret)[0] = ((char*)&value)[1];
	((char*)&ret)[1] = ((char*)&value)[0];
	return ret;
	
}

unsigned short CreateCRC16(unsigned char *buff, size_t len) {
	unsigned short crc16 = 0xFFFF;
	int i = 0;
	while (len--) {
		crc16 ^= *buff++;
		for (i = 0; i < 8; i++) {
			if (crc16 & 1) {
				crc16 >>= 1;
				crc16 ^= 0xA001;
			}
			else {
				crc16 >>= 1;

			}
		}
	}
	return crc16;

}