//
// Created by wlad on 7/15/15.
//

#ifndef LINUX_DEAMON_OPTIONS_H
#define LINUX_DEAMON_OPTIONS_H
#include <wait.h>


// функция обработки сигналов

void SetPidFile(const char *Filename);

// функция установки максимального кол-во дескрипторов которое может быть открыто
int SetFdLimit(int MaxFd);

#endif //LINUX_DEAMON_OPTIONS_H
