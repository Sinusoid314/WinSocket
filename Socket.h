//WinSock Wrapper v1.01
//Written by Andrew Sturges
//April 2015

#ifndef _SOCKET_H
#define _SOCKET_H

#include <string>
#include <vector>
#include <windows.h>
#include <winsock2.h>

//Message for socket events
#define WM_SOCKET WM_USER + 1

//To allow error events to be treated like the other socket events
#define FD_ERROR (1 << FD_MAX_EVENTS)

//Socket states
#define SOCKET_STATE_DISCONNECTED 1
#define SOCKET_STATE_CONNECTED 2
#define SOCKET_STATE_LISTENING 3
#define SOCKET_STATE_CONNECTING 4

extern const char* socketWndClassName;
extern bool inSocketEvent;

class CSocketEvent;
class CSocket;

bool SocketSetup(void);
bool SocketCleanup(void);
LRESULT CALLBACK SocketEventProc(HWND, UINT, WPARAM, LPARAM);
std::string GetSocketErrorStrFromCode(int);

class CSocketEvent
{
  public:
    
    UINT eventID;
    void (*eventFuncPtr)(CSocket*, const CSocketEvent&);
    LONG userData;
    WPARAM wParam;
    CSocketEvent(void);
    CSocketEvent(UINT, void (*newEventFuncPtr)(CSocket*, const CSocketEvent&), LONG);
};

class CSocket
{
  public:
    
	//Main socket info
	SOCKET hSocket;
	HWND hSocketWnd;
	int socketState;
	std::string socketName;
    std::string socketErrorStr;
    std::vector<CSocketEvent> socketEventList;
	
	//Socket constructors & destructor
	CSocket(void);
	CSocket(SOCKET, int);
	~CSocket(void);
    
	//Socket methods
	bool Setup(SOCKET, int);
	bool Cleanup(void);
	void AddEvent(UINT, void (*newEventFuncPtr)(CSocket*, const CSocketEvent&), LONG newUserData = 0);
	bool RemoveEvent(UINT);
	int GetEventIndex(UINT);
	bool Listen(int);
	bool Connect(const char*, int);
	bool Accept(void);
	CSocket* AcceptSpawn(void);
	bool Disconnect(void);
	bool GetData(std::string&);
	bool SendData(std::string&);
	
	//Socket property retrieval functions
	const char* GetStateStr(void);
	std::string GetRemoteHostName(void);
	std::string GetRemoteHostIP(void);
	int GetRemoteHostPort(void);
	std::string GetLocalHostName(void);
	std::string GetLocalHostIP(void);
	int GetLocalHostPort(void);
};
	
#endif
