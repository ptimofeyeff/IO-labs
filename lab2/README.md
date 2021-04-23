# Лабораторная работа 2

**Название:** "Разработка драйверов блочных устройств"

**Цель работы:** Получить знания и навыки разработки драйверов блочных устройств для операционной системы Linux.

## Описание функциональности драйвера

    Драйвер создает логический жесткий диск размером 50 Мбайт.
    Драйвер разбивает логический жесткий диск на
        1. первичный раздел размером 30 Мбайт (mydisk1)
        2. расширенный раздел размером 20 Мбайт разбитый на
            1. логический раздел размером 10 Мбайт (mydisk5)
            2. логический раздел размером 10 Мбайт (mydisk6)

## Инструкция по сборке

1. Для сборки установочных файлов драйвера необходимо выполнить команду 
   из директории `/lab2`
```
# make Makefile
```
2. Для установки драйвера после сборки необходимо выполнить команду
```
# sudo insmod my_blk_dev.ko
```
3. Для удаления драйвера и самого блочного устройства необходимо выполнить команду
```
# sudo rmmod my_blk_dev
```
4. Для удаления всех установочных файлов необходимо выполнить команду
```
# make Makefile clean
```

## Инструкция пользователя

Для взаимодействие с разделом диска через файловую систему Linux необходимо выполнить 
следующие действия, авторизировавшись под пользователем с ***root-правами***.

1. Создать в раделе файловую систему необходимого типа с помощью команды **mkfs**
```
# mkfs -t [fstype] /dev/mydisk1
```
*`fstype` - тип файловой системы.*

2. Создать директорию для доступа к файловой системе раздела 
   с помощью команды **mkdir**
```
# mkdir /mnt/mydisk1
```
3. Смонтировать раздел диска в созданную директории с помощью команды **mount**
```
# mount /dev/mydisk1 /mnt/mydisk1
```

Для просмотра структуры диска необходимо использовать утилиты для просмотра информации 
о дисках. Одной из таких утилит является утилита **fdisk**
```
# sudo fdisk -l /dev/mydisk
```

---

Для просмотра кольцевого буфера ядра, в котором происходит логирование операций
чтения/запись в блочное устройство *mydisk*, необходимо использовать команду 
```
dmesg
```

---

## Примеры использования
**Установка драйвера**
```
root# sudo insmod my_blk_dev.ko
root# dmesg 
[  209.668376] Major Number is : 252
[  209.670368] THIS IS DEVICE SIZE 102400
[  209.671897] mydiskdrive : open 
[  209.671933] my disk: Start Sector: 0, Sector Offset: 0;		Buffer: 000000002c671a74; Length: 8 sectors
[  209.671946] my disk: Start Sector: 61440, Sector Offset: 0;		Buffer: 00000000d455ee7c; Length: 8 sectors
[  209.671951] my disk: Start Sector: 81920, Sector Offset: 0;		Buffer: 000000001a309dcb; Length: 8 sectors
[  209.671954]  mydisk: mydisk1 mydisk2 < mydisk5 mydisk6 >
[  209.673215] mydisk: p6 size 20480 extends beyond EOD, truncated
[  209.673251] mydiskdrive : closed 
[  209.673726] mydiskdrive : open 
[  209.674478] mydiskdrive : closed 
[  209.685529] mydiskdrive : open 
[  209.686255] mydiskdrive : closed 
[  209.688847] mydiskdrive : open 
[  209.689565] mydiskdrive : closed 
[  209.690112] mydiskdrive : open 
[  209.690815] mydiskdrive : closed 
[  209.693062] mydiskdrive : open 
[  209.699778] mydiskdrive : closed 
```
**Просмотр информации о диске**
```
root# sudo fdisk -l /dev/mydisk
Disk /dev/mydisk: 50 MiB, 52428800 bytes, 102400 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0x36e5756d

Device       Boot Start    End Sectors Size Id Type
/dev/mydisk1          1  61440   61440  30M 83 Linux
/dev/mydisk2      61441 102400   40960  20M  5 Extended
/dev/mydisk5      61442  81921   20480  10M 83 Linux
/dev/mydisk6      81923 102402   20480  10M 83 Linux

root# dmesg 
[ 3332.351087] mydiskdrive : open 
[ 3332.351139] my disk: Start Sector: 0, Sector Offset: 0;		Buffer: 000000005f352385; Length: 8 sectors
[ 3332.351160] my disk: Start Sector: 61440, Sector Offset: 0;		Buffer: 00000000e751b6c2; Length: 8 sectors
[ 3332.351174] my disk: Start Sector: 81920, Sector Offset: 0;		Buffer: 00000000b35b8287; Length: 8 sectors
[ 3332.352038] mydiskdrive : closed 
```
## Профилирование

Копирование между разделами виртуального диска (из /dev/mydisk1 в /dev/mydisk5) -- 15.2 MB/s:
```shell
# dd if=/dev/mydisk1 of=/dev/mydisk5 bs=512 count=20479 oflag=direct
20479+0 records in
20479+0 records out
10485248 bytes (10 MB, 10 MiB) copied, 0.690898 s, 15.2 MB/s
```

Копирование из реального диска в виртуальный диск (из /dev/sda1 в /dev/mydisk5) -- 16.2 MB/s:
```shell
# dd if=/dev/sda1 of=/dev/mydisk5 bs=512 count=20479 oflag=direct
20479+0 records in
20479+0 records out
10485248 bytes (10 MB, 10 MiB) copied, 0.64731 s, 16.2 MB/s