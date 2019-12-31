//For getnameinfo() to work
#ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0501
#endif

#include "Socket.h"
#include <ws2tcpip.h>

using namespace std;


const char* socketWndClassName = "SocketWindow";
bool inSocketEvent;

bool SocketSetup(void)
//Set up socket-support resources
{
    WSADATA winSockInfo;
    WNDCLASSEX wndClassInfo;
    
    //Initialize global socket variables
    inSocketEvent = false;
    
    //Initialize Winsock
    if(WSAStartup(MAKEWORD(2,0), &winSockInfo))
    {
        return false;
    }
    
    //Register socket window class
    wndClassInfo.hInstance = (HINSTANCE) GetModuleHandle(NULL);
    wndClassInfo.lpszClassName = socketWndClassName;
    wndClassInfo.lpfnWndProc = SocketEventProc;
    wndClassInfo.style = CS_DBLCLKS;
    wndClassInfo.cbSize = sizeof (WNDCLASSEX);
    wndClassInfo.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wndClassInfo.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wndClassInfo.hCursor = LoadCursor (NULL, IDC_ARROW);
    wndClassInfo.lpszMenuName = NULL;
    wndClassInfo.cbClsExtra = 0;
    wndClassInfo.cbWndExtra = 0;
    wndClassInfo.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
    if (!RegisterClassEx (&wndClassInfo))
    {
        return false;
    }
    
    return true;
}

bool SocketCleanup(void)
//Clean up socket-support resources
{
    //Unregister socket window class
    UnregisterClass(socketWndClassName, (HINSTANCE) GetModuleHandle(NULL));
    
    //Clean up Winsock
    WSACleanup();
    
    return true;
}

LRESULT CALLBACK SocketEventProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
//Process socket events
{
    CSocket* sockObjPtr;
    UINT sockEventID;
    int sockEventIndex;
    
    switch(message)
    {
        case WM_SOCKET:
            
            //Check for event reentrancy
            if(inSocketEvent)
            {
                //Post the socket event to the message queue, to make sure
                //it's only handled after the current event
                PostMessage(hwnd, message, wParam, lParam);
                
                return true;
            }
            
            //Socket event is being executed
            inSocketEvent = true;
            
            //Retrieve CSocket pointer from hwnd's user info
            sockObjPtr = (CSocket*) GetWindowLong(hwnd, GWL_USERDATA);
            
            if (WSAGETSELECTERROR(lParam))
            {
                sockEventID = FD_ERROR;
                
                //Close socket connection
                sockObjPtr->Disconnect();
                
                //Set socket error description
                sockObjPtr->socketErrorStr = GetSocketErrorStrFromCode(WSAGETSELECTERROR(lParam));
            }
            else
            {
                sockEventID = WSAGETSELECTEVENT(lParam);
                
                switch(sockEventID)
                {
                    case FD_CONNECT:
                        //Set socket's state to Connected
                        sockObjPtr->socketState = SOCKET_STATE_CONNECTED;
                        break;
                        
                    case FD_CLOSE:
                        //Close socket connection
                        sockObjPtr->Disconnect();
                        break;
                }
            }
            
            //Get the index of the event object tied to message
            sockEventIndex = sockObjPtr->GetEventIndex(sockEventID);
            
            if(sockEventIndex > -1)
            {
	            //Call the matching event function
                sockObjPtr->socketEventList[sockEventIndex].wParam = wParam;
                (*((sockObjPtr->socketEventList[sockEventIndex]).eventFuncPtr))(sockObjPtr, sockObjPtr->socketEventList[sockEventIndex]);
            }
            
            //Socket event has been executed
            inSocketEvent = false;
            
            return true;
        
        default:
            return DefWindowProc (hwnd, message, wParam, lParam);
    }
}

string GetSocketErrorStrFromCode(int errorCode)
//Get the description string for an error code
{
    LPTSTR tmpStr = NULL;
    string errorStr;

    if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &tmpStr, 0, NULL))
    {
	    errorStr = tmpStr;
        LocalFree(tmpStr);
    }
    
    return errorStr;
}

CSocketEvent::CSocketEvent(void)
//Initialize members
{
    eventID = 0;
    eventFuncPtr = NULL;
    userData = 0;
    wParam = 0;
}

CSocketEvent::CSocketEvent(UINT newEventID, void (*newEventFuncPtr)(CSocket*, const CSocketEvent&), LONG newUserData)
//Initialize members to the given arguments
{
    eventID = newEventID;
    eventFuncPtr = newEventFuncPtr;
    userData = newUserData;
    wParam = 0;
}

CSocket::CSocket(void)
//Initialize the object to a new, disconnected socket
{
    Setup(0, SOCKET_STATE_DISCONNECTED);
}

