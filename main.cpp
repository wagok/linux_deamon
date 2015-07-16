#if !defined(_GNU_SOURCE)
        #define _GNU_SOURCE
#endif


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "config.h"
#include "monitor.h"

int main(int argc, char** argv)
{
    int status;
    int pid;

    // если параметров командной строки меньше двух, то покажем как использовать демана
    if (argc != 2)
    {
        printf("Usage: ./my_daemon filename.cfg\n");
        return -1;
    }

    // загружаем файл конфигурации
    status = LoadConfig(argv[1]);

    if (!status) // если произошла ошибка загрузки конфига
    {
        printf("Error: Load config failed\n");
        return -1;
    }

    // создаем потомка
    pid = fork();

    if (pid == -1) // если не удалось запустить потомка
    {
        // выведем на экран ошибку и её описание
        printf("Start Daemon Error: %s\n", strerror(errno));

        return -1;
    }
    else if (!pid) // если это потомок
    {

        // Данная функция будет осуществлять слежение за процессом
        status = MonitorProc();

        return status;
    }
    else // если это родитель
    {
        // завершим процес, т.к. основную свою задачу (запуск демона) мы выполнили
        return 0;
    }
}
 
 