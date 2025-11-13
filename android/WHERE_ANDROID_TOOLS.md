# Где найти Android Tools в Qt Maintenance Tool

## Расположение Android Tools

Android Tools **НЕ** находятся внутри компонента "Qt 6.7.2 → Android". Они находятся в **отдельной секции** на верхнем уровне списка компонентов.

## Как найти Android Tools:

### Вариант 1: Прокрутите список вверх

1. В Qt Maintenance Tool на шаге **"Customize"**
2. Прокрутите список компонентов **вверх** (к началу списка)
3. Ищите секцию **"Developer and Designer Tools"** или **"Tools"**
4. Внутри этой секции должна быть:
   - **Android Tools** (или **Qt Android Tools**)

### Вариант 2: Используйте поиск

1. В поле поиска (с иконкой лупы) введите: **"Android Tools"**
2. Должен появиться компонент **Android Tools**

### Вариант 3: Структура компонентов

Android Tools обычно находятся в такой структуре:
```
Developer and Designer Tools
  ├── Qt Creator
  ├── Qt Designer
  ├── Qt Linguist
  └── Android Tools  ← ВОТ ОНО!
```

Или может быть отдельно:
```
Tools
  └── Android Tools  ← ВОТ ОНО!
```

## Что установить:

Установите:
- ✅ **Android Tools** (обязательно!)
- ✅ **Qt 6.7.2 → Android** (если еще не установлен)
- ✅ **Qt 6.7.2 → Android → arm64-v8a** (для вашего устройства)

## После установки:

1. Нажмите **"Next >"**
2. Примите лицензию
3. Дождитесь установки
4. Проверьте: `Test-Path "C:\Qt\6.7.2\android_arm64_v8a\bin\androiddeployqt.exe"`

## Важно:

Android Tools - это **отдельный компонент**, не подкомпонент Android. Он находится на том же уровне, что и Qt 6.7.2, Qt Creator и другие основные компоненты.

