// Copyright (C) 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/virtual-scene-page.h"

#include "android/emulation/control/virtual_scene_agent.h"
#include "android/globals.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/skin/qt/qt-settings.h"

#include <QDebug>
#include <QFileInfo>
#include <QSettings>

const QAndroidVirtualSceneAgent* VirtualScenePage::sVirtualSceneAgent = nullptr;

VirtualScenePage::VirtualScenePage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::VirtualScenePage()) {
    mUi->setupUi(this);

    connect(mUi->imageWall, SIGNAL(interaction()), this,
            SLOT(reportInteraction()));
    connect(mUi->imageTable, SIGNAL(interaction()), this,
            SLOT(reportInteraction()));
}

// static
void VirtualScenePage::setVirtualSceneAgent(
        const QAndroidVirtualSceneAgent* agent) {
    sVirtualSceneAgent = agent;
    loadInitialSettings();
}

void VirtualScenePage::showEvent(QShowEvent* event) {
    if (!mHasBeenShown) {
        mHasBeenShown = true;
        loadUi();
    }
}

void VirtualScenePage::on_imageWall_pathChanged(QString path) {
    changePoster("wall", path);
}

void VirtualScenePage::on_imageTable_pathChanged(QString path) {
    changePoster("table", path);
}

void VirtualScenePage::reportInteraction() {
    if (!mHadFirstInteraction) {
        mHadFirstInteraction = true;
        android::metrics::MetricsReporter::get().report(
                [](android_studio::AndroidStudioEvent* event) {
                    event->mutable_emulator_details()
                            ->mutable_used_features()
                            ->set_virtualscene_config(true);
                });
    }
}

void VirtualScenePage::changePoster(QString name, QString path) {
    // Persist to settings.
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        const QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

        QMap<QString, QVariant> savedPosters =
                avdSpecificSettings
                        .value(Ui::Settings::PER_AVD_VIRTUAL_SCENE_POSTERS)
                        .toMap();
        savedPosters[name] = path;

        avdSpecificSettings.setValue(
                Ui::Settings::PER_AVD_VIRTUAL_SCENE_POSTERS, savedPosters);
    }

    // Update the ImageWells so that future file selections will use this file's
    // directory as their starting directory.
    if (!path.isNull()) {
        QString dir = QFileInfo(path).path();
        mUi->imageWall->setStartingDirectory(dir);
        mUi->imageTable->setStartingDirectory(dir);
    }

    // Update the scene.
    if (sVirtualSceneAgent) {
        sVirtualSceneAgent->loadPoster(
                name.toUtf8().constData(),
                path.isNull() ? nullptr : path.toUtf8().constData());
    }
}

void VirtualScenePage::loadUi() {
    // Enumerate existing pointers.  If any are currently set, they were set
    // from the command line so they take precedence over the saved setting.
    sVirtualSceneAgent->enumeratePosters(
            this,
            [](void* context, const char* posterName, const char* filename) {
                auto self = reinterpret_cast<VirtualScenePage*>(context);
                const QString name = posterName;
                if (name == "wall") {
                    self->mUi->imageWall->setPath(filename);
                } else if (name == "table") {
                    self->mUi->imageTable->setPath(filename);
                }
            });
}

// static
void VirtualScenePage::loadInitialSettings() {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        const QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

        const QMap<QString, QVariant> savedPosters =
                avdSpecificSettings
                        .value(Ui::Settings::PER_AVD_VIRTUAL_SCENE_POSTERS)
                        .toMap();

        // Send the saved posters to the virtual scene.  This is called early
        // during emulator startup.  If a poster has been set via a command line
        // option, that poster will take priority and setInitialSetting will not
        // replace it.
        QMapIterator<QString, QVariant> it(savedPosters);
        while (it.hasNext()) {
            it.next();

            const QString& name = it.key();
            const QString path = it.value().toString();
            sVirtualSceneAgent->setInitialPoster(name.toUtf8().constData(),
                                                 path.toUtf8().constData());
        }
    }
}
