//
// Created by Daniel on 20.12.2020.
//

#ifndef LAB31_HTTPSERVICE_H
#define LAB31_HTTPSERVICE_H

#include "string.h"
#include "stdio.h"
#include <ntsid.h>
#include <stdlib.h>
#include <netdb.h>

char *createGet(char *url, size_t *len);

int isMethodGet(const char *httpData);

char *getUrlFromData(char *httpData);

char *getHostFromUrl(char *sH);

int getStatusCodeAnswer(char *httpData);

long getContentLengthFromAnswer(char *httpData);

int getIndexOfBody(char *buff, size_t len);

int getServerSocketBy(char *url);

#endif //LAB31_HTTPSERVICE_H
