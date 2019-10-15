#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <iostream>



#define SERVERIP "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 4096

void err_quit(const char * msg);
void err_display(const char * msg);

int main(int argc, char * argv[]) {
	int retVal;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { return 1; }

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) { err_quit("socket()"); }

	
	// connect()
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr(SERVERIP);
	serverAddr.sin_port = ntohs(SERVERPORT);

	retVal = connect(sock, (SOCKADDR *)&serverAddr, sizeof(serverAddr));
	if (retVal == SOCKET_ERROR) { err_quit("connect()"); }



	// 파일 이름 입력받기
	// 256 글자 이상 파일이 입력이 안되더라...
	// 경로 포함해서 256글자 이상 넘어가면 안되더라...
	char fileName[256];
	ZeroMemory(fileName, 256);
	std::cout << "확장자를 포함한 파일 이름을 입력해주세요. > ";
	std::cin >> fileName;

	// 파일 읽어오기
	FILE *fp = fopen(fileName, "rb");

	if (fp == NULL) {
		std::cout << "해당 파일이 존재하지 않습니다!" << std::endl;
		err_quit("send()");
	}

	// 파일 이름 
	int fileNameLen = strlen(fileName);

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

	//std::cout << "[TCP 클라이언트] " << sizeof(int) << " + " << retVal << "(파일 이름의 크기)" << "바이트를 보냈습니다. \n";


	// 파일 크기얻기
	// 파일 포인터를 뒤로 보내서 크기를 재고
	unsigned int fileSize = 0;
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);

	// 파일 전송을 위해 파일 포인터를 맨 앞으로 이동
	fseek(fp, 0, SEEK_SET);
	char buf[BUFSIZE];

	// 데이터 크기 가르쳐 주기 (고정 길이)
	retVal = send(sock, (char*)&fileSize, sizeof(int), 0);
	if (retVal == SOCKET_ERROR) {
		err_display("send()");
	}

	//std::cout << "[TCP 클라이언트] " << "파일 크기 " << fileSize << "(바이트)를 담은 " << sizeof(int) << "바이트를 보냈습니다. \n";

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
		retVal = send(sock, (char*)&sendFileSize, sizeof(int), 0);
		if (retVal == SOCKET_ERROR) {
			err_display("send()");
		}

		// 가변 길이
		readFileSize = fread(buf, 1, sendFileSize, fp);
		if (readFileSize > 0) {
			retVal = send(sock, buf, readFileSize, 0);
			if (retVal == SOCKET_ERROR) {
				err_display("send()");
			}
			uploadFileSize += readFileSize;
			if (leftFileSize >= BUFSIZE) {
				leftFileSize -= readFileSize;
			}

		}
		else if (readFileSize == 0 && uploadFileSize == fileSize) {
			std::cout << " 파일 전송 성공!" << std::endl;
			break;
		}
		else {
			std::cout << " 파일 입출력 오류!" << std::endl;
		}

		// 진행바 출력
		per = ((double)uploadFileSize / (double)fileSize) * 100;	// 현재 작업 진행 상황 ( % )
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

	std::cout << "[TCP 클라이언트] 파일 업로드 완료! 보낸 파일 크기 : " << uploadFileSize << "바이트" << std::endl;

	fclose(fp);
	closesocket(sock);

	WSACleanup();

	Sleep(50000);


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