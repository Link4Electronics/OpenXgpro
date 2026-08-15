// Copyright (C) 2026 OpenXgpro contributors
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program. If not, see <https://www.gnu.org/licenses/>.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "algfile.h"
#include "blockfilldialog.h"
#include "chips.h"
#include "hexview.h"
#include "logiclist.h"
#include "mainwindow.h"
#include "operations.h"
#include "programmer.h"
#include "programmerdialog.h"

#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QRadioButton>
#include <QTabWidget>

#include <cstdio>

static QByteArray demoData()
{
    QByteArray data;
    for (int i = 0; i < 256; ++i)
        data.append(static_cast<char>(i));
    return data;
}

static HexView *hexViewOf(MainWindow *window)
{
    auto *tabs = window->findChild<QTabWidget *>();
    if (!tabs)
        return nullptr;
    return qobject_cast<HexView *>(tabs->widget(1));
}

static void clickRadio(MainWindow *window, const QString &text)
{
    const auto radios = window->findChildren<QRadioButton *>();
    for (auto *radio : radios) {
        if (radio->text() == text) {
            radio->click();
            return;
        }
    }
    fprintf(stderr, "WARN: radio '%s' not found\n", qPrintable(text));
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("hexview-smoke"));
    QApplication::setOrganizationName(QStringLiteral("OpenXgpro"));
    qputenv("OPENXGPRO_SCREENSHOT", "1"); // skip startup programmer dialog

    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                 : QStringLiteral("/tmp/opencode");
    QDir().mkpath(dir);

    if (Programmer::modelNames().size() != 3) {
        fprintf(stderr, "FAIL: expected 3 programmer models\n");
        return 1;
    }
    ProgrammerDialog progDlg(ProgrammerModel::T48);
    progDlg.show();
    progDlg.grab().save(dir + "/prog_dialog_light.png");
    if (progDlg.selectedModel() != ProgrammerModel::T48) {
        fprintf(stderr, "FAIL: initial model selection\n");
        return 1;
    }
    const auto progRadios = progDlg.findChildren<QRadioButton *>();
    if (progRadios.size() != 3) {
        fprintf(stderr, "FAIL: expected 3 model radios, got %d\n", progRadios.size());
        return 1;
    }

    // BlockFillDialog: rendering + apply logic.
    BlockFillDialog fillDlg(demoData());
    fillDlg.show();
    fillDlg.grab().save(dir + "/blockfill_light.png");
    {
        QByteArray d(32, char(0));
        BlockFillDialog::Params p;
        p.valid = true;
        p.start = 4;
        p.end = 7;
        p.width = 1;
        p.value = 0xFF;
        BlockFillDialog::apply(d, p);
        for (int i = 0; i < 32; ++i) {
            if (d.at(i) != (i >= 4 && i <= 7 ? char(0xFF) : char(0))) {
                fprintf(stderr, "FAIL: single-byte fill at %d\n", i);
                return 1;
            }
        }
        p.valid = true;
        p.start = 0;
        p.end = 3;
        p.width = 2;
        p.value = 0xAB; // little-endian pattern: AB 00 AB 00
        BlockFillDialog::apply(d, p);
        const unsigned char expect[] = {0xAB, 0x00, 0xAB, 0x00};
        for (int i = 0; i < 4; ++i) {
            if (static_cast<unsigned char>(d.at(i)) != expect[i]) {
                fprintf(stderr, "FAIL: 2-byte fill at %d\n", i);
                return 1;
            }
        }
        p.valid = true;
        p.start = 30;
        p.end = 999; // clamps to buffer end
        p.width = 1;
        p.value = 0x01;
        BlockFillDialog::apply(d, p);
        if (d.at(31) != char(0x01) || d.at(29) != char(0)) {
            fprintf(stderr, "FAIL: clamp fill\n");
            return 1;
        }
    }

    // Operations pipeline: validation + no-device error path (real libusb).
    const QString ref = QDir::current().filePath(QStringLiteral("../XgproV1316"));
    {
        const OpResult r1 = runOperation(OpType::Program, ProgrammerModel::T56,
                                         QString(), QByteArray());
        if (r1.ok || !r1.message.contains(QStringLiteral("chip"))) {
            fprintf(stderr, "FAIL: no-chip validation\n");
            return 1;
        }
        const OpResult r2 = runOperation(OpType::Program, ProgrammerModel::T56,
                                         QStringLiteral("AT89C51"), QByteArray());
        if (r2.ok || !r2.message.contains(QStringLiteral("Buffer"))) {
            fprintf(stderr, "FAIL: empty-buffer validation\n");
            return 1;
        }
        // Chip without an algorithm file (built-in sample / no reference).
        const OpResult r3 = runOperation(OpType::Read, ProgrammerModel::T56,
                                         QStringLiteral("AT89C51"), demoData());
        if (r3.ok || !r3.message.contains(QStringLiteral("algorithm"))) {
            fprintf(stderr, "FAIL: missing-algorithm validation\n");
            return 1;
        }
        // Chip with a real algorithm but no device -> no-programmer error.
        OpResult r4 = runOperation(OpType::Read, ProgrammerModel::T56,
                                   QStringLiteral("AT89C51"), demoData(),
                                   QStringLiteral("/nonexistent/AT89C_11.alg"));
        if (r4.ok || !r4.message.contains(QStringLiteral("missing"))) {
            fprintf(stderr, "FAIL: missing-file validation\n");
            return 1;
        }
        if (QDir(ref).exists()) {
            ChipDatabase db;
            db.loadReferenceData(ref);
            const QVector<ChipInfo> at89 = db.matching(
                QString(), QStringLiteral("AT89C_11"), true);
            if (at89.isEmpty()) {
                fprintf(stderr, "FAIL: reference chip AT89C_11 not found\n");
                return 1;
            }
            const QString alg = db.algorithmFile(at89.first());
            if (alg.isEmpty() || !alg.endsWith(QStringLiteral("AT89C_11.alg"))) {
                fprintf(stderr, "FAIL: algorithmFile resolution\n");
                return 1;
            }
            r4 = runOperation(OpType::Read, ProgrammerModel::T56, at89.first().name,
                              demoData(), alg);
            if (r4.ok) {
                fprintf(stderr, "FAIL: read should not succeed without a device\n");
                return 1;
            }
            fprintf(stderr, "op error sample: %s\n", qPrintable(r4.message));
        }
    }

    // Reference data loaders (real XgproV1316 distribution, if present).
    if (QDir(ref).exists()) {
        const QVector<AlgInfo> algs = scanAlgorithms(ref + "/algorithm");
        if (algs.size() < 100) {
            fprintf(stderr, "FAIL: expected >=100 algorithms, got %d\n", algs.size());
            return 1;
        }
        bool sawAt45 = false, sawVersion = true;
        for (const AlgInfo &a : algs) {
            if (a.familyName == QStringLiteral("AT45DB"))
                sawAt45 = true;
            if (a.magic == 0 || a.version != 0x0005)
                sawVersion = false;
        }
        if (!sawAt45 || !sawVersion) {
            fprintf(stderr, "FAIL: algorithm header parse (AT45DB=%d version=%d)\n",
                    int(sawAt45), int(sawVersion));
            return 1;
        }
        const QStringList logic = scanLogicList(ref + "/Logic.lst");
        if (logic.size() < 300) {
            fprintf(stderr, "FAIL: expected >=300 logic parts, got %d\n", logic.size());
            return 1;
        }
        ChipDatabase db;
        const int added = db.loadReferenceData(ref);
        if (added < algs.size() + logic.size()) {
            fprintf(stderr, "FAIL: loadReferenceData added %d, expected >=%d\n",
                    added, algs.size() + logic.size());
            return 1;
        }
        fprintf(stderr, "reference data: %d algs, %d logic parts, %d chips loaded\n",
                algs.size(), logic.size(), db.all().size());
    } else {
        fprintf(stderr, "WARN: %s missing, skipping reference checks\n", qPrintable(ref));
    }

    MainWindow window;
    window.resize(900, 700);
    window.show();

    QMetaObject::invokeMethod(&window, "updateBuffer", Qt::DirectConnection,
                              Q_ARG(QByteArray, demoData()));

    auto *tabs = window.findChild<QTabWidget *>();
    tabs->setCurrentIndex(1);
    HexView *hex = hexViewOf(&window);
    if (!hex) {
        fprintf(stderr, "FAIL: hex view not found\n");
        return 1;
    }

    window.grab().save(dir + "/hexview_light.png");
    hex->grab().save(dir + "/hexview_hex_light.png");

    clickRadio(&window, QStringLiteral("16 Bits"));
    hex->grab().save(dir + "/hexview_hex_16bit_light.png");
    clickRadio(&window, QStringLiteral("8 Bits"));

    QMetaObject::invokeMethod(&window, "setTheme", Qt::DirectConnection,
                              Q_ARG(int, 2));
    hex->grab().save(dir + "/hexview_hex_dark.png");
    window.grab().save(dir + "/hexview_dark.png");

    QMetaObject::invokeMethod(&window, "setTheme", Qt::DirectConnection,
                              Q_ARG(int, 1));

    QByteArray empty;
    QMetaObject::invokeMethod(&window, "updateBuffer", Qt::DirectConnection,
                              Q_ARG(QByteArray, empty));
    hex->grab().save(dir + "/hexview_empty.png");

    // Logic checks on a fresh buffer.
    QMetaObject::invokeMethod(&window, "updateBuffer", Qt::DirectConnection,
                              Q_ARG(QByteArray, demoData()));
    quint64 hit = 0;
    if (!hex->find(QByteArray::fromHex("ff"), 0, true, &hit) || hit != 255) {
        fprintf(stderr, "FAIL: find(FF) expected 255, got %llu\n",
                static_cast<unsigned long long>(hit));
        return 1;
    }
    if (!hex->find(QByteArray::fromHex("000102"), 0, true, &hit) || hit != 0) {
        fprintf(stderr, "FAIL: find(000102) expected 0\n");
        return 1;
    }
    hex->gotoAddress(0);
    if (hex->cursorIndex() != 0) {
        fprintf(stderr, "FAIL: gotoAddress(0) did not move cursor\n");
        return 1;
    }
    hex->fill(0x00, false);
    if (hex->data() != QByteArray(256, char(0))) {
        fprintf(stderr, "FAIL: fill(0x00) did not zero the buffer\n");
        return 1;
    }

    fprintf(stderr, "hexview-smoke OK\n");
    return 0;
}
