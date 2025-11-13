# Пошаговая инструкция по сборке Android APK в Qt Creator

## Шаг 1: Проверка установки Qt для Android

1. Откройте Qt Creator
2. Перейдите в **Tools → Options → Kits**
3. Убедитесь, что установлен Qt для Android (например, Qt 5.15.2 for Android)
4. Если нет - установите через **Qt Maintenance Tool**:
   - Выберите **Add or remove components**
   - Найдите **Qt 5.15.2** → **Android**
   - Установите **Android arm64-v8a** (для Xiaomi K60 Ultra)

## Шаг 2: Настройка Android SDK и NDK

1. В Qt Creator перейдите в **Tools → Options → Devices → Android**
2. Укажите пути:
   - **Android SDK location**: обычно `C:\Users\ВашеИмя\AppData\Local\Android\Sdk`
   - **Android NDK location**: обычно `C:\Users\ВашеИмя\AppData\Local\Android\Sdk\ndk\версия`
   - **JDK location**: обычно `C:\Program Files\Java\jdk-версия` или `C:\Program Files\Android\Android Studio\jbr`

3. Если Android SDK не установлен:
   - Скачайте **Android Studio** с https://developer.android.com/studio
   - Установите Android Studio
   - Откройте **Tools → SDK Manager**
   - Установите:
     - Android SDK Platform-Tools
     - Android SDK Build-Tools
     - Android SDK Platform (API 33 или выше)
     - Android NDK (Side by side) - выберите версию r21 или новее

4. Нажмите **Apply** и дождитесь проверки

## Шаг 3: Настройка Kit для Android

1. В Qt Creator перейдите в **Tools → Options → Kits**
2. Найдите или создайте Kit с:
   - **Device type**: Android Device
   - **Qt version**: Qt 5.15.2 for Android (arm64-v8a)
   - **Compiler**: Android Clang (arm64-v8a)
   - **Debugger**: Android Debugger
   - **CMake**: автоматически выберется

3. Если Kit не найден:
   - Перейдите в **Tools → Options → Kits → Qt Versions**
   - Нажмите **Add** и укажите путь к `qmake.exe` для Android:
     - Обычно: `C:\Qt\5.15.2\android_arm64_v8a\bin\qmake.exe`
   - Затем создайте Kit с этой версией Qt

## Шаг 4: Открытие проекта

1. В Qt Creator нажмите **File → Open File or Project**
2. Выберите файл `CMakeLists.txt` из корня проекта
3. Нажмите **Open**
4. Qt Creator предложит выбрать Kit - выберите **Android Kit** (arm64-v8a)
5. Нажмите **Configure Project**

## Шаг 5: Настройка проекта для Android

1. В левой панели выберите **Projects** (значок молотка)
2. Убедитесь, что выбран **Android Kit**
3. В разделе **Build Settings**:
   - **Build directory**: оставьте по умолчанию или укажите `build-android`
   - **CMake configuration**: проверьте, что указаны правильные пути

4. В разделе **Run Settings**:
   - **Package name**: `com.voiceupsidedown.master`
   - **Application binary**: `voice_upside_down`
   - **Target device**: выберите ваше устройство или эмулятор

## Шаг 6: Подключение устройства (опционально)

1. На телефоне Xiaomi K60 Ultra:
   - Откройте **Настройки → О телефоне**
   - Нажмите 7 раз на **Номер сборки** для активации режима разработчика
   - Вернитесь в **Настройки → Дополнительные настройки → Для разработчиков**
   - Включите **Отладка по USB**
   - Включите **Установка через USB**

2. Подключите телефон к компьютеру через USB
3. В Qt Creator перейдите в **Tools → Options → Devices → Android**
4. Нажмите **Test** для проверки подключения
5. В **Run Settings** выберите ваше устройство

## Шаг 7: Сборка проекта

1. Нажмите **Build → Build Project** (или Ctrl+B)
2. Дождитесь завершения сборки
3. Проверьте панель **Compile Output** на наличие ошибок

## Шаг 8: Создание APK

1. После успешной сборки нажмите **Build → Build Android APK**
2. Укажите путь для сохранения APK (или оставьте по умолчанию)
3. Дождитесь создания APK
4. APK будет находиться в папке сборки, обычно:
   - `build-android/android-build/build/outputs/apk/debug/voice_upside_down.apk`
   - или `build-android/android-build/voice_upside_down.apk`

## Шаг 9: Установка на устройство

### Вариант 1: Через Qt Creator
1. Нажмите **Run** (зеленая кнопка или F5)
2. Qt Creator автоматически установит и запустит приложение на подключенном устройстве

### Вариант 2: Вручную через ADB
```bash
adb install -r путь\к\voice_upside_down.apk
```

## Решение проблем

### Проблема: "Android SDK not found"
- Решение: Проверьте путь к Android SDK в **Tools → Options → Devices → Android**

### Проблема: "NDK not found"
- Решение: Установите NDK через Android Studio SDK Manager и укажите путь

### Проблема: "JDK not found"
- Решение: Установите JDK 8 или новее и укажите путь в настройках Android

### Проблема: "No suitable kits found"
- Решение: Установите Qt для Android через Qt Maintenance Tool

### Проблема: Ошибки компиляции
- Решение: Проверьте, что выбран правильный Kit (Android arm64-v8a)
- Убедитесь, что все зависимости Qt установлены

### Проблема: "Permission denied" при установке
- Решение: На телефоне включите "Установка из неизвестных источников" в настройках безопасности

## Проверка сборки

После успешной сборки проверьте:
1. APK файл создан
2. Размер APK разумный (обычно 20-50 МБ для Qt приложений)
3. Приложение устанавливается на устройство
4. Приложение запускается и работает

## Дополнительные настройки

### Подпись APK (для релизной версии)
1. Создайте keystore:
   ```bash
   keytool -genkey -v -keystore voice_upside_down.keystore -alias voice_upside_down -keyalg RSA -keysize 2048 -validity 10000
   ```
2. В Qt Creator: **Projects → Build → Android Build Settings → Sign package**

### Оптимизация размера APK
- Используйте **Build → Build Android APK → Release** вместо Debug
- Включите сжатие в настройках сборки

