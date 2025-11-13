# Установка Android Kit для Qt Creator

## Шаг 1: Установка Qt для Android через Qt Maintenance Tool

1. **Откройте Qt Maintenance Tool**
   - Найдите в меню Пуск: **Qt Maintenance Tool**
   - Или откройте: `C:\Qt\MaintenanceTool.exe`

2. **Войдите в аккаунт** (если требуется)

3. **Выберите "Add or remove components"**

4. **Найдите вашу версию Qt** (например, Qt 5.15.2)

5. **Разверните раздел "Android"**

6. **Установите следующие компоненты:**
   - ✅ **Android arm64-v8a** (для Xiaomi K60 Ultra - обязательно!)
   - ✅ **Android armv7** (опционально, для старых устройств)
   - ✅ **Sources** (опционально, для отладки)
   - ✅ **Qt Creator** (если еще не установлен)

7. **Нажмите "Next"** и дождитесь установки

## Шаг 2: Установка Android SDK и NDK

### Вариант A: Через Android Studio (рекомендуется)

1. **Скачайте Android Studio**
   - Перейдите на https://developer.android.com/studio
   - Скачайте и установите Android Studio

2. **Запустите Android Studio**

3. **Откройте SDK Manager**
   - **Tools → SDK Manager**
   - Или **More Actions → SDK Manager**

4. **Установите необходимые компоненты:**
   - **SDK Platforms**:
     - ✅ Android 13.0 (Tiramisu) - API Level 33
     - ✅ Android 12.0 (S) - API Level 31
   - **SDK Tools**:
     - ✅ Android SDK Build-Tools
     - ✅ Android SDK Platform-Tools
     - ✅ Android SDK Command-line Tools
     - ✅ NDK (Side by side) - выберите версию **r21e** или новее (рекомендуется r25c)

5. **Запомните путь к SDK**
   - Обычно: `C:\Users\ВашеИмя\AppData\Local\Android\Sdk`
   - Или: `C:\Android\Sdk` (если выбрали другой путь)

### Вариант B: Установка только SDK (без Android Studio)

1. **Скачайте Command Line Tools**
   - https://developer.android.com/studio#command-tools

2. **Распакуйте в папку**, например: `C:\Android\Sdk\cmdline-tools\latest`

3. **Откройте командную строку** и выполните:
   ```bash
   cd C:\Android\Sdk\cmdline-tools\latest\bin
   sdkmanager "platform-tools" "platforms;android-33" "build-tools;33.0.0" "ndk;25.2.9519653"
   ```

## Шаг 3: Установка Java JDK

1. **Скачайте JDK**
   - Рекомендуется: **OpenJDK 11** или **Oracle JDK 8+**
   - Скачать можно с:
     - https://adoptium.net/ (OpenJDK)
     - https://www.oracle.com/java/technologies/downloads/ (Oracle JDK)

2. **Установите JDK** (обычно в `C:\Program Files\Java\jdk-версия`)

3. **Проверьте установку:**
   ```bash
   java -version
   javac -version
   ```

## Шаг 4: Настройка Android в Qt Creator

1. **Откройте Qt Creator**

2. **Перейдите в настройки:**
   - **Tools → Options → Devices → Android**

3. **Укажите пути:**
   - **Android SDK location**: путь к SDK (например, `C:\Users\ВашеИмя\AppData\Local\Android\Sdk`)
   - **Android NDK location**: путь к NDK (например, `C:\Users\ВашеИмя\AppData\Local\Android\Sdk\ndk\25.2.9519653`)
   - **JDK location**: путь к JDK (например, `C:\Program Files\Java\jdk-11`)

4. **Нажмите "Apply"**

5. **Проверьте статус:**
   - Должны появиться зеленые галочки ✅
   - Если есть ошибки - проверьте пути

## Шаг 5: Настройка Qt Versions

1. **В Qt Creator:**
   - **Tools → Options → Kits → Qt Versions**

