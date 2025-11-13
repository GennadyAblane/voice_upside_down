# Создание APK через Qt Creator (без androiddeployqt)

## Способ 1: Использовать Run (F5)

1. **Убедитесь, что устройство подключено:**
   ```powershell
   & "C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe" devices
   ```

2. **В Qt Creator:**
   - Выберите Kit: **Android Qt 6.7.2 Clang arm64-v8a**
   - Нажмите **Run (F5)**
   - Qt Creator должен автоматически:
     - Создать APK
     - Установить на устройство
     - Запустить приложение

## Способ 2: Build Android APK через меню

1. **Build → Build Project** (Ctrl+B) - убедитесь, что проект собран
2. **Build → Build Android APK** - если такой пункт есть в меню

## Способ 3: Проверить настройки Deployment

1. **Projects → Run**
2. В разделе **Deployment** проверьте:
   - **Method:** "Deploy to Android Device"
   - **Target device:** Ваше устройство

## Способ 4: Использовать CMake напрямую

Если Qt Creator не может создать APK автоматически, можно попробовать использовать CMake для генерации Gradle проекта:

1. Убедитесь, что проект собран
2. Проверьте наличие `android-build/build.gradle`
3. Если есть, используйте Gradle для сборки APK

## Важно

В Qt 6.7.2 для Android может не быть отдельного `androiddeployqt`, но Qt Creator должен использовать встроенные механизмы для создания APK.

