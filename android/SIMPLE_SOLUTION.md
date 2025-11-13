# Простое решение: Создание APK без Qt Creator

Поскольку `androiddeployqt` из `mingw_64` не работает, а Qt Creator не может автоматически определить настройки, используйте следующий подход:

## Шаг 1: Проверьте наличие androiddeployqt в Android Kit

```powershell
Test-Path "C:\Qt\6.7.2\android_arm64_v8a\bin\androiddeployqt.exe"
```

Если файл **НЕ найден**, нужно установить Android Tools:
1. Откройте **Qt Maintenance Tool**
2. Выберите **Qt 6.7.2**
3. Установите **Android Tools** (если не установлены)

## Шаг 2: Используйте androiddeployqt из Android Kit

Если файл найден, запустите:

```powershell
cd "D:\git\New Year. Turn your voice upside down\New Year. Turn your voice upside down\voice_upside_down\build\Android_Qt_6_7_2_Clang_arm64_v8a-Release"

# Используйте скрипт, который ищет androiddeployqt в Android Kit
.\use_android_kit_deployqt.bat
```

Или напрямую:

```powershell
& "C:\Qt\6.7.2\android_arm64_v8a\bin\androiddeployqt.exe" --input "android-build\android-deployment-settings.json" --output "android-build" --android-platform android-33 --jdk "C:\Program Files\Java\jdk-21" --gradle
```

## Шаг 3: Установите APK вручную

После создания APK:

```powershell
# Установите APK
& "C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe" install -r "android-build\build\outputs\apk\debug\voice_upside_down-debug.apk"

# Запустите приложение
& "C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe" shell am start -n com.voiceupsidedown.master/org.qtproject.qt.android.bindings.QtActivity
```

## Если androiddeployqt не найден в Android Kit

Установите Android Tools через Qt Maintenance Tool:
1. Запустите Qt Maintenance Tool
2. Выберите **Add or remove components**
3. Выберите **Qt 6.7.2 → Android → Android Tools**
4. Установите компоненты

