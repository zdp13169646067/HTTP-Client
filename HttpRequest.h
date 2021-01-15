#if !defined(AFX_REQUEST_H__9F2C9BB6_CBA7_40AF_80A4_09A1CE1CE220__INCLUDED_)
#define AFX_REQUEST_H__9F2C9BB6_CBA7_40AF_80A4_09A1CE1CE220__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif  // _MSC_VER > 1000
#include <string>
#include "winsock.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define MEM_BUFFER_SIZE 150000
#define SEND_BUFFER_SIZE 150000

/*
    HTTPRequest: Structure that returns the HTTP headers and message
                    from the request
*/
typedef struct {
    char *headerSend;     // Pointer to HTTP header Send
    char *headerReceive;  // Pointer to HTTP headers Receive
    char *message;        // Pointer to the HTTP message
    long messageLength;   // Length of the message
} HTTPRequest;

/*
    MemBuffer:  Structure used to implement a memory buffer, which is a
                buffer of memory that will grow to hold variable sized
                parts of the HTTP message.
                */
typedef struct {
    unsigned char *buffer;
    unsigned char *position;
    size_t size;
} MemBuffer;

class CHttpRequest {
   public:
    CHttpRequest(void *p, bool IsGSCtrl = false);
    virtual ~CHttpRequest();
    void *g_pMainDlg;
    bool m_IsGSCtrl;

   private:
    void MemBufferCreate(MemBuffer *b);
    void MemBufferGrow(MemBuffer *b);
    void MemBufferAddByte(MemBuffer *b, unsigned char byt);
    void MemBufferAddBuffer(MemBuffer *b, unsigned char *buffer, size_t size);
    DWORD GetHostAddress(const char *host);
    bool SendString(SOCKET sock, const char *str);
    bool ValidHostChar(char ch);
    void ParseURL(bool isHttps, std::string url, char *protocol, int lprotocol, char *host, int lhost, char *request, int lrequest, int *port);
    void GetTime(std::string URL, unsigned long &time);
    char *GetHttpHeaderSend(bool isPost, unsigned char *postMessage, unsigned long postLength, char *request, char *host, std::string &base64);
    virtual int SendHTTP(bool isPost, bool isHttps, std::string url, unsigned char *post, unsigned long postLength, HTTPRequest *req,
                         std::string &base64);

   public:
    int SendRequest(bool isPost, bool isHttps, std::string url, std::string &psHeaderSend, std::string &pszHeaderReceive, std::string &pszMessage,
                    std::string &base64);
};
#endif  // !defined(AFX_REQUEST_H__9F2C9BB6_CBA7_40AF_80A4_09A1CE1CE220__INCLUDED_)