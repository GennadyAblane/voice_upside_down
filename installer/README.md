# Инструкция по созданию установщика

Этот установщик создан с помощью Qt Installer Framework и включает все необходимые DLL библиотеки для запуска приложения на "голом компьютере".

## Требования

1. **Qt 5.15.2** (или совместимая версия) с установленным Qt Installer Framework
2. **Собранное приложение** в Release режиме
3. **Windows** (для сборки установщика)

## Шаги по созданию установщика

### Шаг 1: Подготовка файлов

Запустите скрипт `prepare_installer.bat` для автоматического сбора всех необходимых DLL:

```batch
prepare_installer.bat [путь_к_qt] [путь_к_сборке]
```

**Пример:**
```batch
prepare_installer.bat C:\Qt\5.15.2\msvc2019_64 build\Desktop_Qt_5_15_2_MSVC2019_64bit-Release\src
```

**Параметры по умолчанию:**
- Qt Directory: `C:\Qt\5.15.2\msvc2019_64`
- Build Directory: `build\Desktop_Qt_5_15_2_MSVC2019_64bit-Release\src`

Скрипт выполнит:
- Копирование исполняемого файла `voice_upside_down.exe`
- Автоматический сбор всех необходимых Qt DLL через `windeployqt`
- Копирование QML файлов и ресурсов
- Проверку наличия всех зависимостей

### Шаг 2: Сборка установщика

После подготовки файлов запустите скрипт `build_installer.bat`:

```batch
build_installer.bat [путь_к_qt_installer_framework]
```

**Пример:**
```batch
build_installer.bat C:\Qt\Tools\QtInstallerFramework\3.2
```

**Параметр по умолчанию:**
- Qt Installer Framework: `C:\Qt\Tools\QtInstallerFramework\3.2`

В результате будет создан файл `VoiceUpsideDown_Installer.exe` в корневой директории проекта.

## Структура установщика

```
installer/
├── config/
│   └── config.xml          # Конфигурация установщика
├── packages/
│   └── com.voiceupsidedown.app/
│       ├── package.xml     # Описание пакета
│       ├── installscript.qs # Скрипт установки
│       └── data/           # Файлы приложения (создается автоматически)
│           ├── voice_upside_down.exe
│           ├── *.dll       # Все необходимые DLL
│           └── ...
├── prepare_installer.bat   # Скрипт подготовки файлов
└── build_installer.bat      # Скрипт сборки установщика
```

## Что включает установщик

Установщик автоматически включает:
- Исполняемый файл `voice_upside_down.exe`
- Все необходимые Qt DLL (Qt5Core, Qt5Gui, Qt5Qml, Qt5Quick, Qt5Multimedia и др.)
- QML модули и плагины
- Visual C++ Runtime DLL (если найдены)
- Все ресурсы приложения

## Установка Visual C++ Redistributable

Если приложение использует MSVC компилятор, пользователям может потребоваться установить **Visual C++ Redistributable** отдельно. Это можно сделать:

1. Автоматически через установщик (требует дополнительной настройки)
2. Вручную пользователем: скачать с сайта Microsoft

## Альтернативный вариант: Inno Setup

Если Qt Installer Framework недоступен, можно использовать **Inno Setup** (бесплатный и более простой в использовании).

### Требования для Inno Setup
- **Inno Setup Compiler** (скачать с https://jrsoftware.org/isdl.php)
- Qt 5.15.2 с windeployqt

### Шаги для Inno Setup

1. **Подготовка файлов:**
   ```batch
   prepare_for_inno.bat [путь_к_qt] [путь_к_сборке]
   ```
   Этот скрипт запустит `windeployqt` для сбора всех DLL в директорию сборки.

2. **Сборка установщика:**
   - Откройте файл `installer/VoiceUpsideDown.iss` в Inno Setup Compiler
   - Нажмите Build -> Compile (или F9)
   
   Или через командную строку:
   ```batch
   "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\VoiceUpsideDown.iss
   ```

3. **Результат:** Файл `VoiceUpsideDown_Setup.exe` будет создан в корневой директории проекта.

### Преимущества Inno Setup
- Не требует Qt Installer Framework
- Более простой в использовании
- Меньше зависимостей
- Хорошая поддержка русского языка

## Проверка установщика

После создания установщика рекомендуется:
1. Протестировать установку на чистой виртуальной машине Windows
2. Проверить, что приложение запускается без ошибок
3. Убедиться, что все функции работают корректно

## Решение проблем

### Ошибка: "windeployqt не найден"
- Убедитесь, что путь к Qt указан правильно
- Проверьте, что Qt установлен полностью

### Ошибка: "binarycreator не найден"
- Установите Qt Installer Framework через Qt Maintenance Tool
- Укажите правильный путь к Qt Installer Framework

### Приложение не запускается после установки
- Проверьте, что все DLL скопированы (особенно MSVC runtime)
- Установите Visual C++ Redistributable вручную
- Проверьте логи ошибок Windows Event Viewer

