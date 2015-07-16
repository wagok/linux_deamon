//
// Created by wlad on 7/15/15.
//
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include <execinfo.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include "work.h"
#include "options.h"
#include "logger.h"
#include "config.h"

// функция обработки сигналов
static void signal_error(int sig, siginfo_t *si, void *ptr) {
    void *ErrorAddr;
    void *Trace[16];
    int x;
    int TraceSize;
    char **Messages;

    // запишем в лог что за сигнал пришел
    WriteLog("[DAEMON] Signal: %s, Addr: 0x%0.16X\n", strsignal(sig), si->si_addr);


#if __WORDSIZE == 64 // если дело имеем с 64 битной ОС
    // получим адрес инструкции которая вызвала ошибку
    ErrorAddr = (void *) ((ucontext_t *) ptr)->uc_mcontext.gregs[REG_RIP];
#else
    // получим адрес инструкции которая вызвала ошибку
    ErrorAddr = (void*)((ucontext_t*)ptr)->uc_mcontext.gregs[REG_EIP];
#endif

    // произведем backtrace чтобы получить весь стек вызовов
    TraceSize = backtrace(Trace, 16);
    Trace[1] = ErrorAddr;

    // получим расшифровку трасировки
    Messages = backtrace_symbols(Trace, TraceSize);
    if (Messages) {
        WriteLog("== Backtrace ==\n");

        // запишем в лог
        for (x = 1; x < TraceSize; x++) {
            WriteLog("%s\n", Messages[x]);
        }

        WriteLog("== End Backtrace ==\n");
        free(Messages);
    }

    WriteLog("[DAEMON] Stopped\n");

    // остановим все рабочие потоки и корректно закроем всё что надо
    DestroyWorkThread();

    // завершим процесс с кодом требующим перезапуска
    exit(CHILD_NEED_WORK);
}

