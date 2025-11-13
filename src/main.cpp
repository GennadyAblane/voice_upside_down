#include "appcontroller.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("VoiceUpsideDown"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("voiceupside.local"));
    QCoreApplication::setApplicationName(QStringLiteral("Voice Upside Down"));

    QQmlApplicationEngine engine;
    AppController controller;

    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}

