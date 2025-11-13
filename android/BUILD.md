# Сборка Android APK

## Требования

1. **Qt 5.15 или новее** с поддержкой Android
2. **Android SDK** (установлен через Android Studio или отдельно)
3. **Android NDK** (рекомендуется r21 или новее)
4. **Java JDK** (версия 8 или новее)
5. **CMake** 3.16 или новее

## Настройка Qt для Android

1. Откройте Qt Creator
2. Перейдите в **Tools → Options → Devices → Android**
3. Укажите пути к:
   - Android SDK
   - Android NDK
   - Java JDK
4. Нажмите **Apply**

## Сборка через Qt Creator

1. Откройте проект `CMakeLists.txt` в Qt Creator
2. Выберите **Kit** с Android (например, "Android for arm64-v8a (Clang Qt 5.15.2)")
3. Нажмите **Configure Project**
4. Нажмите **Build** (Ctrl+B)
5. После успешной сборки нажмите **Run** для установки на устройство или **Build → Build Android APK**

## Сборка через командную строку

```bash
# Создайте директорию для сборки
mkdir build-android
cd build-android

# Настройте CMake для Android
cmake .. \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
    -DCMAKE_ANDROID_NDK=/path/to/android-ndk \
    -DCMAKE_ANDROID_STL_TYPE=c++_shared \
    -DQT_HOST_PATH=/path/to/qt/5.15.2/android \
    -DCMAKE_PREFIX_PATH=/path/to/qt/5.15.2/android

# Соберите проект
cmake --build . --target voice_upside_down

# Создайте APK (если используется Qt Android deployment)
cmake --build . --target apk
```

## Установка на устройство

### Через Qt Creator
1. Подключите Android устройство через USB
2. Включите **Отладку по USB** в настройках разработчика
3. Нажмите **Run** в Qt Creator

### Через ADB
```bash
adb install build-android/android-build/voice_upside_down.apk
```

## Разрешения

Приложение требует следующие разрешения:
- **READ_EXTERNAL_STORAGE** - для чтения аудио файлов
- **WRITE_EXTERNAL_STORAGE** - для сохранения проектов и результатов
- **RECORD_AUDIO** - для записи с микрофона
- **READ_MEDIA_AUDIO** - для доступа к аудио файлам (Android 13+)

## Поддерживаемые архитектуры

- **arm64-v8a** (рекомендуется, для Xiaomi K60 Ultra)
- **armeabi-v7a**
- **x86** (для эмуляторов)
- **x86_64** (для эмуляторов)

## Особенности для Xiaomi K60 Ultra (HyperOS)

Приложение оптимизировано для:
- **Вертикальная ориентация** (portrait mode)
- **Разрешение экрана**: 1220x2712 пикселей
- **Оптимизированный UI** для сенсорного управления
- **Компактные размеры элементов** для мобильного экрана

## Отладка

Для просмотра логов:
```bash
adb logcat | grep voice_upside_down
```

## Известные проблемы

1. **FileDialog**: На Android может потребоваться использование нативного диалога выбора файлов
2. **Пути к файлам**: Используются стандартные пути Android через QStandardPaths
3. **Микрофон**: Требуется разрешение RECORD_AUDIO, которое запрашивается при первом использовании

