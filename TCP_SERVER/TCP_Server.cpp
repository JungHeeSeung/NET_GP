#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <iostream>

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
	char buf[BUFSIZE + 1];
	unsigned int fileSize = 0;
	unsigned int fileNameLen = 0;
	unsigned int downloadSize = 0;


	int total_len = 20;	// 진행바 길이
	int cur_len;	// 진행바 현재 길이
	float per;		// 현재 작업 진행상황


	while (1) {
		// accept()
		addrLen = sizeof(clientAddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientAddr, &addrLen);

		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}


		// 접속한 클라이언트 정보 출력
		std::cout << "\n[TCP 서버] 클라이언트 접속 : IP 주소 = " << inet_ntoa(clientAddr.sin_addr)
			<< " 포트 번호 = " << ntohs(clientAddr.sin_port) << std::endl;

	
		char fileName[256];
		ZeroMemory(fileName, 256);

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
		std::cout << "[TCP/" << inet_ntoa(clientAddr.sin_addr)
			<< ":" << ntohs(clientAddr.sin_port) << "] " << buf << " 파일 전송" << std::endl;

		strcpy(fileName, buf);

		
		FILE *fp = fopen(fileName, "wb");
		if (fp == NULL) {
			std::cout << "파일 생성 실패" << std::endl;
			exit(1);
		}

		// 파일 저장하기
		// 클라이언트와 데이터 통신
		// 데이터 받기(고정 길이)
		retVal = recvn(client_sock, (char *)&fileSize, sizeof(int), 0);
		if (retVal == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retVal == 0) { break; }
		std::cout << " 파일크기: " << fileSize << std::endl;

		unsigned int recvFileSize = 0;

		// 데이터 받기(가변 길이)
		while (1) {
			ZeroMemory(buf, BUFSIZE);

			retVal = recvn(client_sock, (char *)&recvFileSize, sizeof(int), 0);
			if (retVal == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retVal == 0) { break; }

			retVal = recvn(client_sock, buf, recvFileSize, 0);
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
			std::cout << "\r" << "[";
			for (int i = 0; i < total_len; ++i)
			{
				if (i < cur_len)
				{
					std::cout << "■";
				}
				else
				{
					std::cout << "  ";
				}
			}
			std::cout << "]" << per << "%";
		}


		std::cout << "[TCP/" << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << "] "
			<< "파일 다운로드 완료! 받은 파일 크기 : " << ftell(fp) - 4 << std::endl;
		fclose(fp);

		downloadSize = 0;

		// closesocket()
		closesocket(client_sock);
		std::cout << "[TCP 서버] 클라이언트 종료 : IP 주소 = " << inet_ntoa(clientAddr.sin_addr)
			<< " 포트 번호 = " << ntohs(clientAddr.sin_port) << std::endl;
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

	std::cout << "[" << msg << "] " << (char*)lpMsgBuf << std::endl;
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