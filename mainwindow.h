#pragma once

#include <QMainWindow>
#include <QCamera>
#include <QImageCapture>
#include <QMediaCaptureSession>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onCameraChanged(int index);
    void onStartStopClicked();
    void onTakePhotoClicked();
    void onImageCaptured(int id, const QImage &preview);

private:
    void setupCamera(const QCameraDevice &device);
    void updateCaptureButtonState();

    Ui::MainWindow *ui;

    QCamera *m_camera = nullptr;
    QMediaCaptureSession m_captureSession;
    QImageCapture *m_imageCapture = nullptr;
};
