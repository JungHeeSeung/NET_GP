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
	
	// 데이터 통신에 사용할 변수
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

		// 접속한 클라이언트 정보 출력
		cout << "\n[TCP 서버] 클라이언트 접속 : IP 주소 = " << inet_ntoa(clientAddr.sin_addr)
			<< " 포트 번호 = " << ntohs(clientAddr.sin_port) << endl;


		// 데이터 받기(고정 길이)
		// 파일 이름 크기
		retVal = recvn(client_sock, (char *)&fileNameLen, sizeof(int), 0);
		if (retVal == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retVal == 0) { break; }


		// 데이터 받기(가변 길이)
		// 파일 이름 받기
		retVal = recvn(client_sock, buf, fileNameLen, 0);
		if (retVal == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retVal == 0) { break; }

		// 받은 데이터 출력
		buf[retVal] = '\0';
		cout << "[TCP/" << inet_ntoa(clientAddr.sin_addr)
			<< ":" << ntohs(clientAddr.sin_port) << "] " << buf << " 파일 전송" << endl;

		strcpy(fileName, buf);

		// 파일 읽어오기
		FILE *fp = fopen(fileName, "rb");

		if (fp == NULL) {
			cout << "해당 파일이 존재하지 않습니다!" << endl;
			err_quit("send()");
		}

		// 파일 크기얻기
		// 파일 포인터를 뒤로 보내서 크기를 재고
		unsigned int fileSize = 0;
		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp);

		// 파일 전송을 위해 파일 포인터를 맨 앞으로 이동
		fseek(fp, 0, SEEK_SET);
		char buf[BUFSIZE];

		// 데이터 크기 가르쳐 주기 (고정 길이)
		retVal = send(client_sock, (char*)&fileSize, sizeof(int), 0);
		if (retVal == SOCKET_ERROR) {
			err_display("send()");
		}

		// 데이터 보내기 (가변 길이)
		// 파일의 크기 < BUFSIZE -> 바로 전송
		// 파일의 크기 > BUFSIZE -> 나눠서 전송

		unsigned int sendFileSize = 0;
		unsigned int uploadFileSize = 0;
		unsigned int readFileSize = 0;
		unsigned int leftFileSize = 0;
		leftFileSize = fileSize;


		int total_len = 20;	// 진행바 길이
		int cur_len;	// 진행바 현재 길이
		float per;		// 현재 작업 진행상황

		// 진짜 보내는 부분
		while (1) {
			ZeroMemory(buf, BUFSIZE);

			// 보내야 할 파일 사이즈가 버퍼의 크기보다 크다면 잘라주기
			if (leftFileSize >= BUFSIZE) {
				sendFileSize = BUFSIZE - 1;
			}
			else {
				sendFileSize = leftFileSize;
			}

			// 고정 길이
			retVal = send(client_sock, (char*)&sendFileSize, sizeof(int), 0);
			if (retVal == SOCKET_ERROR) {
				err_display("send()");
			}

			// 가변 길이
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
				cout << " 파일 전송 성공!" << endl;
				break;
			}
			else {
				cout << " 파일 입출력 오류!" << endl;
			}

			// 진행바 출력
			per = ((double)uploadFileSize / (double)fileSize) * 100;	// 현재 작업 진행 상황 ( % )
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
		downloadSize = 0;

		// closesocket()
		closesocket(client_sock);
		cout << "[TCP 서버] 클라이언트 종료 : IP 주소 = " << inet_ntoa(clientAddr.sin_addr)
			<< " 포트 번호 = " << ntohs(clientAddr.sin_port) << endl;
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