//**********************************************************************
// CRhsSocket
// ==========
//
// C++ class to encapsulate a WinSock socket
//
// J. Rennie
// jrennie@cix.compulink.co.uk
// 16th April 1998
//**********************************************************************

#ifndef _INC_CRHSSOCKET
#define _INC_CRHSSOCKET

#include <winsock.h>
// #include <winsock2.h>


//**********************************************************************
// CRhsSocket
// ----------
//**********************************************************************

class CRhsSocket
{

  public:
    CRhsSocket();
    ~CRhsSocket();

    BOOL Startup(void);
    void Cleanup(void);

    BOOL Connect(const char* Peer, int PeerPort, const char* Host = NULL, int HostPort = 0);

    BOOL Listen(char* Host, int Port);
    CRhsSocket* Accept(char* Peer, int MaxLen);

    int printf(const char* Format, ...);

    char* gets(char* Data, int MaxLen);
    BOOL  puts(const char* Data);

    char sgetc(void);
    char sputc(char c);

    int read(void* Data, int Size, int NumObj);
    int write(const void* Data, int Size, int NumObj);

    int recv(char* buf, int len, int flags);
    int send(const char* buf, int len, int flags);
    int recvfrom(char* buf, int len, int flags);
    int sendto(const char* buf, int len, int flags);

    int select(BOOL Read = TRUE, BOOL Write = FALSE, BOOL Error = FALSE, DWORD Timeout = 0);

    int ReadTimeout(char* buf, int len, int flags);
    int WriteTimeout(const char* buf, int len, int flags);

    inline void Timeout(DWORD t) { m_Timeout = t; }
    inline DWORD Timeout(void) { return(m_Timeout); }

    BOOL Shutdown(int How);
    BOOL CloseSocket(void);
    BOOL Bind(const char* Host, int Port);
    inline BOOL BindSocket(char* Host, int Port) { return Bind(Host, Port); }
    BOOL BindInRange(char* Host, int Low, int High, int Retries);
    BOOL Linger(BOOL LingerOn, int LingerTime = 0);
    BOOL CreateSocket(DWORD Type = SOCK_STREAM);

    BOOL Socket(SOCKET Socket, BOOL Bound = TRUE);
    inline SOCKET Socket(void) { return(m_Socket); }

    BOOL SetPeer(const char* Peer, int Port);
    BOOL GetPeer(char* Peer, int MaxLen, int* Port);
    BOOL SetHost(const char* Host, int Port);
    BOOL GetHost(char* Host, int MaxLen, int* Port);

    char*  GetLastErrorMessage(char* Error, int MaxLen);
    inline DWORD GetLastError(void) { return(m_LastError); }

  private:
    SOCKET m_Socket;
    struct sockaddr m_HostAddr, m_PeerAddr;
    BOOL   m_Bound;

#ifdef WIN32
    OVERLAPPED m_ReadOverlapped, m_WriteOverlapped;
#endif
    DWORD m_Timeout;

    DWORD m_LastError;
};


//**********************************************************************
// End of CRhsSocket
// -----------------
//**********************************************************************

#endif // _INC_CRHSSOCKET

