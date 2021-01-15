#include "HttpRequest.h"
#include <math.h>
#include "openssl\ssl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CHttpRequest::CHttpRequest(void *p, bool IsGSCtrl) {
    g_pMainDlg = p;
    m_IsGSCtrl = IsGSCtrl;
}

CHttpRequest::~CHttpRequest() {}

//*******************************************************************************************************
// MemBufferCreate:
//                  Passed a MemBuffer structure, will allocate a memory buffer
//                   of MEM_BUFFER_SIZE.  This buffer can then grow as needed.
//*******************************************************************************************************
void CHttpRequest::MemBufferCreate(MemBuffer *b) {
    b->size = MEM_BUFFER_SIZE;
    b->buffer = (unsigned char *)malloc(b->size);
    b->position = b->buffer;
    memset(b->buffer, 0, b->size);
}

//*******************************************************************************************************
// MemBufferGrow:
//                  Double the size of the buffer that was passed to this function.
//*******************************************************************************************************
void CHttpRequest::MemBufferGrow(MemBuffer *b) {
    size_t sz;
    sz = b->position - b->buffer;
    b->size = b->size * 2;
    b->buffer = (unsigned char *)realloc(b->buffer, b->size);
    b->position = b->buffer + sz;  // readjust current position
}
//*******************************************************************************************************
// MemBufferAddByte:
//                  Add a single byte to the memory buffer, grow if needed.
//*******************************************************************************************************
void CHttpRequest::MemBufferAddByte(MemBuffer *b, unsigned char byt) {
    if ((size_t)(b->position - b->buffer) >= b->size) MemBufferGrow(b);

    *(b->position++) = byt;
}

//*******************************************************************************************************
// MemBufferAddBuffer:
//                  Add a range of bytes to the memory buffer, grow if needed.
//*******************************************************************************************************
void CHttpRequest::MemBufferAddBuffer(MemBuffer *b, unsigned char *buffer, size_t size) {
    while (((size_t)(b->position - b->buffer) + size) >= b->size) MemBufferGrow(b);

    memcpy(b->position, buffer, size);
    b->position += size;
}

//*******************************************************************************************************
// GetHostAddress:
//                  Resolve using DNS or similar(WINS,etc) the IP
//                   address for a domain name such as www.wdj.com.
//*******************************************************************************************************
DWORD CHttpRequest::GetHostAddress(const char *host) {
    struct hostent *phe;
    char *p;

    phe = gethostbyname(host);

    if (phe == NULL) return 0;

    p = *phe->h_addr_list;
    return *((DWORD *)p);
}

//*******************************************************************************************************
// SendString:
//                  Send a std::string(null terminated) over the specified socket.
//*******************************************************************************************************
bool CHttpRequest::SendString(SOCKET sock, const char *str) {
    if (SOCKET_ERROR == send(sock, str, strlen(str), 0)) {
        return false;
    }
    return true;
}

//*******************************************************************************************************
// ValidHostChar:
//                  Return TRUE if the specified character is valid
//                      for a host name, i.e. A-Z or 0-9 or -.:
//*******************************************************************************************************
bool CHttpRequest::ValidHostChar(char ch) { return (isalpha(ch) || isdigit(ch) || ch == '-' || ch == '.' || ch == ':'); }

//*******************************************************************************************************
// ParseURL:
//                  Used to break apart a URL such as
//                      http://www.localhost.com:80/TestPost.htm into protocol, port, host and request.
//*******************************************************************************************************
void CHttpRequest::ParseURL(bool isHttps, string url, char *protocol, int lprotocol, char *host, int lhost, char *request, int lrequest, int *port) {
    char *work, *ptr, *ptr2;

    *protocol = *host = *request = 0;
    if (isHttps) {
        *port = 443;  // HTTPS端口443
    } else {
        *port = 80;  // HTTP端口80
    }
    work = strdup(url.c_str());
    strupr(work);

    ptr = strchr(work, ':');  // find protocol if any
    if (ptr != NULL) {
        *(ptr++) = 0;
        lstrcpynA(protocol, work, lprotocol);
    } else {
        lstrcpynA(protocol, "HTTP", lprotocol);
        ptr = work;
    }

    if ((*ptr == '/') && (*(ptr + 1) == '/'))  // skip past opening /'s
        ptr += 2;

    ptr2 = ptr;  // find host
    while (ValidHostChar(*ptr2) && *ptr2) ptr2++;

    *ptr2 = 0;
    lstrcpynA(host, ptr, lhost);

    lstrcpynA(request, url.c_str() + (ptr2 - work), lrequest);  // find the request

    ptr = strchr(host, ':');  // find the port number, if any
    if (ptr != NULL) {
        *ptr = 0;
        *port = atoi(ptr + 1);
    }
    free(work);
}

void CHttpRequest::GetTime(std::string URL, unsigned long &time) {
    if ((URL.find("vb.htm?sdformat=1") >= 0)) {
        time = 60000;
    } else if (URL.find("timefrequency=-1") >= 0 || (URL.find("vb.htm?sd") >= 0)) {
        time = 20000;
    } else {
        time = 1000;
    }
}

