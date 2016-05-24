#ifndef __PARSE_H__
#define __PARSE_H__

int parsePacketFormatId(char *__source, char* __value);
int setPacketFormat(char *__buf, char* __format);
int getFormatIdByString(char* str);
unsigned int parseAuthentication(char *__buf);
unsigned int parseClientId(char *__buf);
unsigned int parseSrcClientId(char *__buf);

int parseDstImsi(char *__source, char *__value);
int parseSrcImsi(char *__source, char *__value);
int setSrcImsi(char *__source, char *__value);
int setDstImsi(char *__source, char *__value);

int parseSrcUser(char *__source, char *__value);
int parseDstUser(char *__source, char *__value);
int setSrcUser(char *__source, char *__value);
int setDstUser(char *__source, char *__value);

int parseSrcHouse(char *__source, char *__value);
int parseDstHouse(char *__source, char *__value);
int setSrcHouse(char *__source, char *__value);
int setDstHouse(char *__source, char *__value);

int parseSrcKey(char *__source, char *__value);
int parseDstKey(char *__source, char *__value);
int setSrcKey(char *__source, char *__value);
int setDstKey(char *__source, char *__value);

int parseFunction(char *__source, char *__value);
int setFunction(char *__source, char *__value);

int parseSrcDevType(char *__source, char *__value);
int parseDstDevType(char *__source, char *__value);
int setSrcDevType(char *__source, char *__value);
int setDstDevType(char *__source, char *__value);

char * strGetLine(char* str);
char* getValueByKey(char* src, char* key);

#endif

