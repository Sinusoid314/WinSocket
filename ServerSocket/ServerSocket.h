#ifndef _SERVER_SOCKET_H
#define _SERVER_SOCKET_H

const char* serverWndClassName = "ServerSocketWindow";
HWND hServerWnd;
HWND hServerDispWnd;
CSocket* serverSocketPtr = NULL;

bool ServerSocketSetup(void);
void ServerSocketCleanup(void);
int WINAPI WinMain (HINSTANCE, HINSTANCE, LPSTR, int);
LRESULT CALLBACK ServerWndProc (HWND, UINT, WPARAM, LPARAM);
void PrintStr(const char*);
void PrintNum(double);

void Socket_OnConnectionRequest(CSocket*, const CSocketEvent&);
void Socket_OnDisconnect(CSocket*, const CSocketEvent&);
void Socket_OnDataArrival(CSocket*, const CSocketEvent&);
void Socket_OnError(CSocket*, const CSocketEvent&);

#endif
