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
//
// Disabled: this trace was added while debugging the composite media modules
// and it opens/writes a file on every call, which is very expensive in the
// hot paths (every storage slice update and every page request). Kept as a
// no-op so the call sites stay in place if the trace is needed again.
inline void append(const QString &text) {
	(void)text;
}

[[nodiscard]] inline QString num(qlonglong value) {
	return QString::number(value);
}

} // namespace Depur