CSocket::CSocket(SOCKET hNewSocket, int initState)
//Initialize the object to an existing socket handle
{
    Setup(hNewSocket, initState);
}

CSocket::~CSocket()
//
{
    Cleanup();
}

bool CSocket::Setup(SOCKET hNewSocket, int initState)
//Initialize the socket object's resources
{
    hSocket = 0;
    hSocketWnd = 0;
    
    //Create socket's window
    hSocketWnd = CreateWindowEx(0, socketWndClassName, "", WS_OVERLAPPEDWINDOW,
                              0, 0, 0, 0, HWND_DESKTOP, NULL,
                              (HINSTANCE) GetModuleHandle(NULL), NULL);
    
    //Save current CSocket pointer to hSocketWnd's user info
    SetWindowLong(hSocketWnd, GWL_USERDATA, (LONG) this);
    
    //Create socket
    if(hNewSocket == 0)
    {
        hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(hSocket == INVALID_SOCKET)
        {
            socketErrorStr = "Socket setup failed: " + GetSocketErrorStrFromCode(WSAGetLastError());
            return false;
        }
    }
    else
    {
        hSocket = hNewSocket;
    }
    
    //Have all events for hSocket sent to hSocketWnd
    if(WSAAsyncSelect(hSocket, hSocketWnd, WM_SOCKET, FD_CONNECT|FD_CLOSE|FD_ACCEPT|FD_READ) == SOCKET_ERROR)
    {
        socketErrorStr = "Socket setup failed: " + GetSocketErrorStrFromCode(WSAGetLastError());
        return false;
    }
    
    //Set socket's initial state
    socketState = initState;
    
    return true;
}

bool CSocket::Cleanup(void)
//Clean up the socket object's resources
{
    //Close the socket's window
    DestroyWindow(hSocketWnd);
    
    //Close the socket
    closesocket(hSocket);
    
    socketState = SOCKET_STATE_DISCONNECTED;
    
    return true;
}

void CSocket::AddEvent(UINT newEventID, void (*newEventFuncPtr)(CSocket*, const CSocketEvent&), LONG newUserData)
//Add a new socket event with the given message/function-pointer pair
{
    int eventIdx;
    
    //Get the index of the event object (if present)
    eventIdx = GetEventIndex(newEventID);
    
    if(eventIdx == -1)
    {
        //Add new event object to winEventList
        socketEventList.push_back(CSocketEvent(newEventID, newEventFuncPtr, newUserData));
    }
    else
    {
        //Update the existing event with the new function pointer and user data
        socketEventList[eventIdx].eventFuncPtr = newEventFuncPtr;
        socketEventList[eventIdx].userData = newUserData;
    }
}

bool CSocket::RemoveEvent(UINT eventID)
//Remove the event object identified by the given socket message
{
    int eventIdx;
    
    //Get the index of the event object (if present)
    eventIdx = GetEventIndex(eventID);
    
    if(eventIdx == -1)
    {
	    //Event not found
	    return false;
    }
    else
    {
	    //Remove event
        socketEventList.erase(socketEventList.begin() + eventIdx);
        return true;
    }
}

int CSocket::GetEventIndex(UINT eventID)
//Return the index of the event that matches the given socket message
{
    for(int n=0; n < socketEventList.size(); n++)
    {
	    if(socketEventList[n].eventID == eventID)
        {
	        return n;
        }
    }
    
    return -1;
}

bool CSocket::Listen(int portNum)
//Listen for a connection request
{    
    sockaddr_in listenInfo;
    
    //Socket must be DISCONNECTED
    if(socketState != SOCKET_STATE_DISCONNECTED)
    {
	    socketErrorStr = "Socket must be disconnected before going into listening mode.";
        return false;
    }
    
    //Set listen information
    memset(&listenInfo, 0, sizeof(listenInfo));
    listenInfo.sin_family = AF_INET;
    listenInfo.sin_port = htons(portNum);
    listenInfo.sin_addr.s_addr = INADDR_ANY;
    
    //Bind socket to the listen information
    if( bind(hSocket, (sockaddr *)&listenInfo, sizeof(sockaddr_in)) == SOCKET_ERROR )
    {
	    socketErrorStr = "Listen() failed: " + GetSocketErrorStrFromCode(WSAGetLastError());
        return false;
    }
  
    //Set socket to listen on portNum
    if( listen(hSocket, 10) ==  SOCKET_ERROR )
    {
	    socketErrorStr = "Listen() failed: " + GetSocketErrorStrFromCode(WSAGetLastError());
        return false;
    }
    
    socketState = SOCKET_STATE_LISTENING;
    
    return true;
}

