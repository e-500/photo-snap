/*
 * Photo Snap is a free, cross-platform desktop application for managing a domestic picture archive.
 * Copyright (C) 2026 Andrea Piovesan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Photo Snap"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("Photo Snap"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/photosnap.png")));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("A simple desktop photo viewer."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("photo"),
                                 QStringLiteral("Photo to open."),
                                 QStringLiteral("[photo]"));
    parser.process(app);

    MainWindow window;
    window.show();

    const QStringList arguments = parser.positionalArguments();
    if (!arguments.isEmpty())
        window.openImage(arguments.constFirst());

    return app.exec();
}
