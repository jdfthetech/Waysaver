#include "settingswindow.h"

#include "appconfig.h"
#include "packageapi.h"
#include "scrimporter.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDBusInterface>
#include <QDBusReply>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <functional>

static QWidget *pathRow(QLineEdit **edit, const QString &buttonText,
                        const std::function<void()> &action)
{
    auto *row = new QWidget;
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    *edit = new QLineEdit;
    (*edit)->setReadOnly(true);
    auto *button = new QPushButton(buttonText);
    QObject::connect(button, &QPushButton::clicked, row, [action] { action(); });
    layout->addWidget(*edit, 1);
    layout->addWidget(button);
    return row;
}

SettingsWindow::SettingsWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("WaySaver Settings");
    setMinimumWidth(650);
    auto *root = new QVBoxLayout(this);
    auto *title = new QLabel(
        "<h1>WaySaver</h1><p>A Wayland screensaver for Hyprland and compatible compositors</p>");
    root->addWidget(title);

    auto *form = new QFormLayout;
    m_mode = new QComboBox;
    m_mode->addItem("Image folder", "images");
    m_mode->addItem("Windows .scr through Wine", "windows-scr");
    m_mode->addItem("WaySaver package", "package");
    form->addRow("Screensaver type", m_mode);
    m_imageRow = pathRow(&m_imageDirectory, "Choose Folder", [this] { chooseImageFolder(); });
    m_scrRow = pathRow(&m_scrPath, "Import .scr", [this] { importScr(); });
    m_packageRow = pathRow(&m_packagePath, "Import Package", [this] { importPackage(); });
    form->addRow("Images", m_imageRow);
    form->addRow("Windows saver", m_scrRow);
    form->addRow("Package", m_packageRow);

    m_idleMinutes = new QSpinBox;
    m_idleMinutes->setRange(1, 60);
    m_idleMinutes->setSuffix(" minutes");
    form->addRow("Start after", m_idleMinutes);
    m_slideSeconds = new QSpinBox;
    m_slideSeconds->setRange(2, 3600);
    m_slideSeconds->setSuffix(" seconds");
    form->addRow("Change image every", m_slideSeconds);
    m_shuffle = new QCheckBox("Shuffle images");
    form->addRow(QString(), m_shuffle);
    m_mediaGuard = new QCheckBox("Do not start while a media player is playing");
    m_mediaGuard->setToolTip(
        "Uses MPRIS. The compositor's Wayland idle inhibitors are also honored.");
    form->addRow(QString(), m_mediaGuard);
    root->addLayout(form);

    auto *warning = new QLabel(
        "<b>Windows screensaver warning:</b> .scr files are executable software. "
        "Only import files you trust. 32 bit savers require Wine. Many 16 bit savers "
        "also require a WineVDM setup and may not run on a modern Linux system.");
    warning->setWordWrap(true);
    warning->setStyleSheet("padding: 8px; background: palette(alternate-base);");
    root->addWidget(warning);

    m_status = new QLabel;
    m_status->setWordWrap(true);
    root->addWidget(m_status);
    auto *buttons = new QHBoxLayout;
    auto *configure = new QPushButton("Configure .scr");
    auto *previewButton = new QPushButton("Preview Now");
    auto *saveButton = new QPushButton("Save");
    buttons->addWidget(configure);
    buttons->addStretch();
    buttons->addWidget(previewButton);
    buttons->addWidget(saveButton);
    root->addLayout(buttons);

    connect(m_mode, &QComboBox::currentIndexChanged, this, &SettingsWindow::refreshControls);
    connect(configure, &QPushButton::clicked, this, &SettingsWindow::configureScr);
    connect(previewButton, &QPushButton::clicked, this, &SettingsWindow::preview);
    connect(saveButton, &QPushButton::clicked, this, &SettingsWindow::save);

    const AppConfig c = AppConfig::load();
    m_mode->setCurrentIndex(qMax(0, m_mode->findData(AppConfig::modeName(c.mode))));
    m_imageDirectory->setText(c.imageDirectory);
    m_scrPath->setText(c.scrPath);
    m_packagePath->setText(c.packagePath);
    m_idleMinutes->setValue(c.idleMinutes);
    m_slideSeconds->setValue(c.slideSeconds);
    m_shuffle->setChecked(c.shuffle);
    m_mediaGuard->setChecked(c.mediaGuard);
    refreshControls();
}

