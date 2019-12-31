#ifndef _CLIENT_SOCKET_H
#define _CLIENT_SOCKET_H

extern const char* clientWndClassName;
extern HWND hClientWnd;
extern HWND hClientDispWnd;
extern HWND hClientConnectBtn;
extern HWND hClientDisconnectBtn;
extern HWND hClientStatusWnd;
extern CSocket* clientSocketPtr;

bool ClientSocketSetup(void);
void ClientSocketCleanup(void);
int WINAPI WinMain (HINSTANCE, HINSTANCE, LPSTR, int);
LRESULT CALLBACK ClientWndProc (HWND, UINT, WPARAM, LPARAM);
void PrintStr(const char*);
void PrintNum(double);

void ConnectBtn_OnClick(void);
void DisconnectBtn_OnClick(void);
void Socket_OnConnect(CSocket*, const CSocketEvent&);
void Socket_OnDisconnect(CSocket*, const CSocketEvent&);
void Socket_OnDataArrival(CSocket*, const CSocketEvent&);
void Socket_OnError(CSocket*, const CSocketEvent&);

#endif
