# Установка Android Tools для Qt 6.7.2

## Проблема

`androiddeployqt.exe` отсутствует в Android Kit, что не позволяет создать APK.

## Решение: Установить Android Tools

### Шаг 1: Откройте Qt Maintenance Tool

1. Найдите **Qt Maintenance Tool** в меню Пуск или в папке установки Qt
2. Запустите его от имени администратора (правой кнопкой → "Запуск от имени администратора")

### Шаг 2: Добавьте Android Tools

1. В Qt Maintenance Tool выберите **Add or remove components**
2. Найдите **Qt 6.7.2** в списке
3. Разверните **Android**
4. Найдите и установите:
   - **Android Tools** (обязательно)
   - **Android arm64-v8a** (если не установлен)

### Шаг 3: Проверьте установку

После установки проверьте:

```powershell
Test-Path "C:\Qt\6.7.2\android_arm64_v8a\bin\androiddeployqt.exe"
```

Должно вернуть `True`.

### Шаг 4: Пересоберите проект

После установки Android Tools:
1. В Qt Creator: **Build → Clean**
2. **Build → Build Project** (Ctrl+B)
3. Попробуйте создать APK снова

## Альтернатива: Использовать CMake с Gradle напрямую

Если установка Android Tools не помогает, можно создать APK через CMake/Gradle напрямую (см. `ALTERNATIVE_APK.md`).