2. **Нажмите "Add"**

3. **Укажите путь к qmake для Android:**
   - Обычно: `C:\Qt\5.15.2\android_arm64_v8a\bin\qmake.exe`
   - Или: `C:\Qt\5.15.2\android\bin\qmake.exe`

4. **Нажмите "Apply"**

5. **Проверьте, что версия Qt определилась правильно**

## Шаг 6: Создание Android Kit

1. **В Qt Creator:**
   - **Tools → Options → Kits**

2. **Нажмите "Add"** (или найдите существующий Android Kit)

3. **Заполните параметры:**
   - **Name**: `Android arm64-v8a (Qt 5.15.2)` (или любое удобное имя)
   - **Device type**: `Android Device`
   - **Qt version**: выберите установленную Qt для Android
   - **Compiler**: `Android Clang (arm64-v8a)` (должен определиться автоматически)
   - **Debugger**: `Android Debugger` (должен определиться автоматически)
   - **CMake**: автоматически выберется

4. **Нажмите "OK"**

5. **Проверьте Kit:**
   - Должна быть зеленая галочка ✅
   - Если есть предупреждения - исправьте их

## Шаг 7: Проверка установки

1. **Откройте проект** в Qt Creator:
   - **File → Open File or Project**
   - Выберите `CMakeLists.txt`

2. **При выборе Kit** вы должны увидеть ваш Android Kit

3. **Выберите Android Kit** и нажмите **Configure Project**

4. **Проверьте, что проект настроился без ошибок**

## Решение проблем

### Проблема: "Android SDK not found"
**Решение:**
- Проверьте путь к SDK в настройках
- Убедитесь, что SDK установлен (проверьте папку `platforms` в SDK)

### Проблема: "NDK not found"
**Решение:**
- Установите NDK через Android Studio SDK Manager
- Или скачайте вручную с https://developer.android.com/ndk/downloads
- Укажите правильный путь в настройках Qt Creator

### Проблема: "JDK not found"
**Решение:**
- Установите JDK 8 или новее
- Укажите путь к папке JDK (не к `bin`, а к корневой папке JDK)
- Проверьте переменную окружения `JAVA_HOME`

### Проблема: "No suitable compiler found"
**Решение:**
- Убедитесь, что установлен Android NDK
- Qt Creator должен автоматически найти компилятор в NDK
- Если нет - проверьте путь к NDK

### Проблема: Qt для Android не устанавливается
**Решение:**
- Убедитесь, что у вас есть лицензия Qt (или используйте открытую версию)
- Проверьте интернет-соединение
- Попробуйте запустить Qt Maintenance Tool от имени администратора

### Проблема: Kit создан, но проект не собирается
**Решение:**
- Проверьте, что все пути указаны правильно
- Убедитесь, что выбран правильный Kit при открытии проекта
- Проверьте логи сборки на наличие ошибок

## Альтернативный способ: Использование готового Qt для Android

Если у вас проблемы с установкой через Qt Maintenance Tool:

1. **Скачайте готовый Qt для Android:**
   - https://www.qt.io/download-open-source
   - Выберите версию Qt 5.15.2 для Android

2. **Распакуйте** в папку, например: `C:\Qt\5.15.2\android_arm64_v8a`

3. **В Qt Creator добавьте эту версию Qt:**
   - **Tools → Options → Kits → Qt Versions → Add**
   - Укажите путь к `qmake.exe` в этой папке

## Проверка готовности

После выполнения всех шагов проверьте:

✅ Qt для Android установлен  
✅ Android SDK установлен и настроен  
✅ Android NDK установлен и настроен  
✅ JDK установлен и настроен  
✅ Qt Creator видит все компоненты (зеленые галочки)  
✅ Android Kit создан и активен  
✅ Проект открывается с Android Kit без ошибок  

Если все пункты выполнены - можно начинать сборку!