int WorkProc() {
    struct sigaction sigact;
    sigset_t sigset;
    int signo;
    int status;

    // сигналы об ошибках в программе будут обрататывать более тщательно
    // указываем что хотим получать расширенную информацию об ошибках
    sigact.sa_flags = SA_SIGINFO;
    // задаем функцию обработчик сигналов
    sigact.sa_sigaction = signal_error;

    sigemptyset(&sigact.sa_mask);

    // установим наш обработчик на сигналы

    sigaction(SIGFPE, &sigact, 0); // ошибка FPU
    sigaction(SIGILL, &sigact, 0); // ошибочная инструкция
    sigaction(SIGSEGV, &sigact, 0); // ошибка доступа к памяти
    sigaction(SIGBUS, &sigact, 0); // ошибка шины, при обращении к физической памяти

    sigemptyset(&sigset);

    // блокируем сигналы которые будем ожидать
    // сигнал остановки процесса пользователем
    sigaddset(&sigset, SIGQUIT);

    // сигнал для остановки процесса пользователем с терминала
    sigaddset(&sigset, SIGINT);

    // сигнал запроса завершения процесса
    sigaddset(&sigset, SIGTERM);

    // пользовательский сигнал который мы будем использовать для обновления конфига
    sigaddset(&sigset, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    // Установим максимальное кол-во дискрипторов которое можно открыть
    SetFdLimit(FD_LIMIT);

    // запишем в лог, что наш демон стартовал
    WriteLog("[DAEMON] Started\n");

    // запускаем все рабочие потоки
    status = InitWorkThread();
    if (status != -1) {
        // цикл ожижания сообщений
        for (; ;) {
            // ждем указанных сообщений
            sigwait(&sigset, &signo);

            // если то сообщение обновления конфига
            if (signo == SIGUSR1) {
                // обновим конфиг
                status = ReloadConfig();
                if (status == 0) {
                    WriteLog("[DAEMON] Reload config failed\n");
                }
                else {
                    WriteLog("[DAEMON] Reload config OK\n");
                }
            }
            else // если какой-либо другой сигнал, то выйдим из цикла
            {
                break;
            }
        }

        // остановим все рабочеи потоки и корректно закроем всё что надо
        DestroyWorkThread();
    }
    else {
        WriteLog("[DAEMON] Create work thread failed\n");
    }

    WriteLog("[DAEMON] Stopped\n");

    // вернем код не требующим перезапуска
    return CHILD_NEED_TERMINATE;
}

// функция для остановки потоков и освобождения ресурсов
void DestroyWorkThread() {
    // тут должен быть код который остановит все потоки и
    // корректно освободит ресурсы
}

// функция которая инициализирует рабочие потоки
int InitWorkThread() {
    pthread_t pthread_t1;
    void *arg;
    int server_fd = server_start_listen();
    if (server_fd == -1) {
        return -1;
    }
    arg = (void *) server_fd;
    pthread_create(&pthread_t1, NULL, work_tread, arg);
}

void *work_tread(void *arg) {
    long long sig1 = reinterpret_cast<long long> (arg);
    int server_fd =static_cast<int>(sig1);
    pthread_t pthread_t1;
    int rfd;
    for (; ;) {
        rfd = server_establish_connection(server_fd);
        arg = (void *) rfd;
        pthread_create(&pthread_t1, NULL, tcp_server_read, arg);
    }
}

int server_establish_connection(int server_fd)
// This function will establish a connection between the server and the
// client. It will be executed for every new client that connects to the server.
// This functions returns the socket filedescriptor for reading the clients data
// or an error if it failed.
{
    char ipstr[INET6_ADDRSTRLEN];
    int port;


    int new_sd;
    struct sockaddr_storage remote_info;
    socklen_t addr_size;

    addr_size = sizeof(addr_size);
    new_sd = accept(server_fd, (struct sockaddr *) &remote_info, &addr_size);

    getpeername(new_sd, (struct sockaddr *) &remote_info, &addr_size);

    // deal with both IPv4 and IPv6:
    if (remote_info.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *) &remote_info;
        port = ntohs(s->sin_port);
        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    } else { // AF_INET6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *) &remote_info;
        port = ntohs(s->sin6_port);
        inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
    }


    return new_sd;

}


int server_start_listen() {

    struct addrinfo hostinfo, *res;

    int sock_fd;

    int server_fd; // the fd the server listens on
    int ret;
    int yes = 1;

// first, load up address structs with getaddrinfo():

    memset(&hostinfo, 0, sizeof(hostinfo));

    hostinfo.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hostinfo.ai_socktype = SOCK_STREAM;
    hostinfo.ai_flags = AI_PASSIVE;     // fill in my IP for me

    getaddrinfo(NULL, "5555", &hostinfo, &res);


    server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    //if(server_fd < 0) throw some error;

    //prevent "Error Address already in use"
    ret = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    // if(ret < 0) throw some error;

    ret = bind(server_fd, res->ai_addr, res->ai_addrlen);

    if (ret != 0) {
        WriteLog("[DAEMON] bind port error\n");
        return -1;
    }

    ret = listen(server_fd, 5);
    if (ret == -1) {
        WriteLog("[DAEMON] listen error\n");
        return -1;
    }


    return server_fd;

}

void *tcp_server_read(void *arg)
/// This function runs in a thread for every client, and reads incomming data.
/// It also writes the incomming data to all other clients.

{
    int rfd;

    char buf[1024];
    int buflen;
    long long sig1 = reinterpret_cast<long long> (arg);
    rfd =static_cast<int>(sig1);
    for (; ;) {
        //read incomming message.
        buflen = recv(rfd, buf, sizeof(buf), 0);
        if (buflen <= 0) {
            close(rfd);
            pthread_exit(NULL);
        }
        // add the users FD to the message to give it an unique ID.
        send(rfd, buf, buflen, 0);
        if (strncmp("bye",buf,3)==0) {
            pthread_exit(NULL);
        }
    }

return NULL;
}

