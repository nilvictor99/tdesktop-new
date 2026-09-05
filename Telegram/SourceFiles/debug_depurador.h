/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "settings.h"

#include <QMutex>
#include <QFile>
#include <QDateTime>
#include <QTextStream>

// Debug helper: writes a detailed trace of the FilesPhotos ("Fotos + Archivos")
// and All ("Todo") media modules to depurador.txt, next to log.txt. The file is
// cleared once per application launch (first time a trace line is written), so
// each run produces a fresh depurador.txt.
namespace Depur {

[[nodiscard]] inline QString filePath() {
	return cWorkingDir() + QStringLiteral("depurador.txt");
}

// Appends one line with a timestamp. Thread-safe. Clears the file on the very
// first append of the process so the log is fresh for every run.
inline void append(const QString &text) {
	static QMutex mutex;
	QMutexLocker lock(&mutex);
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		if (QFile::exists(filePath())) {
			QFile::remove(filePath());
		}
	}
	QFile file(filePath());
	if (file.open(QIODevice::Append | QIODevice::Text)) {
		QTextStream out(&file);
		out << QDateTime::currentDateTime().toString(
			QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")) << ' '
			<< text << '\n';
	}
}

[[nodiscard]] inline QString num(qlonglong value) {
	return QString::number(value);
}

} // namespace Depur