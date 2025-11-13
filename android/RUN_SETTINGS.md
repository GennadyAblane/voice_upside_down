# Настройка запуска Android приложения в Qt Creator

## Проблема: "Bad component name: /"

Эта ошибка возникает, когда Qt Creator не может определить имя пакета для запуска.

## Решение

### 1. Проверьте настройки Run в Qt Creator

1. **Projects → Run Settings**
2. Убедитесь, что указаны:
   - **Package name**: `com.voiceupsidedown.master`
   - **Application binary**: `voice_upside_down`
   - **Target device**: Ваше Android устройство

### 2. Создайте APK вручную

Если APK не создался автоматически:

1. **Build → Build Project** (Ctrl+B) - убедитесь, что сборка прошла успешно
2. **Build → Build Android APK** - создайте APK вручную

### 3. Установите APK вручную через ADB

Если автоматический запуск не работает:

```bash
# Найдите APK в папке сборки
# Обычно: build/Android_Qt_6_7_2_Clang_arm64_v8a-Release/android-build/build/outputs/apk/debug/voice_upside_down-debug.apk

# Установите APK
C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe install -r "путь\к\voice_upside_down-debug.apk"

# Запустите приложение
C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe shell am start -n com.voiceupsidedown.master/org.qtproject.qt.android.bindings.QtActivity
```

### 4. Проверьте, что APK создан

APK должен быть в:
- `build/Android_Qt_6_7_2_Clang_arm64_v8a-Release/android-build/build/outputs/apk/debug/voice_upside_down-debug.apk`

Если APK нет, попробуйте:
1. **Build → Clean**
2. **Build → Build Project**
3. **Build → Build Android APK**

