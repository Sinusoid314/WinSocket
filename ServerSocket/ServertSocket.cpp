#include <sstream>
#include "..\Socket.h"
#include "ServerSocket.h"

using namespace std;


bool ServerSocketSetup(void)
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
    winClassInfo.lpszClassName = serverWndClassName;
    winClassInfo.lpfnWndProc = ServerWndProc;
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
    hServerWnd = CreateWindowEx(0, serverWndClassName, "Server Socket", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                winX, winY, winWidth, winHeight, HWND_DESKTOP, NULL,
                                (HINSTANCE) GetModuleHandle(NULL), NULL);
    
    //Create socket
    PrintStr("Creating socket... \r\n");
    serverSocketPtr = new CSocket;
    serverSocketPtr->socketName = "server1";
    PrintStr("Socket '");
    PrintStr(serverSocketPtr->socketName.c_str());
    PrintStr("' created. \r\n\r\n");
    
    //Set socket events
    PrintStr("Setting socket events... \r\n");
    serverSocketPtr->AddEvent(FD_ACCEPT, Socket_OnConnectionRequest);
    serverSocketPtr->AddEvent(FD_READ, Socket_OnDataArrival);
    serverSocketPtr->AddEvent(FD_CLOSE, Socket_OnDisconnect);
    serverSocketPtr->AddEvent(FD_ERROR, Socket_OnError);
    PrintStr("Socket events set. \r\n\r\n");
    
    return true;
}

void ServerSocketCleanup(void)
{
    //Close socket
    PrintStr("Closing socket... \r\n");
    delete serverSocketPtr;
    PrintStr("Socket closed. \r\n\r\n");
    
    //Close window
    DestroyWindow(hServerWnd);
    
    //Unregister window class
    UnregisterClass(serverWndClassName, (HINSTANCE) GetModuleHandle(NULL));
    
    //Cleanup socket support
    SocketCleanup();
}

int WINAPI WinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nFunsterStil)
{
    MSG messages;
    int listenPort = 69;
    
    ServerSocketSetup();
    
    //Print socket state
    PrintStr("Socket "); PrintStr(serverSocketPtr->GetStateStr()); PrintStr("\r\n\r\n");
    
    //Listen for incomming connection requests
    PrintStr("Calling Listen()... \r\n\r\n");
    serverSocketPtr->Listen(listenPort);
    
    //Print socket state
    PrintStr("Socket "); PrintStr(serverSocketPtr->GetStateStr()); PrintStr("\r\n\r\n");
    
    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage (&messages, NULL, 0, 0))
    {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }
    
    ServerSocketCleanup();
    
    return messages.wParam;
}

LRESULT CALLBACK ServerWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_SIZE:
            MoveWindow(hServerDispWnd, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
            return TRUE;
            
        case WM_CREATE:
            hServerDispWnd = CreateWindowEx(0, "EDIT", "",
                                            WS_VISIBLE|WS_CHILD|WS_BORDER|
                                            WS_VSCROLL|WS_HSCROLL|ES_READONLY|
                                            ES_MULTILINE|ES_WANTRETURN|
                                            ES_AUTOHSCROLL|ES_AUTOVSCROLL,
                                            1, 1, 1, 1, hwnd, NULL,
                                            (HINSTANCE) GetModuleHandle(NULL), NULL);
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
    
    dispTextLen = GetWindowTextLength( hServerDispWnd );
    SendMessage( hServerDispWnd, EM_SETSEL, (WPARAM)dispTextLen, (LPARAM)dispTextLen );
    SendMessage( hServerDispWnd, EM_REPLACESEL, 0, (LPARAM) ((LPSTR) outStr) );
}

void PrintNum(double outNum)
//Convert outNum to a string and add it to display
{
    string outStr;
    
    outStr = static_cast<ostringstream*>(&(ostringstream() << outNum))->str();
    PrintStr(outStr.c_str());
}

void Socket_OnConnectionRequest(CSocket* socketPtr, const CSocketEvent& eventObj)
{
    PrintStr("Socket received connection request. \r\n\r\n");
    
    //Print socket state
    PrintStr("Socket "); PrintStr(socketPtr->GetStateStr()); PrintStr("\r\n\r\n");
    
    //Accpet connection request
    PrintStr("Calling Accept()... \r\n\r\n");
    socketPtr->Accept();

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
}

void Socket_OnDisconnect(CSocket* socketPtr, const CSocketEvent& eventObj)
{
    //Print socket state
    PrintStr("Socket "); PrintStr(socketPtr->GetStateStr()); PrintStr("\r\n\r\n");
}

void Socket_OnDataArrival(CSocket* socketPtr, const CSocketEvent& eventObj)
{
    string dataStr = "";
    
    //Retrieve incoming data from socketPtr
    socketPtr->GetData(dataStr);
    PrintStr("Data received: \r\n");
    PrintStr(dataStr.c_str());
    PrintStr("\r\nEND DATA \r\n\r\n");
    
    //Send data back to client
    PrintStr("Sending data back to client... \r\n");
    socketPtr->SendData(dataStr);
    PrintStr("Data sent. \r\n\r\n");
}

void Socket_OnError(CSocket* socketPtr, const CSocketEvent& eventObj)
{
    PrintStr("Socket ERROR! \r\n\r\n");
    
    //Print socket state
    PrintStr("Socket "); PrintStr(socketPtr->GetStateStr()); PrintStr("\r\n\r\n");
}

