//
// Created by wlad on 7/15/15.
//


#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "options.h"
#include "logger.h"


void SetPidFile(const char *Filename)
{
    FILE* f;

    f = fopen(Filename, "w+");
    if (f)
    {
        fprintf(f, "%u", getpid());
        fclose(f);
    } else {
        WriteLog("[MONITOR] Can't open pid file (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}




// функция установки максимального кол-во дескрипторов которое может быть открыто
int SetFdLimit(int MaxFd)
{
    struct rlimit lim;
    int           status;

    // зададим текущий лимит на кол-во открытых дискриптеров
    lim.rlim_cur = MaxFd;
    // зададим максимальный лимит на кол-во открытых дискриптеров
    lim.rlim_max = MaxFd;

    // установим указанное кол-во
    status = setrlimit(RLIMIT_NOFILE, &lim);

    return status;
}
