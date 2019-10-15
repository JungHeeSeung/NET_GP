#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <iostream>
#include <string>

using namespace std;

#define SERVERIP "127.0.0.1"
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
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) { err_quit("socket()"); }

	string serverIP;
	cout << "IP�� �Է����ּ���: ";
	cin >> serverIP;

	// connect()
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr(serverIP.c_str());
	serverAddr.sin_port = ntohs(SERVERPORT);

	retVal = connect(sock, (SOCKADDR *)&serverAddr, sizeof(serverAddr));
	if (retVal == SOCKET_ERROR) { err_quit("connect()"); }

	int addrLen;
	char buf[BUFSIZE];
	unsigned int fileSize = 0;
	unsigned int fileNameLen = 0;
	unsigned int downloadSize = 0;

	int total_len = 20;	// ����� ����
	int cur_len;	// ����� ���� ����
	float per;		// ���� �۾� �����Ȳ

	// ���� �̸� �Է¹ޱ�
	// 256 ���� �̻� ������ �Է��� �ȵǴ���...
	// ��� �����ؼ� 256���� �̻� �Ѿ�� �ȵǴ���...
	char fileName[256];
	ZeroMemory(fileName, 256);
	cout << "Ȯ���ڸ� ������ ���� �̸��� �Է����ּ���.: ";
	cin >> fileName;

	// ���� �̸� 
	fileNameLen = strlen(fileName);

	// ������ ������ (���� ����)
	// ���� �̸��� int(����)���� ���̸� �������ֱ�
	retVal = send(sock, (char*)&fileNameLen, sizeof(int), 0);
	if (retVal == SOCKET_ERROR) {
		err_display("send()");
	}

	// ������ ������ (���� ����)
	// ���� �̸� ����(����)��ŭ �̸��� ������
	retVal = send(sock, fileName, fileNameLen, 0);
	if (retVal == SOCKET_ERROR) {
		err_display("send()");
	}


	FILE *fp = fopen(fileName, "wb");
	if (fp == NULL) {
		cout << "���� ���� ����" << endl;
		exit(1);
	}

	// ���� �����ϱ�
	// Ŭ���̾�Ʈ�� ������ ���
	// ������ �ޱ�(���� ����)
	retVal = recvn(sock, (char *)&fileSize, sizeof(int), 0);
	if (retVal == SOCKET_ERROR) {
		err_display("recv()");
	}

	unsigned int recvFileSize = 0;

	// ������ �ޱ�(���� ����)
	while (1) {
		ZeroMemory(buf, BUFSIZE);

		retVal = recvn(sock, (char *)&recvFileSize, sizeof(int), 0);
		if (retVal == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retVal == 0) { break; }

		retVal = recvn(sock, buf, recvFileSize, 0);
		if (retVal == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (recvFileSize == 0) { break; }

		fwrite(buf, 1, retVal, fp);
		downloadSize += retVal;
		if (downloadSize > fileSize) {
			downloadSize -= 4;
		}


		// ����� ���
		per = ((double)downloadSize / (double)fileSize) * 100;	// ���� �۾� ���� ��Ȳ ( % )
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

	closesocket(sock);


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