# Лабораторная работа 3

**Название:** "Разработка драйверов сетевых устройств"

**Цель работы:** Получить знания и навыки разработки драйверов сетевых интерфейсов для операционной системы Linux

## Описание функциональности драйвера

Драйвер сетевого интерфейса перехватывает пакеты протокола UDP, адресуемые на конкретный порт. Вывести порт отправителя и получателя. Номер порта определяется исполнителями. Состояние разбора пакетов необходимо выводить в файл в директории /proc.

## Инструкция по сборке

Собирается с помощью команды ```make```.

## Инструкция пользователя

Для запуска драйвера необходимо ввести команду ```insmod net_driver.ko```, которая загружает модуль ядра в систему.
Для выгрузки модуля драйвера нужно ввести команду ```sudo rmmod net_driver```.

## Примеры использования

### Отправка пакетов UDP на нужный порт с помощью nping

```bash
root@mint:/home/mint/IO-labs/lab3# nping -N -udp -p 8081 127.0.0.1

Starting Nping 0.7.60 ( https://nmap.org/nping ) at 2021-04-29 02:58 PDT
SENT (0.0037s) UDP packet with 4 bytes to 127.0.0.1:8081
SENT (1.0057s) UDP packet with 4 bytes to 127.0.0.1:8081
SENT (2.0067s) UDP packet with 4 bytes to 127.0.0.1:8081
SENT (3.0080s) UDP packet with 4 bytes to 127.0.0.1:8081
Nping done: 1 IP address pinged in 4.01 seconds
```

### Вывод содержимого proc файла

```
root@mint:/home/mint/IO-labs/lab3# cat /proc/var3
Packets: 5; Bytes: 160
```

### Вывод кольцевого буфера

```
[ 3099.360118] Module net_driver unloaded
[ 3135.897728] net_driver: proc file is created
[ 3135.910987] Module net_driver loaded
[ 3135.910989] net_driver: create link vni0
[ 3135.910991] net_driver: registered rx handler for lo
[ 3135.946263] IPv6: ADDRCONF(NETDEV_UP): vni0: link is not ready
[ 3135.946313] vni0: device opened
[ 3140.512208] Source port: 53; Destination port: 8081
[ 3141.514423] Source port: 53; Destination port: 8081
[ 3142.515441] Source port: 53; Destination port: 8081
[ 3143.516809] Source port: 53; Destination port: 8081
[ 3144.518621] Source port: 53; Destination port: 8081