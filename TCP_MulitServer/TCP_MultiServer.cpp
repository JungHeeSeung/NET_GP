#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <iostream>

using namespace std;


#define SERVERPORT 9000
#define BUFSIZE 4096

void err_quit(const char * msg);
void err_display(const char * msg);
int recvn(SOCKET s, char * buf, int len, int flags);

int main(int argc, char * argv[]) {
	int retVal;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { return 1; }


	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) { err_quit("socket()"); }


	// connect()
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = ntohs(SERVERPORT);

	retVal = bind(listen_sock, (SOCKADDR *)&serverAddr, sizeof(serverAddr));
	if (retVal == SOCKET_ERROR) { err_quit("bind()"); }


	// listen()
	retVal = listen(listen_sock, SOMAXCONN);
	if (retVal == SOCKET_ERROR) { err_quit("listen()"); }
	
	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	SOCKADDR_IN clientAddr;
	int addrLen;
	char buf[BUFSIZE];
	unsigned int fileSize = 0;
	unsigned int fileNameLen = 0;
	unsigned int downloadSize = 0;

	char fileName[256];
	ZeroMemory(fileName, 256);

	while (1) {
		// accept()
		addrLen = sizeof(clientAddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientAddr, &addrLen);

		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// ������ Ŭ���̾�Ʈ ���� ���
		cout << "\n[TCP ����] Ŭ���̾�Ʈ ���� : IP �ּ� = " << inet_ntoa(clientAddr.sin_addr)
			<< " ��Ʈ ��ȣ = " << ntohs(clientAddr.sin_port) << endl;


		// ������ �ޱ�(���� ����)
		// ���� �̸� ũ��
		retVal = recvn(client_sock, (char *)&fileNameLen, sizeof(int), 0);
		if (retVal == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retVal == 0) { break; }


		// ������ �ޱ�(���� ����)
		// ���� �̸� �ޱ�
		retVal = recvn(client_sock, buf, fileNameLen, 0);
		if (retVal == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retVal == 0) { break; }

		// ���� ������ ���
		buf[retVal] = '\0';
		cout << "[TCP/" << inet_ntoa(clientAddr.sin_addr)
			<< ":" << ntohs(clientAddr.sin_port) << "] " << buf << " ���� ����" << endl;

		strcpy(fileName, buf);

		// ���� �о����
		FILE *fp = fopen(fileName, "rb");

		if (fp == NULL) {
			cout << "�ش� ������ �������� �ʽ��ϴ�!" << endl;
			err_quit("send()");
		}

		// ���� ũ����
		// ���� �����͸� �ڷ� ������ ũ�⸦ ���
		unsigned int fileSize = 0;
		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp);

		// ���� ������ ���� ���� �����͸� �� ������ �̵�
		fseek(fp, 0, SEEK_SET);
		char buf[BUFSIZE];

		// ������ ũ�� ������ �ֱ� (���� ����)
		retVal = send(client_sock, (char*)&fileSize, sizeof(int), 0);
		if (retVal == SOCKET_ERROR) {
			err_display("send()");
		}

		// ������ ������ (���� ����)
		// ������ ũ�� < BUFSIZE -> �ٷ� ����
		// ������ ũ�� > BUFSIZE -> ������ ����

		unsigned int sendFileSize = 0;
		unsigned int uploadFileSize = 0;
		unsigned int readFileSize = 0;
		unsigned int leftFileSize = 0;
		leftFileSize = fileSize;


		int total_len = 20;	// ����� ����
		int cur_len;	// ����� ���� ����
		float per;		// ���� �۾� �����Ȳ

		// ��¥ ������ �κ�
		while (1) {
			ZeroMemory(buf, BUFSIZE);

			// ������ �� ���� ����� ������ ũ�⺸�� ũ�ٸ� �߶��ֱ�
			if (leftFileSize >= BUFSIZE) {
				sendFileSize = BUFSIZE - 1;
			}
			else {
				sendFileSize = leftFileSize;
			}

			// ���� ����
			retVal = send(client_sock, (char*)&sendFileSize, sizeof(int), 0);
			if (retVal == SOCKET_ERROR) {
				err_display("send()");
			}

			// ���� ����
			readFileSize = fread(buf, 1, sendFileSize, fp);
			if (readFileSize > 0) {
				retVal = send(client_sock, buf, readFileSize, 0);
				if (retVal == SOCKET_ERROR) {
					err_display("send()");
				}
				uploadFileSize += readFileSize;
				if (leftFileSize >= BUFSIZE) {
					leftFileSize -= readFileSize;
				}

			}
			else if (readFileSize == 0 && uploadFileSize == fileSize) {
				cout << " ���� ���� ����!" << endl;
				break;
			}
			else {
				cout << " ���� ����� ����!" << endl;
			}

			// ����� ���
			per = ((double)uploadFileSize / (double)fileSize) * 100;	// ���� �۾� ���� ��Ȳ ( % )
			cur_len = per / 5;	// 5 % ���� �� ����
			cout << "\r" << "[";
			for (int i = 0; i < total_len; ++i)
			{
				if (i < cur_len)
				{
					cout << "��";
				}
				else
				{
					cout << "  ";
				}
			}
			cout << "]" << per << "%";
		}

		fclose(fp);
		downloadSize = 0;

		// closesocket()
		closesocket(client_sock);
		cout << "[TCP ����] Ŭ���̾�Ʈ ���� : IP �ּ� = " << inet_ntoa(clientAddr.sin_addr)
			<< " ��Ʈ ��ȣ = " << ntohs(clientAddr.sin_port) << endl;
	}
	
	// closesocket()
	closesocket(listen_sock);

	WSACleanup();
	return 0;
}


void err_quit(const char * msg) {
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

void err_display(const char * msg) {
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

	cout << "[" << msg << "] " << (char*)lpMsgBuf << endl;
	LocalFree(lpMsgBuf);
}

int recvn(SOCKET s, char * buf, int len, int flags) {
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);

		if (received == SOCKET_ERROR) { return SOCKET_ERROR; }
		else if (received == 0) { break; }

		left -= received;
		ptr += received;
	}

	return (len - left);
}