bool CSocket::Connect(const char* hostName, int portNum)
//Request a connection from a remote socket
{
    hostent* hostInfo;
    sockaddr_in connectInfo;
    
    //Socket must be DISCONNECTED
    if(socketState != SOCKET_STATE_DISCONNECTED)
    {
	    socketErrorStr = "Socket must be disconnected before connecting.";
        return false;
    }
    
    //Get host information from host name
    hostInfo = gethostbyname(hostName);
    if(hostInfo == NULL)
    {
        socketErrorStr = "Connect() failed: " + GetSocketErrorStrFromCode(WSAGetLastError());
        return false;
    }
    
    //Set connection information
    connectInfo.sin_family = AF_INET;
    connectInfo.sin_port = htons(portNum);
    connectInfo.sin_addr = *((in_addr *)hostInfo->h_addr);
    memset(&(connectInfo.sin_zero), 0, 8); 

    //Connect hSocket to hostName on portNum
    if( connect(hSocket, (sockaddr *) &connectInfo, sizeof(sockaddr)) )
    {
        if(WSAGetLastError() != WSAEWOULDBLOCK)
        {
            socketErrorStr = "Connect() failed: " + GetSocketErrorStrFromCode(WSAGetLastError());
            return false;
        }
    }
    
    socketState = SOCKET_STATE_CONNECTING;
    
    return true;
}

bool CSocket::Accept(void)
//Accept a connection request on the current socket object
{
    SOCKET hNewSocket;
    
    //Socket must be LISTENING
    if(socketState != SOCKET_STATE_LISTENING)
    {
	    socketErrorStr = "Socket must be listening before accepting a connection.";
        return false;
    }
    
    //Accept new connection
    hNewSocket = accept(hSocket, 0, 0);
    if(hNewSocket == INVALID_SOCKET)
    {
	    socketErrorStr = "Accept() failed: " + GetSocketErrorStrFromCode(WSAGetLastError());
        return false;
    }
    
    //Close current socket
    closesocket(hSocket);
    
    //Set hSocket to the new socket handle
    hSocket = hNewSocket;
    
    //Have all events for hSocket sent to hSocketWnd
    if(WSAAsyncSelect(hSocket, hSocketWnd, WM_SOCKET, FD_CONNECT|FD_CLOSE|FD_ACCEPT|FD_READ) == SOCKET_ERROR)
    {
        socketErrorStr = "Accept() failed: " + GetSocketErrorStrFromCode(WSAGetLastError());
        return false;
    }
    
    socketState = SOCKET_STATE_CONNECTED;
    
    return true;
}

CSocket* CSocket::AcceptSpawn(void)
//Accept a connection request on a new socket object
{
    SOCKET hNewSocket;
    
    //Socket must be LISTENING
    if(socketState != SOCKET_STATE_LISTENING)
    {
	    socketErrorStr = "Socket must be listening before accpeting a connection.";
        return NULL;
    }
    
    //Accept new connection
    hNewSocket = accept(hSocket, 0, 0);
    if(hNewSocket == INVALID_SOCKET)
    {
	    socketErrorStr = "AcceptSpawn() failed: " + GetSocketErrorStrFromCode(WSAGetLastError());
        return NULL;
    }
    
    //Create and return a new CSocket object
    return new CSocket(hNewSocket, SOCKET_STATE_CONNECTED);
}

bool CSocket::Disconnect(void)
//End the current connection
{
    //Socket must NOT be DISCONNECTED
    if(socketState == SOCKET_STATE_DISCONNECTED)
    {
	    socketErrorStr = "Socket is already disconnected.";
        return false;
    }
    
    //Close current socket
    closesocket(hSocket);
    
    //Create new socket
    hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(hSocket == INVALID_SOCKET)
    {
        socketErrorStr = "Disconnect() failed: " + GetSocketErrorStrFromCode(WSAGetLastError());
        return false;
    }
    
    //Have all events for hSocket sent to hSocketWnd
    if(WSAAsyncSelect(hSocket, hSocketWnd, WM_SOCKET, FD_CONNECT|FD_CLOSE|FD_ACCEPT|FD_READ) == SOCKET_ERROR)
    {
        socketErrorStr = "Disconnect() failed: " + GetSocketErrorStrFromCode(WSAGetLastError());
        return false;
    }
    
    socketState = SOCKET_STATE_DISCONNECTED;
    
    return true;
}
  
