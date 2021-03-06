﻿
#include <QCoreApplication>
#include <QFile>
#include <QMutex>
#include <QSettings>
#include <QTextCodec>
#include <QStringList>
#include <time.h>
#include <iostream>
#include "ClientManager.h"

static QFile fileLog;
static QMutex mutexFileLog;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
void MyMessageHandler(QtMsgType type, const QMessageLogContext & context, const QString & text)
#else
void MyMessageHandler(QtMsgType type, const char * text)
#endif
{
	const QDateTime datetime = QDateTime::currentDateTime();
	const char * typeText = NULL;
	switch (type)
	{
	case QtDebugMsg:
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	case QtInfoMsg:
#endif
		typeText = "Info";
		break;
	case QtWarningMsg:
		typeText = "Warning";
		break;
	case QtCriticalMsg:
		typeText = "Critical";
		break;
	case QtFatalMsg:
		abort();
	}
	const QString finalText = QString("%1 %2 %3\n").arg(datetime.toString("yyyyMMdd/hh:mm:ss.zzz")).arg(typeText).arg(text);
	if (fileLog.isOpen())
	{
		QMutexLocker locker(&mutexFileLog);
		if (fileLog.size() == 0)
			fileLog.write("\xef\xbb\xbf");
		fileLog.write(finalText.toUtf8());
		fileLog.flush();
		locker.unlock();
	}

	std::cout << finalText.toLocal8Bit().constData();
}

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);

	fileLog.setFileName(app.applicationDirPath() + "/NatTunnelServer.log");
	fileLog.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	qInstallMessageHandler(MyMessageHandler);
#else
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("utf8"));
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf8"));
	qInstallMsgHandler(MyMessageHandler);
#endif

	QSettings setting("NatTunnelServer.ini", QSettings::IniFormat);
	QMap<QString, QString> mapUser;
	setting.beginGroup("User");
	foreach (QString userName, setting.childKeys())
		mapUser[userName] = setting.value(userName).toString();
	setting.endGroup();

	const int tcpPort = setting.value("Port/Tcp").toInt();
	const int udp1Port = setting.value("Port/Udp1").toInt();
	const int udp2Port = setting.value("Port/Udp2").toInt();

	const QByteArray globalKey = setting.value("Other/GlobalKey").toByteArray();
	if (globalKey.isEmpty())
		qWarning() << QString("Empty GlobalKey");

	ClientManager clientManager;

	setting.beginGroup("Binary");
	foreach(QString platform, setting.childKeys())
	{
		const QByteArray binary = readFile(setting.value(platform).toString());
		clientManager.setPlatformBinary(platform, binary);
	}
	setting.endGroup();
	
	clientManager.setGlobalKey(globalKey);
	clientManager.setDatabase("User.db", "root", "000000");
	if (!clientManager.start(tcpPort, udp1Port, udp2Port))
		return 0;

	return app.exec();
}
