//
// Created by wlad on 7/15/15.
//
#include <syslog.h>
#include "logger.h"
// функция записи лога
void WriteLog(const char* Msg, ...)
{
    // тут должен быть код который пишет данные в лог
    openlog("my_daemon", LOG_PID|LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "%s", Msg);
    closelog();
}