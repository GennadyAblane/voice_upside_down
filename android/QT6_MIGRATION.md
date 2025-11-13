# Миграция на Qt 6.7.2 для Android

## Преимущества Qt 6 для Android

1. **Лучшая поддержка Android**: Qt 6 имеет улучшенную поддержку Android и более современные инструменты
2. **Автоматическое создание APK**: Встроенная поддержка через `qt_android_add_apk_target`
3. **Совместимость с новыми NDK**: Лучшая работа с NDK 25.x
4. **Современный CMake**: Полная поддержка CMake (Qt 6 полностью на CMake)

## Шаги миграции

### 1. Обновить CMakeLists.txt

Замените корневой `CMakeLists.txt` на версию для Qt 6:

```bash
# Создайте резервную копию
cp CMakeLists.txt CMakeLists_Qt5.txt

# Используйте новую версию
cp CMakeLists_Qt6.txt CMakeLists.txt
```

Аналогично для `src/CMakeLists.txt`:

```bash
cp src/CMakeLists.txt src/CMakeLists_Qt5.txt
cp src/CMakeLists_Qt6.txt src/CMakeLists.txt
```

### 2. Проверить совместимость кода

Qt 6 имеет некоторые изменения API:

- `QGuiApplication` вместо `QApplication` (уже используется)
- `Qt6::Core`, `Qt6::Gui` вместо `Qt5::Widgets`
- Некоторые классы Multimedia могут иметь изменения

### 3. Настроить Kit в Qt Creator

1. **Projects → Manage Kits**
2. Выберите Kit: **"Android Qt 6.7.2 Clang arm64-v8a"**
3. Нажмите **Configure Project**

### 4. Сборка

1. **Build → Clean** (очистить старую сборку)
2. **Build → Build Project** (Ctrl+B)
3. APK должен создаться автоматически через `qt_android_add_apk_target`

### 5. Создание APK

В Qt 6 APK создается автоматически при сборке, или можно использовать:

```bash
cmake --build . --target voice_upside_down_apk
```

APK будет в:
- `build/Android_Qt_6_7_2_Clang_arm64_v8a-Release/android-build/build/outputs/apk/debug/voice_upside_down-debug.apk`

## Откат на Qt 5.15.2

Если нужно вернуться к Qt 5.15.2:

```bash
cp CMakeLists_Qt5.txt CMakeLists.txt
cp src/CMakeLists_Qt5.txt src/CMakeLists.txt
```

Затем выберите Kit "Android arm64-v8a (Qt 5.15.2)" в Qt Creator.

## Проверка совместимости

Основные изменения в коде, которые могут потребоваться:

1. **QAudioFormat**: Проверить совместимость методов
2. **QAudioOutput/QAudioInput**: Проверить API
3. **QStandardPaths**: Должен работать без изменений
4. **QML**: Должен работать без изменений

## Рекомендации

- **Для новых проектов**: Используйте Qt 6.7.2
- **Для существующих проектов**: Можно мигрировать постепенно
- **Для Android**: Qt 6 предпочтительнее из-за лучшей поддержки

