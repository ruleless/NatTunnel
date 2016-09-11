
#include <QCoreApplication>
#include <QFile>
#include <QMutex>
#include <time.h>
#include <iostream>
#include "ClientManager.h"

static QFile fileLog;
static QMutex mutexFileLog;

void MyMessageHandler(QtMsgType type, const QMessageLogContext & context, const QString & text)
{
	const QDateTime datetime = QDateTime::currentDateTime();
	const char * typeText = nullptr;
	switch (type)
	{
	case QtDebugMsg:
	case QtInfoMsg:
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
	if (fileLog.isOpen())
	{
		QMutexLocker locker(&mutexFileLog);
		if (fileLog.size() == 0)
			fileLog.write("\xef\xbb\xbf");
		const QString finalText = QString("%1 %2 %3\n").arg(datetime.toString("yyyyMMdd/hh:mm:ss.zzz")).arg(typeText).arg(text);
		fileLog.write(finalText.toUtf8());
		fileLog.flush();
		locker.unlock();

		std::cout << finalText.toLocal8Bit().constData();
	}
}

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);

	fileLog.setFileName(app.applicationDirPath() + "/NatTunnelServer.log");
	fileLog.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
	qInstallMessageHandler(MyMessageHandler);

	QMap<QString, QString> mapUserList;
	mapUserList["user1"] = "123456";
	mapUserList["user2"] = "654321";
	ClientManager clientManager;
	clientManager.setUserList(mapUserList);
	clientManager.start(7771, 7772, 7773);

	return app.exec();
}