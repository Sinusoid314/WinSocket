#include <sstream>
#include "..\Socket.h"
#include "ClientSocket.h"

using namespace std;


const char* clientWndClassName = "SocketTestWindow";
HWND hClientWnd;
HWND hClientDispWnd;
HWND hClientConnectBtn;
HWND hClientDisconnectBtn;
HWND hClientStatusWnd;
CSocket* clientSocketPtr;

bool ClientSocketSetup(void)
//
{
    WNDCLASSEX winClassInfo;
    int winWidth = int(GetSystemMetrics(SM_CXSCREEN) / 1.9);
    int winHeight = int(GetSystemMetrics(SM_CYSCREEN) / 1.6);
    int winX = (GetSystemMetrics(SM_CXSCREEN) - winWidth) / 2;
    int winY = (GetSystemMetrics(SM_CYSCREEN) - winHeight) / 2;
    
    //Setup socket support
    SocketSetup();
    
    //Register window class
    winClassInfo.hInstance = (HINSTANCE) GetModuleHandle(NULL);
    winClassInfo.lpszClassName = clientWndClassName;
    winClassInfo.lpfnWndProc = ClientWndProc;
    winClassInfo.style = CS_DBLCLKS;
    winClassInfo.cbSize = sizeof (WNDCLASSEX);
    winClassInfo.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    winClassInfo.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    winClassInfo.hCursor = LoadCursor (NULL, IDC_ARROW);
    winClassInfo.lpszMenuName = NULL;
    winClassInfo.cbClsExtra = 0;
    winClassInfo.cbWndExtra = 0;
    winClassInfo.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
    if (!RegisterClassEx (&winClassInfo))
    {
        return false;
    }
    
    //Create window
    hClientWnd = CreateWindowEx(0, clientWndClassName, "Client Socket", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                winX, winY, winWidth, winHeight, HWND_DESKTOP, NULL,
                                (HINSTANCE) GetModuleHandle(NULL), NULL);
    
    //Create socket
    PrintStr("Creating socket... \r\n");
    clientSocketPtr = new CSocket;
    clientSocketPtr->socketName = "client1";
    PrintStr("Socket '");
    PrintStr(clientSocketPtr->socketName.c_str());
    PrintStr("' created. \r\n\r\n");
    
    //Set client socket events
    PrintStr("Setting socket events... \r\n");
    clientSocketPtr->AddEvent(FD_CONNECT, Socket_OnConnect);
    clientSocketPtr->AddEvent(FD_CLOSE, Socket_OnDisconnect);
    clientSocketPtr->AddEvent(FD_READ, Socket_OnDataArrival);
    clientSocketPtr->AddEvent(FD_ERROR, Socket_OnError);
    PrintStr("Socket events set. \r\n\r\n");
    
    return true;
}

void ClientSocketCleanup(void)
//
{
    //Close socket
    PrintStr("Closing socket... \r\n");
    delete clientSocketPtr;
    PrintStr("Socket closed. \r\n\r\n");
    
    //Close window
    DestroyWindow(hClientWnd);
    
    //Unregister window class
    UnregisterClass(clientWndClassName, (HINSTANCE) GetModuleHandle(NULL));
    
    //Cleanup socket support
    SocketCleanup();
}

int WINAPI WinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nFunsterStil)
//
{
    MSG messages;

    ClientSocketSetup();
    
    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage (&messages, NULL, 0, 0))
    {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }
    
    ClientSocketCleanup();
    
    return messages.wParam;
}

