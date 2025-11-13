# Альтернативные способы создания APK для Qt 6.7.2

## Проблема

`androiddeployqt` из `mingw_64` может не работать корректно с Android Kit. В Qt 6.7.2 для Android рекомендуется использовать CMake напрямую с Gradle.

## Решение 1: Использовать Qt Creator

1. Откройте проект в Qt Creator
2. Выберите Kit: **Android Qt 6.7.2 Clang arm64-v8a**
3. Нажмите **Run (F5)** - Qt Creator автоматически создаст и установит APK

## Решение 2: Использовать CMake напрямую

Если `androiddeployqt` не работает, можно использовать CMake с Gradle:

### Шаг 1: Убедитесь, что Gradle установлен

```powershell
gradle --version
```

Если Gradle не установлен, установите его:
- Скачайте с https://gradle.org/releases/
- Или используйте Gradle Wrapper (автоматически загружается при сборке)

### Шаг 2: Создайте Android проект через CMake

CMake должен автоматически создать Gradle проект в `android-build`. Проверьте наличие:
- `android-build/build.gradle`
- `android-build/app/build.gradle`

### Шаг 3: Соберите APK через Gradle

```powershell
cd "D:\git\New Year. Turn your voice upside down\New Year. Turn your voice upside down\voice_upside_down\build\Android_Qt_6_7_2_Clang_arm64_v8a-Release\android-build"

# Если Gradle установлен глобально
gradle assembleDebug

# Или используйте Gradle Wrapper (если есть)
.\gradlew assembleDebug
```

APK будет в: `android-build/app/build/outputs/apk/debug/app-debug.apk`

## Решение 3: Проверить наличие androiddeployqt в Android Kit

Возможно, `androiddeployqt` должен быть в Android Kit, но не установлен. Проверьте:

```powershell
Get-ChildItem "C:\Qt\6.7.2" -Recurse -Filter "androiddeployqt.exe" -ErrorAction SilentlyContinue
```

Если файл не найден, возможно, нужно установить дополнительные компоненты Qt через Qt Maintenance Tool:
- Qt 6.7.2 → Android → Android Tools

## Решение 4: Использовать Qt 5.15.2 для Android

Если проблемы продолжаются, можно временно использовать Qt 5.15.2 для Android, где `androiddeployqt` работает стабильнее.

