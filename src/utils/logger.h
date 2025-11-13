#pragma once

#include <QDebug>

#define LOG_INFO(...) qInfo(__VA_ARGS__)
#define LOG_WARN(...) qWarning(__VA_ARGS__)
#define LOG_ERROR(...) qCritical(__VA_ARGS__)

