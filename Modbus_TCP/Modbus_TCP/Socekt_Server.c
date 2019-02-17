#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <WinSock2.h>

#pragma comment(lib,"WS2_32.LIB") //Socket library�� �ڵ����� ����

#define PORT 4578	//��Ʈ��ȣ
#define PACKET_SIZE 1024	//��Ŷ������
#define RW_READ 0x03
#define RW_WRITE 0x06

unsigned short ReverseByteOrder(unsigned short value);
unsigned short CreateCRC16(unsigned char *buff, size_t len);

void main() {
	
	unsigned short reg_map[150]; unsigned char recv_buff[8];
	unsigned char *send_buff;
	memset(reg_map, 0x00, sizeof(short) * 150); unsigned short address;
	reg_map[6] = 0xFE;


	WSADATA wsaData;	//�������� ���� �ʱ�ȭ ������ �����ϱ����� ����ü
	WSAStartup(MAKEWORD(2, 2), &wsaData);	//( ���Ϲ���, WSADATA ����ü �ּ�) -> ������� ��� ������ Ȱ���� ������ �˷���
											//MAKEWORD() -> wordŸ������ ����

	/*���� ����*/
	SOCKET hListen;
	hListen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);  // PF_INET : ipv4�� ����Ѵ� 
														  // SOCK_STREAM : ���������� ������ ����ڴ�.
														  // IPPROTO_TCP : protocol�� TCP�� ����ϰڴ�.

	/*���� ����*/
	SOCKADDR_IN tListenAddr;
	tListenAddr.sin_family = AF_INET;	//�ݵ�� AF_INET	
	tListenAddr.sin_port = htons(PORT);	//��Ʈ��ȣ ���� , host to network short -> �򿣵�� ������� �����͸� ��ȯ
	tListenAddr.sin_addr.s_addr = htonl(INADDR_ANY);	

	/*���ε� */
	bind(hListen, (SOCKADDR*)&tListenAddr, sizeof(tListenAddr)); //���Ͽ� ���� ������ �ּ������� �����ְ�  , bind(����, ���� ������� ����ü�� �ּ�, �� ����ü�� ũ��)
	
	/*Listen (client�� ��ٸ�)*/
	listen(hListen, SOMAXCONN);	//hListen������ ���Ӵ����·� ������� 
								//SOMAXCONN�� �Ѳ����� ��û ������ �ִ� ���ӽ��� ���� �ǹ�
	char local_name[1024];
	struct hostent *host_ptr = NULL;
	memset(local_name, 0, 1024);
	gethostname(local_name, 1024);		//gethosname(������ �迭 , ����) : ���� ��ǻ���� local_name��������
	host_ptr = gethostbyname(local_name);	//gethostbyname(�̸�) : �̸����� ��ǻ���� ���� �����ͼ� ����
	printf("Server IP: %u.%u.%u.%u\n",
		(unsigned char)host_ptr->h_addr_list[0][0],
		(unsigned char)host_ptr->h_addr_list[0][1],
		(unsigned char)host_ptr->h_addr_list[0][2],
		(unsigned char)host_ptr->h_addr_list[0][3]);
	printf("Port Number is %d\n", PORT);
	printf("Wait client connection!\n");

	/*-----------------Client �� ���� ���� �� ������ ���� ����ü ���� �� �� �Ҵ�*/
	SOCKADDR_IN tClientAddr;
	int iClientSize = sizeof(tClientAddr);
	
	/*Accept (client�� ����) */
	SOCKET hClient = accept(hListen, (SOCKADDR*)&tClientAddr, &iClientSize);	//���� ��û ���� (����ȭ�� ��� : ��û�� �������ϱ� ������ ������)
																				//(server ���� , client���� , ũ�� )
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
		recv(hClient, (char*)recv_buff, 8, 0);		//recv_buff[0] = id , recv_buff[1] = �б����� ��������?
													//recv_buff[2] = ? , recv_buff[3] = ?
													//recv_buff[4] = ? , recv_buff[5] = ?
													//recv_buff[6] = ? , recv_buff[7] = ?

		if (recv_buff[6] == reg_map[6] || recv_buff[0] == 0xFF) {
			if (recv_buff[1] == RW_READ) {
				memcpy(&address, &recv_buff[2], 2); //�ּҸ� �����ͼ� address�� ����
				address = ReverseByteOrder(address); //�ּҸ� byteordering  
				memcpy(&len, &recv_buff[4], 2);
				len = ReverseByteOrder(len);
				send_buff = (unsigned char*)malloc(3 + len * 2 + 2); //���� ũ�⸸ŭ send_buff �Ҵ�
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

				free(send_buff); //���� �� �Ҵ��� send_buff�޸� �ݳ�
				send_buff = NULL;
			}
		}
	}



	/*������ ����*/
	while (1) {
		char cBuffer[PACKET_SIZE];	//Client�� ������ �����ϱ����� �����صδ� ��Ŷũ�⸸ŭ�� ����
		memset(cBuffer, 0, 1024);
		recv(hClient, cBuffer, PACKET_SIZE, 0);	//recv(data�� ������ ����, ���������� ���� �迭�ּ� , �迭�� ũ�� , flag)
		printf("Recv Msg : %s\n", cBuffer); //���޹��� ������ ���
		char cMsg[] = "Server Send : ok!";
		send(hClient, cMsg, strlen(cMsg), 0); //server -> client �޼����� ������ �Լ� ( ����� socket , �޼��� , ���� , flag)
	}
	closesocket(hClient);
	closesocket(hListen);


	WSACleanup();		//�� ���̳��� �����ִ�,�Ҹ��� ���� 
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