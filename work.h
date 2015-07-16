//
// Created by wlad on 7/15/15.
//

#ifndef LINUX_DEAMON_WORK_H
#define LINUX_DEAMON_WORK_H

// лимит для установки максимально кол-во открытых дискрипторов
#define FD_LIMIT                        1024*10

// константы для кодов завершения процесса
#define CHILD_NEED_WORK                 1
#define CHILD_NEED_TERMINATE    2

int WorkProc();

// функция для остановки потоков и освобождения ресурсов
void DestroyWorkThread();

// функция которая инициализирует рабочие потоки
int InitWorkThread();

int server_start_listen();

int server_establish_connection(int);
void *work_tread(void *);
void *tcp_server_read(void *);
#endif //LINUX_DEAMON_WORK_H
