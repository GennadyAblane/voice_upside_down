# Установка Qt6Multimedia для Android

## Проблема

При сборке проекта для Android появляется ошибка:
```
Could NOT find Qt6Multimedia (missing: Qt6Multimedia_DIR)
```

Это означает, что модуль Qt6Multimedia не установлен для Android.

## Решение: Установка Qt6Multimedia

### Шаг 1: Откройте Qt Maintenance Tool

1. Найдите **Qt Maintenance Tool** в меню Пуск или в папке установки Qt
2. Запустите его (может потребоваться права администратора)

### Шаг 2: Добавьте Qt6Multimedia для Android

1. В Qt Maintenance Tool выберите **Add or remove components**
2. Найдите **Qt 6.7.2** в списке
3. Разверните **Android**
4. Разверните **Android arm64-v8a**
5. Найдите и отметьте **Qt Multimedia**
6. Нажмите **Next** и следуйте инструкциям для установки

### Шаг 3: Проверка установки

После установки проверьте, что файл существует:
```
C:\Qt\6.7.2\android_arm64_v8a\lib\cmake\Qt6Multimedia\Qt6MultimediaConfig.cmake
```

### Шаг 4: Пересоберите проект

1. В Qt Creator: **Build → Clean**
2. **Build → Run CMake** (или **Configure Project**)
3. **Build → Build Project** (Ctrl+B)

## Альтернатива: Использование Qt 5.15.2

Если установка Qt6Multimedia вызывает проблемы, можно использовать Qt 5.15.2 для Android:

1. В Qt Creator: **Projects → Manage Kits**
2. Выберите Kit: **"Android arm64-v8a (Qt 5.15.2)"**
3. Нажмите **Configure Project**

Qt 5.15.2 для Android обычно включает Multimedia по умолчанию.

## Примечание

Приложение использует аудио функции (QAudioOutput, QAudioInput), поэтому Qt Multimedia необходим для работы приложения на Android.

