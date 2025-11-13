# Быстрое создание APK для Qt 6.7.2

## Способ 1: Использовать готовый скрипт (рекомендуется)

1. **Откройте командную строку (cmd.exe) или PowerShell**

2. **Перейдите в папку сборки:**
   ```cmd
   cd "D:\git\New Year. Turn your voice upside down\New Year. Turn your voice upside down\voice_upside_down\build\Android_Qt_6_7_2_Clang_arm64_v8a-Release"
   ```

3. **Запустите скрипт:**
   - В **cmd.exe**: `create_apk_qt6.bat`
   - В **PowerShell**: `.\create_apk_qt6.bat`

## Способ 2: Выполнить команды вручную

### Шаг 1: Подготовка

```powershell
cd "D:\git\New Year. Turn your voice upside down\New Year. Turn your voice upside down\voice_upside_down\build\Android_Qt_6_7_2_Clang_arm64_v8a-Release"

# Создайте папку
New-Item -ItemType Directory -Force -Path "android-build\libs\arm64-v8a"

# Скопируйте файл
Copy-Item "src\voice_upside_down" -Destination "android-build\libs\arm64-v8a\libvoice_upside_down_arm64-v8a.so" -Force
```

### Шаг 2: Создание APK

```powershell
& "C:\Qt\6.7.2\android_arm64_v8a\bin\androiddeployqt.exe" --input "android-build\android-deployment-settings.json" --output "android-build" --android-platform android-33 --jdk "C:\Program Files\Java\jdk-21" --gradle
```

### Шаг 3: Установка APK

```powershell
& "C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe" install -r "android-build\build\outputs\apk\debug\voice_upside_down-debug.apk"
```

### Шаг 4: Запуск приложения

```powershell
& "C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe" shell am start -n com.voiceupsidedown.master/org.qtproject.qt.android.bindings.QtActivity
```

## Где находится APK?

После успешного создания APK будет в:
- `build/Android_Qt_6_7_2_Clang_arm64_v8a-Release/android-build/build/outputs/apk/debug/voice_upside_down-debug.apk`

