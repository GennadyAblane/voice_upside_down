# Исправление настроек Run для Android

## Проблема: "Bad component name: /"

Эта ошибка возникает, когда Qt Creator не может определить имя пакета для запуска.

## Решение

### Вариант 1: Создайте APK вручную

1. **Build → Build Project** (Ctrl+B) - убедитесь, что сборка прошла успешно
2. **Build → Build Android APK** - создайте APK вручную
3. После создания APK настройки Run должны заполниться автоматически

### Вариант 2: Установите APK вручную через ADB

Если автоматический запуск не работает:

1. **Найдите APK:**
   - Обычно находится в: `build/Android_Qt_6_7_2_Clang_arm64_v8a-Release/android-build/build/outputs/apk/debug/voice_upside_down-debug.apk`

2. **Установите APK:**
   ```bash
   C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe install -r "D:\git\New Year. Turn your voice upside down\New Year. Turn your voice upside down\voice_upside_down\build\Android_Qt_6_7_2_Clang_arm64_v8a-Release\android-build\build\outputs\apk\debug\voice_upside_down-debug.apk"
   ```

3. **Запустите приложение:**
   ```bash
   C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe shell am start -n com.voiceupsidedown.master/org.qtproject.qt.android.bindings.QtActivity
   ```

### Вариант 3: Проверьте настройки Deployment

В разделе **Deployment** убедитесь, что:
- **Method:** "Deploy to Android Device"
- **Target device:** Ваше устройство (Xiaomi K60 Ultra)

### Вариант 4: Пересоздайте конфигурацию Run

1. В разделе **Run** нажмите **Remove** (удалить текущую конфигурацию)
2. Нажмите **Add...** (добавить новую)
3. Выберите "Android Application"
4. Укажите:
   - **Package name:** `com.voiceupsidedown.master`
   - **Application binary:** `voice_upside_down`

## Проверка APK

Убедитесь, что APK был создан:
- Проверьте папку: `build/Android_Qt_6_7_2_Clang_arm64_v8a-Release/android-build/build/outputs/apk/debug/`
- Должен быть файл: `voice_upside_down-debug.apk`

Если APK нет:
1. **Build → Clean**
2. **Build → Build Project**
3. **Build → Build Android APK**

