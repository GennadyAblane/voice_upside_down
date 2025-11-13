# Создание APK для Qt 6.7.2

## Проблема

В Qt 6.7.2 `qt_android_add_apk_target` может быть недоступен, поэтому нужно создавать APK вручную через `androiddeployqt`.

## Решение

### Вариант 1: Использовать готовый скрипт

1. **Перейдите в папку сборки:**
   ```
   cd "D:\git\New Year. Turn your voice upside down\New Year. Turn your voice upside down\voice_upside_down\build\Android_Qt_6_7_2_Clang_arm64_v8a-Release"
   ```

2. **Запустите скрипт:**
   ```
   create_apk_qt6.bat
   ```

### Вариант 2: Создать APK вручную

1. **Создайте папку android-build:**
   ```bash
   mkdir android-build\libs\arm64-v8a
   ```

2. **Скопируйте исполняемый файл:**
   ```bash
   copy src\voice_upside_down android-build\libs\arm64-v8a\libvoice_upside_down_arm64-v8a.so
   ```

3. **Создайте android-deployment-settings.json** (см. пример ниже)

4. **Запустите androiddeployqt:**
   ```bash
   C:\Qt\6.7.2\android_arm64_v8a\bin\androiddeployqt.exe --input android-build\android-deployment-settings.json --output android-build --android-platform android-33 --jdk "C:\Program Files\Java\jdk-21" --gradle
   ```

### Пример android-deployment-settings.json

```json
{
  "description": "This file is created by CMake to be read by androiddeployqt and should not be modified by hand.",
  "qt": "C:/Qt/6.7.2/android_arm64_v8a",
  "sdk": "C:/Users/User/AppData/Local/Android/Sdk",
  "ndk": "C:/Users/User/AppData/Local/Android/Sdk/ndk/25.2.9519653",
  "ndk-host": "windows-x86_64",
  "target-architecture": "arm64-v8a",
  "application-binary": "libvoice_upside_down_arm64-v8a.so",
  "android-package-source-directory": "D:/git/New Year. Turn your voice upside down/New Year. Turn your voice upside down/voice_upside_down/android",
  "android-package": "com.voiceupsidedown.master",
  "android-min-sdk-version": "23",
  "android-target-sdk-version": "33",
  "stdcpp-path": "C:/Users/User/AppData/Local/Android/Sdk/ndk/25.2.9519653/sources/cxx-stl/llvm-libc++/libs/arm64-v8a",
  "tool-prefix": "llvm",
  "toolchain-prefix": "llvm",
  "useLLVM": true
}
```

## После создания APK

APK будет в:
- `build/Android_Qt_6_7_2_Clang_arm64_v8a-Release/android-build/build/outputs/apk/debug/voice_upside_down-debug.apk`

Установите APK:
```bash
C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe install -r "путь\к\voice_upside_down-debug.apk"
```

Запустите приложение:
```bash
C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe shell am start -n com.voiceupsidedown.master/org.qtproject.qt.android.bindings.QtActivity
```

