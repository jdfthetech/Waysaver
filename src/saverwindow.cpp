#include "saverwindow.h"

#include <QDirIterator>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QRandomGenerator>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

SaverWindow::SaverWindow(QScreen *screen, const QString &directory,
                         int slideSeconds, bool shuffle, QWidget *parent)
    : QWidget(parent), m_directory(directory), m_shuffle(shuffle)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setCursor(Qt::BlankCursor);
    setStyleSheet("background: black;");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setStyleSheet("color: #ddd; background: black; font-size: 22px;");
    layout->addWidget(m_label);

    winId();
    if (windowHandle()) windowHandle()->setScreen(screen);
    setGeometry(screen->geometry());
    rebuildList();
    nextImage();

    m_timer = new QTimer(this);
    m_timer->setInterval(qMax(2, slideSeconds) * 1000);
    connect(m_timer, &QTimer::timeout, this, &SaverWindow::nextImage);
    m_timer->start();
}

void SaverWindow::rebuildList()
{
    static const QStringList filters{"*.jpg", "*.jpeg", "*.png", "*.webp", "*.bmp", "*.gif"};
    QDirIterator it(m_directory, filters, QDir::Files | QDir::Readable, QDirIterator::Subdirectories);
    while (it.hasNext()) m_images << it.next();
}

void SaverWindow::nextImage()
{
    if (m_images.isEmpty()) {
        m_label->setText("WaySaver\n\nChoose an image folder in Settings");
        return;
    }
    if (m_shuffle && m_images.size() > 1) {
        int next = m_index;
        while (next == m_index) next = QRandomGenerator::global()->bounded(m_images.size());
        m_index = next;
    } else {
        m_index = (m_index + 1) % m_images.size();
    }
    const QPixmap image(m_images.at(m_index));
    if (image.isNull()) return;
    const qreal dpr = devicePixelRatioF();
    const QSize target = (size() * dpr);
    QPixmap scaled = image.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(dpr);
    m_label->setPixmap(scaled);
}

void SaverWindow::keyPressEvent(QKeyEvent *event)
{
    emit dismissRequested();
    event->accept();
}

void SaverWindow::mousePressEvent(QMouseEvent *event)
{
    emit dismissRequested();
    event->accept();
}

void SaverWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_haveMousePosition) {
        m_firstMousePosition = event->globalPosition();
        m_haveMousePosition = true;
        return;
    }
    if ((event->globalPosition() - m_firstMousePosition).manhattanLength() >= 3.0)
        emit dismissRequested();
}

