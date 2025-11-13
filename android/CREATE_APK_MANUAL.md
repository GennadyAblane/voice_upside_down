# Создание APK вручную для Qt 6.7.2

## Шаги

### 1. Подготовка файлов

Убедитесь, что:
- Исполняемый файл собран: `build/Android_Qt_6_7_2_Clang_arm64_v8a-Release/src/voice_upside_down`
- JSON файл создан: `build/Android_Qt_6_7_2_Clang_arm64_v8a-Release/android-build/android-deployment-settings.json`

### 2. Скопируйте исполняемый файл

В PowerShell:
```powershell
cd "D:\git\New Year. Turn your voice upside down\New Year. Turn your voice upside down\voice_upside_down\build\Android_Qt_6_7_2_Clang_arm64_v8a-Release"

# Создайте папку
New-Item -ItemType Directory -Force -Path "android-build\libs\arm64-v8a"

# Скопируйте файл
Copy-Item "src\voice_upside_down" -Destination "android-build\libs\arm64-v8a\libvoice_upside_down_arm64-v8a.so" -Force
```

### 3. Запустите androiddeployqt

```powershell
& "C:\Qt\6.7.2\android_arm64_v8a\bin\androiddeployqt.exe" --input "android-build\android-deployment-settings.json" --output "android-build" --android-platform android-33 --jdk "C:\Program Files\Java\jdk-21" --gradle
```

### 4. Проверьте APK

APK должен быть в:
- `android-build/build/outputs/apk/debug/voice_upside_down-debug.apk`

### 5. Установите APK

```powershell
& "C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe" install -r "android-build\build\outputs\apk\debug\voice_upside_down-debug.apk"
```

### 6. Запустите приложение

```powershell
& "C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe" shell am start -n com.voiceupsidedown.master/org.qtproject.qt.android.bindings.QtActivity
```

