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
	cout << "IP를 입력해주세요: ";
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

	int total_len = 20;	// 진행바 길이
	int cur_len;	// 진행바 현재 길이
	float per;		// 현재 작업 진행상황

	// 파일 이름 입력받기
	// 256 글자 이상 파일이 입력이 안되더라...
	// 경로 포함해서 256글자 이상 넘어가면 안되더라...
	char fileName[256];
	ZeroMemory(fileName, 256);
	cout << "확장자를 포함한 파일 이름을 입력해주세요.: ";
	cin >> fileName;

	// 파일 이름 
	fileNameLen = strlen(fileName);

	// 데이터 보내기 (고정 길이)
	// 파일 이름을 int(고정)으로 길이를 가르쳐주기
	retVal = send(sock, (char*)&fileNameLen, sizeof(int), 0);
	if (retVal == SOCKET_ERROR) {
		err_display("send()");
	}

	// 데이터 보내기 (가변 길이)
	// 파일 이름 길이(가변)만큼 이름을 보내기
	retVal = send(sock, fileName, fileNameLen, 0);
	if (retVal == SOCKET_ERROR) {
		err_display("send()");
	}


	FILE *fp = fopen(fileName, "wb");
	if (fp == NULL) {
		cout << "파일 생성 실패" << endl;
		exit(1);
	}

	// 파일 저장하기
	// 클라이언트와 데이터 통신
	// 데이터 받기(고정 길이)
	retVal = recvn(sock, (char *)&fileSize, sizeof(int), 0);
	if (retVal == SOCKET_ERROR) {
		err_display("recv()");
	}

	unsigned int recvFileSize = 0;

	// 데이터 받기(가변 길이)
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


		// 진행바 출력
		per = ((double)downloadSize / (double)fileSize) * 100;	// 현재 작업 진행 상황 ( % )
		cur_len = per / 5;	// 5 % 마다 한 개씩
		cout << "\r" << "[";
		for (int i = 0; i < total_len; ++i)
		{
			if (i < cur_len)
			{
				cout << "■";
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