void SettingsWindow::refreshControls()
{
    const QString mode = m_mode->currentData().toString();
    m_imageRow->setVisible(mode == "images");
    m_scrRow->setVisible(mode == "windows-scr");
    m_packageRow->setVisible(mode == "package");
    m_slideSeconds->setEnabled(mode == "images");
    m_shuffle->setEnabled(mode == "images");
}

void SettingsWindow::chooseImageFolder()
{
    const QString path = QFileDialog::getExistingDirectory(this, "Choose an image folder", m_imageDirectory->text());
    if (!path.isEmpty()) m_imageDirectory->setText(path);
}

void SettingsWindow::importScr()
{
    const QString source = QFileDialog::getOpenFileName(this, "Import a Windows screensaver", {}, "Windows screensavers (*.scr);;All files (*)");
    if (source.isEmpty()) return;
    const auto inspection = ScrImporter::inspect(source);
    if (!inspection.valid) {
        QMessageBox::critical(this, "Invalid screensaver", inspection.error);
        return;
    }
    const QString message = QString("WaySaver detected a %1.\n\nThis file is an executable program and will run through Wine. Only continue if you trust its source.")
                                .arg(inspection.description);
    if (QMessageBox::warning(this, "Import executable screensaver", message,
                             QMessageBox::Cancel | QMessageBox::Ok, QMessageBox::Cancel) != QMessageBox::Ok)
        return;
    QString error;
    const QString imported = ScrImporter::importFile(source, &error);
    if (imported.isEmpty()) {
        QMessageBox::critical(this, "Import failed", error);
        return;
    }
    m_scrPath->setText(imported);
    m_status->setText("Imported " + inspection.description + ".");
}

void SettingsWindow::importPackage()
{
    const QString path = QFileDialog::getOpenFileName(this, "Import a WaySaver package", {}, "WaySaver packages (*.waysaver);;JSON files (*.json)");
    if (path.isEmpty()) return;
    PackageApi::Definition definition;
    const auto result = PackageApi::validate(path, &definition);
    if (!result.ok) {
        QMessageBox::critical(this, "Invalid package", result.error);
        return;
    }
    if (definition.type == "executable") {
        const QString warning =
            "This package starts a native executable. Only continue if you trust "
            "the package and every file it references.";
        if (QMessageBox::warning(this, "Import executable package", warning,
                                 QMessageBox::Cancel | QMessageBox::Ok,
                                 QMessageBox::Cancel) != QMessageBox::Ok)
            return;
    }
    m_packagePath->setText(path);
    m_status->setText("Selected package: " + definition.name);
}

void SettingsWindow::save()
{
    AppConfig c;
    c.mode = AppConfig::modeFromName(m_mode->currentData().toString());
    c.imageDirectory = m_imageDirectory->text();
    c.scrPath = m_scrPath->text();
    c.packagePath = m_packagePath->text();
    c.idleMinutes = m_idleMinutes->value();
    c.slideSeconds = m_slideSeconds->value();
    c.shuffle = m_shuffle->isChecked();
    c.mediaGuard = m_mediaGuard->isChecked();
    c.save();
    QDBusInterface daemon("org.waysaver.Service", "/WaySaver", "org.waysaver.Service");
    daemon.asyncCall("Reload");
    m_status->setText("Settings saved.");
}

void SettingsWindow::preview()
{
    save();
    QDBusInterface daemon("org.waysaver.Service", "/WaySaver", "org.waysaver.Service");
    if (!daemon.isValid()) {
        m_status->setText("The WaySaver background service is not running.");
        return;
    }
    daemon.asyncCall("Activate");
}

void SettingsWindow::configureScr()
{
    if (m_scrPath->text().isEmpty()) return;
    const QString wine = QStandardPaths::findExecutable("wine");
    if (wine.isEmpty()) {
        QMessageBox::warning(this, "Wine missing", "Install Wine to configure this screensaver.");
        return;
    }
    QProcess::startDetached(wine, {m_scrPath->text(), "/c"});
}