//*******************************************************************************************************
// GetHttpHeaderSend:
//                  Used to make up HTTP request head.
//*******************************************************************************************************
char *CHttpRequest::GetHttpHeaderSend(bool isPost, BYTE *postMessage, DWORD postLength, char *request, char *host, string &base64) {
    char headerSend[SEND_BUFFER_SIZE];
    char buffer[2048];

    if (!*request) lstrcpynA(request, "/", sizeof(request));
    if (postMessage == NULL) {
        strcpy_s(headerSend, "GET ");
    } else {
        strcpy_s(headerSend, "POST ");
    }

    strcat_s(headerSend, request);
    strcat_s(headerSend, " HTTP/1.1\r\n");
    strcat_s(headerSend, "Accept: */*\r\n");
    strcat_s(headerSend, "Accept-Language: en-us\r\n");
    strcat_s(headerSend, "Accept-Encoding: gzip, default\r\n");
    strcat_s(headerSend, "User-Agent: http client\r\n");
    strcat_s(headerSend, "Authorization: Basic ");
    strcat_s(headerSend, base64.c_str());
    strcat_s(headerSend, "\r\n");
    if (postLength) {
        sprintf_s(buffer, "Content-Length: %ld\r\n", postLength);
        strcat_s(headerSend, buffer);
    }

    strcat_s(headerSend, "Host: ");
    strcat_s(headerSend, host);
    strcat_s(headerSend, "\r\n");

    if (isPost) {
        strcat_s(headerSend, "Content-Type: application/x-www-form-urlencoded\r\n");
        if (base64 != "") {
            strcat_s(headerSend, LPCSTR(base64.c_str()));
            strcat_s(headerSend, "\r\n");
        }
    }

    strcat_s(headerSend, "\r\n");

    if ((postMessage != NULL) && postLength) {
        postMessage[postLength] = '\0';
        strcat_s(headerSend, (const char *)postMessage);
    }

    return headerSend;
}

