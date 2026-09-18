#pragma once

#include <QElapsedTimer>
#include <QPointF>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QWidget>

class QLabel;
class QScreen;
class QTimer;

class SaverWindow final : public QWidget {
    Q_OBJECT
public:
    explicit SaverWindow(QScreen *screen, const QString &directory,
                         int slideSeconds, bool shuffle, QWidget *parent = nullptr);

signals:
    void dismissRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void nextImage();

private:
    void rebuildList();
    QLabel *m_label = nullptr;
    QTimer *m_timer = nullptr;
    QString m_directory;
    QStringList m_images;
    int m_index = -1;
    bool m_shuffle = true;
    QPointF m_firstMousePosition;
    bool m_haveMousePosition = false;
};
