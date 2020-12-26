//
// Created by Daniel on 20.12.2020.
//
#include "httpService.h"


char *createGet(char *url, size_t *len) {

    //printf("URL=%s\n",url);
    //protocol
    char *afterProtocol = strstr(url, "://");
    if (NULL == afterProtocol) {
        fprintf(stderr, "Incorrect input.\n");
    }

    //host
    int hostLength;

    char *afterHost = strchr(afterProtocol + 3, '/');
    if (NULL == afterHost) {
        hostLength = strlen(afterProtocol + 3);
    } else {
        hostLength = afterHost - (afterProtocol + 3);
    }

    char hostName[hostLength + 1];
    strncpy(hostName, afterProtocol + 3, hostLength);
    hostName[hostLength] = 0;
    //---------------------------------------------Send request

    char format[] = "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n\0";
    //printf("format size = %d",sizeof(format));

    char *buffer;
    if (NULL == afterHost) {
        buffer = (char *) malloc(sizeof(char) * (27 + 1 + sizeof(hostName)));
        *len = 27 + 1 + sizeof(hostName);
        sprintf(buffer, format, "/", hostName);
    } else {
        buffer = (char *) malloc(sizeof(char) * (27 + strlen(afterHost) + sizeof(hostName)));
        *len = 27 + strlen(afterHost) + sizeof(hostName);
        //printf("LEN=%d",*len);
        sprintf(buffer, format, afterHost, hostName);
    }

    //printf("newGetMethod = (%s)\n", buffer);
    return buffer;
}


int isMethodGet(const char *httpData) {
    return httpData[0] == 'G' &&
           httpData[1] == 'E' &&
           httpData[2] == 'T' &&
           httpData[3] == ' ';
}

char *getUrlFromData(char *httpData) {

    char *smth = httpData + 4;
    char *endSmth = strchr(smth, ' ');

    char *result = (char *) malloc(sizeof(char) * (endSmth - smth + 1));
    if (NULL == result) {
        printf("ERROR WHILE MALLOC getUrlFromData");
    }

    memcpy(result, smth, (size_t) (endSmth - smth));
    result[endSmth - smth] = '\0';

    //printf("URL:%s\n", result);
    char *KASPSHIT = strstr(httpData, "kis.v2.scr.kaspersky");
    if (KASPSHIT != NULL) {
        return NULL;
    }
    return result;
}

char *getHostFromUrl(char *sH) {

    char *startHost = sH + 7;
    char *endHost = strchr(startHost, '/');

    char *result = (char *) malloc(sizeof(char) * (endHost - startHost + 1));
    if (NULL == result) {
        printf("ERROR WHILE MALLOC getHostFromUrl");
        //return 1;

    }

    memcpy(result, startHost, (size_t) (endHost - startHost));
    result[endHost - startHost] = '\0';

    //printf("Host:%s\n", result);
    return result;
}

int getStatusCodeAnswer(char *httpData) {
    char *afterHTTP = httpData + 9;// ignore "HTTP/1.1 "

    char *StrAfterCode;
    int statusCode = (int) strtol(afterHTTP, &StrAfterCode, 10);
    if (StrAfterCode == afterHTTP || statusCode <= 0) {
        return -1;
    }
    return statusCode;
}

long getContentLengthFromAnswer(char *httpData) {

    char header[] = "Content-Length:";

    char *startLength = strstr(httpData, header);

    if (startLength == NULL) {
        return -1;
    }

    startLength += strlen(header) + 1;
    char *StrAfterLength;
    long contentLength = strtol(startLength, &StrAfterLength, 10);
    if (StrAfterLength == startLength || contentLength <= 0) {
        return -1;
    }
    //printf("ContLength = %d\n", contentLength);
    return contentLength;
}

int getIndexOfBody(char *buff, size_t len) {
    for (size_t j = 0; j < len - 3; ++j) {
        if (buff[j] == '\r' && buff[j + 1] == '\n' &&
            buff[j + 2] == '\r' && buff[j + 3] == '\n') {

            return (int) j + 4;
        }
    }

    return -1;
}

int getServerSocketBy(char *url) {

    char *host = getHostFromUrl(url);

    struct hostent *hostInfo = gethostbyname(host);

    if (NULL == hostInfo) {
        fprintf(stderr, "Cannot get host by name\n");
        return -1;
    }

    struct sockaddr_in destinationAddress;

    destinationAddress.sin_family = AF_INET;
    destinationAddress.sin_port = htons(80);
    memcpy(&destinationAddress.sin_addr, hostInfo->h_addr, hostInfo->h_length);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == -1) {
        perror("Cannot create socket");
        return -1;
    }

    if (-1 == connect(serverSocket, (struct sockaddr *) &destinationAddress, sizeof(destinationAddress))) {
        perror("Cannot connect");
        return -1;
    }
    return serverSocket;
}