//*******************************************************************************************************
// SendHTTP:
//                  Main entry point for this code.
//                    IsHttps       - TRUE if Https, FALSE if Http
//                    url           - The URL to GET/POST to/from.
//                    headerSend        - Headers to be sent to the server.
//                    post          - Data to be posted to the server, NULL if GET.
//                    postLength    - Length of data to post.
//                    req           - Contains the message and headerSend sent by the server.
//
//                    returns 1 on failure, 0 on success.
//*******************************************************************************************************
int CHttpRequest::SendHTTP(bool isPost, bool isHttps, std::string url, BYTE *post, DWORD postLength, HTTPRequest *req, std::string &base64) {
    WSADATA wsaData;
    SOCKADDR_IN sin;
    SOCKET sock = -1;
    char buffer[2048];
    char protocol[20], host[2048], request[2048];
    int l, port, chars, nStarup, nErr = 0;
    MemBuffer headersBuffer, messageBuffer;
    char headerSend[SEND_BUFFER_SIZE];
    BOOL done;
    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;
    memset(buffer, 0, sizeof(buffer));

    std::string tempURL(url.c_str());
    if (isHttps) {
        ParseURL(TRUE, url, protocol, sizeof(protocol), host, sizeof(host),  // Parse the URL
                 request, sizeof(request), &port);
    } else {
        ParseURL(FALSE, url, protocol, sizeof(protocol), host, sizeof(host),  // Parse the URL
                 request, sizeof(request), &port);
    }
    // 1. 初始化WSA
    nStarup = WSAStartup(MAKEWORD(2, 2), &wsaData);  // WSAStartup()调用之后才能调用进一步的Windows Sockets API函数
    if (nStarup != NO_ERROR) {
        wprintf(L" WSAStartup function failed with error: %d\n", nStarup);
        return 0;
    }

    // 2. 创建套接字并连接服务器
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    DWORD time;
    GetTime(tempURL, time);

    sin.sin_family = AF_INET;
    sin.sin_port = htons((unsigned short)port);
    sin.sin_addr.s_addr = GetHostAddress(host);

    int nSocketRet;
    unsigned long block = 1;

    if (SOCKET_ERROR == ioctlsocket(sock, FIONBIO, (unsigned long *)&block)) {  // block为1时，设置socket为非阻塞模式
        return 1;
    }

    nSocketRet = connect(sock, (LPSOCKADDR)&sin, sizeof(SOCKADDR_IN));
    if (nSocketRet == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {  // 非阻塞第一次立即返回
            return 1;
        }
    }

    fd_set write_set;
    struct timeval TimeOut;
    FD_ZERO(&write_set);
    FD_SET(sock, &write_set);
    TimeOut.tv_sec = 0;
    TimeOut.tv_usec = 4000 * 1000;
    nSocketRet = select(1, NULL, &write_set, NULL, &TimeOut);
    if (nSocketRet <= 0) {
        return 1;
    }
    block = 0;
    if (SOCKET_ERROR == ioctlsocket(sock, FIONBIO, (unsigned long *)&block)) {  // block为0时，设置socket为阻塞模式
        return 1;
    }
    if (SOCKET_ERROR == setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&time, sizeof(time))) {  // 设置send超时时间
        return 1;
    }
    DWORD time1;
    time1 = 10000;                                                                                         // 接收超时时间设置为10秒
    if (SOCKET_ERROR == setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&time1, sizeof(time1))) {  // 设置recv超时时间
        return 1;
    }

    // 3. 如果是Https请求，初始化SSL
    if (isHttps) {
        SSL_library_init();                         // SSL 库初始化
        OpenSSL_add_all_algorithms();               // 载入所有 SSL 算法
        SSL_load_error_strings();                   // 载入所有 SSL 错误消息
        ctx = SSL_CTX_new(SSLv23_client_method());  // 以 SSL V2 和 V3 标准兼容方式产生一个 SSL_CTX ，即 SSL Content Text
        if (!ctx) {
            nSocketRet = closesocket(sock);
            if (nSocketRet == SOCKET_ERROR) {
                wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
            }
            sock = 0;
            WSACleanup();
            return 0;
        }
        ssl = SSL_new(ctx);  // 基于 ctx 产生一个新的 SSL

        if (!ssl) {
            SSL_CTX_free(ctx);
            wprintf(L"SSL_new failed\n");
            nSocketRet = closesocket(sock);
            if (nSocketRet == SOCKET_ERROR) {
                wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
            }
            sock = 0;
            WSACleanup();
            return 0;
        }
    }

    // 4. 获取HTTP请求头
    char *headerSend1 = GetHttpHeaderSend(isPost, post, postLength, request, host, base64);
    strcpy(headerSend, headerSend1);

    // 5. 发HTTP请求消息给服务器
    if (isHttps) {
        nErr = SSL_set_fd(ssl, sock);                           // 绑定原套接字到 ssl套接字
        nErr = SSL_connect(ssl);                                // 使用ssl套件字建立连接
        nErr = SSL_write(ssl, headerSend, strlen(headerSend));  // 发HTTP请求消息给服务器
    } else {
        if (!SendString(sock, headerSend)) {
            return 1;
        }
    }

    // 6. 接收服务器返回的信息
    done = FALSE;
    chars = 0;
    req->headerSend = (char *)malloc(sizeof(char *) * strlen(headerSend) + 1);
    strcpy(req->headerSend, (char *)headerSend);

    MemBufferCreate(&headersBuffer);

    while (!done) {  // 这里接收http头
        if (isHttps) {
            l = SSL_read(ssl, buffer, 1);  // 接收服务器返回的消息
        } else {
            l = recv(sock, buffer, 1, 0);  // 接收服务器返回的消息
        }

        if (l <= 0) {
            int a = GetLastError();
            done = TRUE;
        }

        switch (*buffer) {
            case '\r':
                break;
            case '\n':
                if (chars == 0) done = TRUE;
                chars = 0;
                break;
            default:
                chars++;
                break;
        }

        MemBufferAddByte(&headersBuffer, *buffer);
    }

    req->headerReceive = (char *)headersBuffer.buffer;
    *(headersBuffer.position) = 0;

    MemBufferCreate(&messageBuffer);  // Now read the HTTP body

    do {  // 这里接收http体
        if (isHttps) {
            l = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        } else {
            l = recv(sock, buffer, sizeof(buffer) - 1, 0);
        }
        if (l <= 0) break;
        if (l > 0) {
            *(buffer + l) = 0;
            MemBufferAddBuffer(&messageBuffer, (unsigned char *)&buffer, l);
        }
    } while (l > 0);

    *messageBuffer.position = 0;
    req->message = (char *)messageBuffer.buffer;

    req->messageLength = (messageBuffer.position - messageBuffer.buffer);

    // 7. 释放、关闭
    SSL_free(ssl);
    SSL_CTX_free(ctx);

    nSocketRet = closesocket(sock);  // Cleanup
    if (nSocketRet == SOCKET_ERROR) {
        wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
    }
    sock = 0;
    WSACleanup();
    return 0;
}

//*******************************************************************************************************
// SendRequest
//
//*******************************************************************************************************
int CHttpRequest::SendRequest(bool isPost, bool isHttps, string url, string &psHeaderSend, string &psHeaderReceive, string &psMessage,
                              string &base64) {
    HTTPRequest req;
    int i, rtn;
    LPSTR buffer;

    req.headerSend = NULL;
    req.headerReceive = NULL;
    req.message = NULL;

    if (isPost) {  // POST方式
        i = psHeaderSend.length();
        buffer = (char *)malloc(i + 1);
        strcpy(buffer, psHeaderSend.c_str());
        rtn = SendHTTP(isPost, isHttps, url, (unsigned char *)buffer, i, &req, base64);
        free(buffer);
    } else {  // GET方式
        rtn = SendHTTP(isPost, isHttps, url, NULL, 0, &req, base64);
        if (rtn == 2) {
            return 2;
        }
    }

    if (!rtn) {  // Output message and/or headerSend
        psHeaderSend = req.headerSend;
        psHeaderReceive = req.headerReceive;
        psMessage = req.message;

        free(req.headerSend);
        free(req.headerReceive);
        free(req.message);
        return 1;
    } else {
        return 0;
    }
}