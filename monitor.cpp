//
// Created by wlad on 7/15/15.
//
#include "monitor.h"
#include "logger.h"
#include "options.h"
#include "work.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

int MonitorProc()
{
    pid_t pid, sid;
    int       status;
    int       need_start = 1;
    sigset_t  sigset;
    siginfo_t siginfo;



    // данный код уже выполняется в процессе потомка
    // разрешаем выставлять все биты прав на создаваемые файлы,
    // иначе у нас могут быть проблемы с правами доступа
    umask(0);

    // создаём новый сеанс, чтобы не зависеть от родителя
    sid = setsid();
    if (sid < 0) {
        /* Log any failure here */
        exit(EXIT_FAILURE);
    }
    // переходим в корень диска, если мы этого не сделаем, то могут быть проблемы.
    // к примеру с размантированием дисков
    if ((chdir("/")) < 0) {
        /* Log any failure here */
        exit(EXIT_FAILURE);
    }

    // закрываем дискрипторы ввода/вывода/ошибок, так как нам они больше не понадобятся
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);


    // настраиваем сигналы которые будем обрабатывать
    sigemptyset(&sigset);

    // сигнал остановки процесса пользователем
    sigaddset(&sigset, SIGQUIT);

    // сигнал для остановки процесса пользователем с терминала
    sigaddset(&sigset, SIGINT);

    // сигнал запроса завершения процесса
    sigaddset(&sigset, SIGTERM);

    // сигнал посылаемый при изменении статуса дочернего процесс
    sigaddset(&sigset, SIGCHLD);

    // сигнал посылаемый при изменении статуса дочернего процесс
    sigaddset(&sigset, SIGCHLD);

    // пользовательский сигнал который мы будем использовать для обновления конфига
    sigaddset(&sigset, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    // данная функция создат файл с нашим PID'ом
    SetPidFile(PID_FILE);

    // бесконечный цикл работы
    for (;;)
    {
        // если необходимо создать потомка
        if (need_start)
        {
            // создаём потомка
            pid = fork();
        }

        need_start = 1;

        if (pid == -1) // если произошла ошибка
        {
            // запишем в лог сообщение об этом
            WriteLog("[MONITOR] Fork failed (%s)\n", strerror(errno));
        }
        else if (!pid) // если мы потомок
        {
            // данный код выполняется в потомке

            // запустим функцию отвечающую за работу демона
            status = WorkProc();

            // завершим процесс
            exit(status);
        }
        else // если мы родитель
        {
            // данный код выполняется в родителе

            // ожидаем поступление сигнала
            sigwaitinfo(&sigset, &siginfo);

            // если пришел сигнал от потомка
            if (siginfo.si_signo == SIGCHLD)
            {
                // получаем статус завершение
                wait(&status);

                // преобразуем статус в нормальный вид
                status = WEXITSTATUS(status);

                // если потомок завершил работу с кодом говорящем о том, что нет нужны дальше работать
                if (status == CHILD_NEED_TERMINATE)
                {
                    // запишем в лог сообщени об этом
                    WriteLog("[MONITOR] Childer stopped\n");

                    // прервем цикл
                    break;
                }
                else if (status == CHILD_NEED_WORK) // если требуется перезапустить потомка
                {
                    // запишем в лог данное событие
                    WriteLog("[MONITOR] Childer restart\n");
                }
            }
            else if (siginfo.si_signo == SIGUSR1) // если пришел сигнал что необходимо перезагрузить конфиг
            {
                kill(pid, SIGUSR1); // перешлем его потомку
                need_start = 0; // установим флаг что нам не надо запускать потомка заново
            }
            else // если пришел какой-либо другой ожидаемый сигнал
            {
                // запишем в лог информацию о пришедшем сигнале
                WriteLog("[MONITOR] Signal %s\n", strsignal(siginfo.si_signo));

                // убьем потомка
                kill(pid, SIGTERM);
                status = 0;
                break;
            }
        }
    }

    // запишем в лог, что мы остановились
    WriteLog("[MONITOR] Stopped\n");

    // удалим файл с PID'ом
    unlink(PID_FILE);

    return status;
}
