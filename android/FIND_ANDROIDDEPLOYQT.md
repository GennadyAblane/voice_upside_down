# Поиск androiddeployqt.exe для Qt 6.7.2

## Проблема

`androiddeployqt.exe` может находиться в разных местах в зависимости от установки Qt.

## Возможные расположения

1. **В Android Kit:**
   - `C:\Qt\6.7.2\android_arm64_v8a\bin\androiddeployqt.exe`

2. **В Desktop Kit (может быть доступен):**
   - `C:\Qt\6.7.2\mingw_64\bin\androiddeployqt.exe`
   - `C:\Qt\6.7.2\msvc2019_64\bin\androiddeployqt.exe`

## Как проверить

В PowerShell выполните:

```powershell
# Проверка в Android Kit
Test-Path "C:\Qt\6.7.2\android_arm64_v8a\bin\androiddeployqt.exe"

# Проверка в Desktop Kits
Test-Path "C:\Qt\6.7.2\mingw_64\bin\androiddeployqt.exe"
Test-Path "C:\Qt\6.7.2\msvc2019_64\bin\androiddeployqt.exe"
```

## Альтернатива: Использовать CMake

Если `androiddeployqt` недоступен, можно попробовать использовать CMake напрямую с Gradle:

1. Убедитесь, что Gradle установлен
2. Используйте CMake для генерации Android проекта
3. Соберите APK через Gradle

## Решение

Обновленный скрипт `create_apk_qt6.bat` теперь автоматически ищет `androiddeployqt.exe` в нескольких местах.

