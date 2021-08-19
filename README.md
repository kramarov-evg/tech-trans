# Определение направления движения объекта
## Инструкция для запуска
Предполагается, что система, на которой будет запускаться программа удовлетворяет условиям:

- На ней установлен Docker ([Инструкции по установке на сайте Docker'а](https://docs.docker.com/get-docker/));
- Используется сервер X11.

Для того, чтобы запустить приложение необходимо:
1. Склонировать/скачать+распаковать этот репозиторий (`git clone ...`) и перейти в директорию с файлами репозитория;
2. Собрать образ докера: `docker build -t <имя образа на ваше усмотрение> .` (процесс может занять некоторое время);
3. Выполнить `xhost +`. Необходимо, чтобы докер-образ смог получить доступ к дисплею системы и вывести результаты в окне.
4. Запустить докер-образ: `docker run -e DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix:ro -v <Полный путь к видео для обраотки на хост системе>:/app_build/video.mov <имя образа из пункта 2> video.mov`