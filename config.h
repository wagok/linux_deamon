//
// Created by wlad on 7/15/15.
//

#ifndef LINUX_DEAMON_CONFIG_H
#define LINUX_DEAMON_CONFIG_H
// функция загрузки конфига
int LoadConfig(char* FileName);

// функция которая загрузит конфиг заново
// и внесет нужные поправки в работу
int ReloadConfig();

#endif //LINUX_DEAMON_CONFIG_H
