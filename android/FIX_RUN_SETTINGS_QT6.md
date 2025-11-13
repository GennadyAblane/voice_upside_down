# Исправление ошибки "Bad component name: /" для Qt 6.7.2

## Проблема

При попытке запустить Android приложение через Qt Creator возникает ошибка:
```
Activity Manager error: Exception occurred while executing 'start':
java.lang.IllegalArgumentException: Bad component name: /
```

Это означает, что Qt Creator не может определить правильное имя пакета для запуска.

## Решение

### Шаг 1: Проверьте настройки Run в Qt Creator

1. Откройте **Projects** (Ctrl+5)
2. Выберите Kit: **Android Qt 6.7.2 Clang arm64-v8a**
3. Перейдите на вкладку **Run**
4. В разделе **Android** проверьте следующие настройки:

   - **Package name**: `com.voiceupsidedown.master`
   - **Activity name**: `org.qtproject.qt.android.bindings.QtActivity`
   - **Application binary**: `voice_upside_down`
   - **Target device**: Ваше Android устройство (Xiaomi K60 Ultra)

### Шаг 2: Если настройки пустые или неправильные

Если поля "Package name" или "Activity name" пустые:

1. **Убедитесь, что проект собран:**
   - Нажмите **Build → Build Project** (Ctrl+B)
   - Дождитесь успешной сборки

2. **Проверьте AndroidManifest.xml:**
   - Файл должен быть в `android/AndroidManifest.xml`
   - Package должен быть: `com.voiceupsidedown.master`
   - Activity должна быть: `org.qtproject.qt.android.bindings.QtActivity`

3. **Вручную введите настройки в Qt Creator:**
   - **Package name**: `com.voiceupsidedown.master`
   - **Activity name**: `org.qtproject.qt.android.bindings.QtActivity`

### Шаг 3: Создайте APK вручную (если автоматический запуск не работает)

1. **Соберите проект:**
   ```
   Build → Build Project (Ctrl+B)
   ```

2. **Создайте APK через скрипт:**
   ```powershell
   cd "D:\git\New Year. Turn your voice upside down\New Year. Turn your voice upside down\voice_upside_down\build\Android_Qt_6_7_2_Clang_arm64_v8a-Release"
   .\create_apk_qt6.bat
   ```

3. **Установите APK вручную:**
   ```powershell
   & "C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe" install -r "android-build\build\outputs\apk\debug\voice_upside_down-debug.apk"
   ```

4. **Запустите приложение вручную:**
   ```powershell
   & "C:\Users\User\AppData\Local\Android\Sdk\platform-tools\adb.exe" shell am start -n com.voiceupsidedown.master/org.qtproject.qt.android.bindings.QtActivity
   ```

### Шаг 4: Альтернатива - Использовать Qt Creator без автоматического запуска

1. **Соберите APK** (через скрипт или вручную)
2. **Установите APK** через ADB
3. **Запустите приложение** вручную на устройстве или через ADB

## Проверка

После настройки:
1. Убедитесь, что устройство подключено: `adb devices`
2. Попробуйте запустить через Qt Creator (F5)
3. Если не работает, используйте ручную установку через ADB

