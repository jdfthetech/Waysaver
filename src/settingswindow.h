#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;

class SettingsWindow final : public QWidget {
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget *parent = nullptr);

private slots:
    void chooseImageFolder();
    void importScr();
    void importPackage();
    void save();
    void preview();
    void configureScr();
    void refreshControls();

private:
    QComboBox *m_mode = nullptr;
    QLineEdit *m_imageDirectory = nullptr;
    QLineEdit *m_scrPath = nullptr;
    QLineEdit *m_packagePath = nullptr;
    QSpinBox *m_idleMinutes = nullptr;
    QSpinBox *m_slideSeconds = nullptr;
    QCheckBox *m_shuffle = nullptr;
    QCheckBox *m_mediaGuard = nullptr;
    QLabel *m_status = nullptr;
    QWidget *m_imageRow = nullptr;
    QWidget *m_scrRow = nullptr;
    QWidget *m_packageRow = nullptr;
};