bool CSocket::GetData(string& dataStr)
//Read in data sent from the remote socket
{
    char* dataChunk;
    u_long bytesRead = 0;
    
    //Socket must be CONNECTED
    if(socketState != SOCKET_STATE_CONNECTED)
    {
	    socketErrorStr = "Socket must be connected before receiving data.";
        return false;
    }

    //Figure out how many bytes of data can be read in from the socket
    if(ioctlsocket(hSocket, FIONREAD, &bytesRead) == SOCKET_ERROR)
    {
	    socketErrorStr = "GetData() failed: " + GetSocketErrorStrFromCode(WSAGetLastError());
        return false;
    }
    if(bytesRead <= 0) return true;  //No data to read, but not really an error
    
    //Create temporary buffer for incoming data
    dataChunk = new char[bytesRead];
    
    //Read in data from the socket
    bytesRead = recv(hSocket, dataChunk, bytesRead, 0);
    if (bytesRead > 0)
    {
        //Append data chunk to the main data string
        dataStr.append(dataChunk, bytesRead);
    }
    
    //Delete buffer
    delete [] dataChunk;
    
    return true;
}

bool CSocket::SendData(string& dataStr)
//Send data to the remote socket
{
    //Socket must be CONNECTED
    if(socketState != SOCKET_STATE_CONNECTED)
    {
	    socketErrorStr = "Socket must be connected before sending data.";
        return false;
    }
    
    //Send data through the socket
    if(send(hSocket, dataStr.c_str(), dataStr.length(), 0) == SOCKET_ERROR)
    {
	    socketErrorStr = "SendData() failed: " + GetSocketErrorStrFromCode(WSAGetLastError());
        return false;
    }
    
    return true;
}

const char* CSocket::GetStateStr(void)
//Returns a description of the socket's current state
{
    switch(socketState)
    {
        case SOCKET_STATE_DISCONNECTED:
            return "Disconnected";
        case SOCKET_STATE_CONNECTED:
            return "Connected";
        case SOCKET_STATE_LISTENING:
            return "Listening";
        case SOCKET_STATE_CONNECTING:
            return "Connecting";
    }
    
    return "";
}

string CSocket::GetRemoteHostName(void)
//Get the name of the remote socket
{
    sockaddr_in ipInfo;
    int ipInfoSize = sizeof(ipInfo);
    char nameStr[256];
    
    nameStr[0] = '\0';
    
    if(getpeername(hSocket, (sockaddr *) &ipInfo, &ipInfoSize) == 0)
    {
	    getnameinfo((sockaddr *) &ipInfo, ipInfoSize, nameStr, sizeof(nameStr), NULL, 0, NI_NAMEREQD | NI_NOFQDN);
    }
    
    return string(nameStr);
}

string CSocket::GetRemoteHostIP(void)
//Get the IP number of the remote socket
{
    sockaddr_in ipInfo;
    int ipInfoSize = sizeof(ipInfo);
    string ipStr;
    
    if(getpeername(hSocket, (sockaddr *) &ipInfo, &ipInfoSize) == 0)
    {
        ipStr = inet_ntoa(ipInfo.sin_addr);
    }
    
    return ipStr;  
}

int CSocket::GetRemoteHostPort(void)
//Get the port number of the remote socket
{
    sockaddr_in portInfo;
    int portInfoSize = sizeof(portInfo);
    int portNum = 0;
    
    if(getpeername(hSocket, (sockaddr *) &portInfo, &portInfoSize) == 0)
    {
        portNum = int(ntohs(portInfo.sin_port));
    }
    
    return portNum;
}

string CSocket::GetLocalHostName(void)
//Get the name of the local socket
{
    sockaddr_in ipInfo;
    int ipInfoSize = sizeof(ipInfo);
    char nameStr[256];
    
    nameStr[0] = '\0';
    
    if(getsockname(hSocket, (sockaddr *) &ipInfo, &ipInfoSize) == 0)
    {
	    getnameinfo((sockaddr *) &ipInfo, ipInfoSize, nameStr, sizeof(nameStr), NULL, 0, NI_NAMEREQD | NI_NOFQDN);
    }
    
    return string(nameStr);
}
	
string CSocket::GetLocalHostIP(void)
//Get the IP number of the local socket
{
    sockaddr_in ipInfo;
    int ipInfoSize = sizeof(ipInfo);
    string ipStr;
    
    if(getsockname(hSocket, (sockaddr *) &ipInfo, &ipInfoSize) == 0)
    {
        ipStr = inet_ntoa(ipInfo.sin_addr);
    }
    
    return ipStr;      
}

int CSocket::GetLocalHostPort(void)
//Get the port number of the local socket
{
    sockaddr_in portInfo;
    int portInfoSize = sizeof(portInfo);
    int portNum = 0;
    
    if(getsockname(hSocket, (sockaddr *) &portInfo, &portInfoSize) == 0)
    {
        portNum = int(ntohs(portInfo.sin_port));
    }
    
    return portNum;
}