LRESULT CALLBACK ClientWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
//
{
    switch (message)
    {
        case WM_COMMAND:
            if(HIWORD(wParam) == BN_CLICKED)
            {
                if((HWND)lParam == hClientConnectBtn)
                {
                    ConnectBtn_OnClick();
                }
                else
                if((HWND)lParam == hClientDisconnectBtn)
                {
                    DisconnectBtn_OnClick();
                }
            }
            return 0;
            
        case WM_SIZE:
            MoveWindow(hClientDispWnd, 0, 40, LOWORD(lParam), HIWORD(lParam)-40, TRUE);
            return TRUE;
            
        case WM_CREATE:
            hClientConnectBtn = CreateWindowEx(0, "BUTTON", "Connect",
                                               WS_VISIBLE|WS_CHILD|WS_BORDER,
                                               5, 5, 85, 30, hwnd, NULL,
                                               (HINSTANCE) GetModuleHandle(NULL), NULL);
            
            hClientDisconnectBtn = CreateWindowEx(0, "BUTTON", "Disconnect",
                                                  WS_VISIBLE|WS_CHILD|WS_BORDER|WS_DISABLED,
                                                  95, 5, 85, 30, hwnd, NULL,
                                                  (HINSTANCE) GetModuleHandle(NULL), NULL);
            
            hClientStatusWnd = CreateWindowEx(0, "STATIC", "disconnected",
                                              WS_VISIBLE|WS_CHILD|WS_BORDER,
                                              200, 10, 95, 20, hwnd, NULL,
                                              (HINSTANCE) GetModuleHandle(NULL), NULL);
            
            hClientDispWnd = CreateWindowEx(0, "EDIT", "",
                                            WS_VISIBLE|WS_CHILD|WS_BORDER|
                                            WS_VSCROLL|WS_HSCROLL|ES_READONLY|
                                            ES_MULTILINE|ES_WANTRETURN|
                                            ES_AUTOHSCROLL|ES_AUTOVSCROLL,
                                            1, 1, 1, 1, hwnd, NULL,
                                            (HINSTANCE) GetModuleHandle(NULL), NULL);
            SendMessage(hClientDispWnd, EM_SETLIMITTEXT, 200000, 0);
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage (0);
            return 0;
            
        default:
            return DefWindowProc (hwnd, message, wParam, lParam);
    }
}

void PrintStr(const char* outStr)
//Add outStr to display
{
    int dispTextLen;
    
    dispTextLen = GetWindowTextLength( hClientDispWnd );
    SendMessage( hClientDispWnd, EM_SETSEL, (WPARAM)dispTextLen, (LPARAM)dispTextLen );
    SendMessage( hClientDispWnd, EM_REPLACESEL, 0, (LPARAM) ((LPSTR) outStr) );
}

void PrintNum(double outNum)
//Convert outNum to a string and add it to display
{
    string outStr;
    
    outStr = static_cast<ostringstream*>(&(ostringstream() << outNum))->str();
    PrintStr(outStr.c_str());
}

void ConnectBtn_OnClick(void)
//
{
    const char* hostNameStr = "127.0.0.1";
    int hostPort = 69;
    
    //Connect client socket to a remote server
    clientSocketPtr->Connect(hostNameStr, hostPort);
}

void DisconnectBtn_OnClick(void)
//
{
    
}

void Socket_OnConnect(CSocket* socketPtr, const CSocketEvent& eventObj)
//
{
    string initData = "";
    
    initData += "GET / HTTP/1.0\r\n";
    initData += "Host: www.britcoms.com\r\n";
    initData += "\r\n";
    
    //Print socket state
    PrintStr("Socket "); PrintStr(socketPtr->GetStateStr()); PrintStr("\r\n\r\n");
    
    //Print local socket info
    PrintStr("Local Name: "); PrintStr(socketPtr->GetLocalHostName().c_str()); PrintStr("\r\n");
    PrintStr("Local IP Address: "); PrintStr(socketPtr->GetLocalHostIP().c_str()); PrintStr("\r\n");
    PrintStr("Local port: "); PrintNum(socketPtr->GetLocalHostPort()); PrintStr("\r\n\r\n");
    
    //Print remote socket info
    PrintStr("Remote Name: "); PrintStr(socketPtr->GetRemoteHostName().c_str()); PrintStr("\r\n");
    PrintStr("Remote IP Address: "); PrintStr(socketPtr->GetRemoteHostIP().c_str()); PrintStr("\r\n");
    PrintStr("Remote port: "); PrintNum(socketPtr->GetRemoteHostPort()); PrintStr("\r\n\r\n");
    
    PrintStr("Sending initial data... \r\n");
    socketPtr->SendData(initData);
    PrintStr("Initial data sent. \r\n\r\n");
}

void Socket_OnDisconnect(CSocket* socketPtr, const CSocketEvent& eventObj)
//
{
    //Print socket state
    PrintStr("Socket "); PrintStr(socketPtr->GetStateStr()); PrintStr("\r\n\r\n");
}

void Socket_OnDataArrival(CSocket* socketPtr, const CSocketEvent& eventObj)
//
{
    string dataStr = "";
    
    //Retrieve incoming data from socketPtr
    socketPtr->GetData(dataStr);
    PrintStr("Data received: \r\n");
    PrintStr(dataStr.c_str());
    PrintStr("\r\nEND DATA \r\n\r\n");
}

void Socket_OnError(CSocket* socketPtr, const CSocketEvent& eventObj)
//
{
    PrintStr("Socket ERROR: \r\n");
    PrintStr((socketPtr->socketErrorStr + "\r\n\r\n").c_str());
    
    //Print socket state
    PrintStr("Socket "); PrintStr(socketPtr->GetStateStr()); PrintStr("\r\n\r\n");
}


