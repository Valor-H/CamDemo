#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCameraDevice>
#include <QMediaDevices>
#include <QVideoWidget>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Connect the capture session to the viewfinder widget
    m_captureSession.setVideoOutput(ui->viewfinder);

    // Populate the camera selector with all available video input devices
    const auto cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &device : cameras) {
        ui->cameraComboBox->addItem(device.description(), QVariant::fromValue(device));
    }

    if (cameras.isEmpty()) {
        ui->statusLabel->setText(tr("No cameras found"));
        ui->startStopButton->setEnabled(false);
        ui->captureButton->setEnabled(false);
        return;
    }

    connect(ui->cameraComboBox, &QComboBox::currentIndexChanged,
            this, &MainWindow::onCameraChanged);
    connect(ui->startStopButton, &QPushButton::clicked,
            this, &MainWindow::onStartStopClicked);
    connect(ui->captureButton, &QPushButton::clicked,
            this, &MainWindow::onTakePhotoClicked);

    // Start with the default (first) camera
    onCameraChanged(0);
}

MainWindow::~MainWindow()
{
    if (m_camera)
        m_camera->stop();
    delete ui;
}

void MainWindow::onCameraChanged(int index)
{
    if (index < 0)
        return;

    const QCameraDevice device =
        ui->cameraComboBox->itemData(index).value<QCameraDevice>();
    setupCamera(device);
}

void MainWindow::setupCamera(const QCameraDevice &device)
{
    // Tear down any previously active camera and capture object
    if (m_camera) {
        m_camera->stop();
        delete m_camera;
        m_camera = nullptr;
    }
    delete m_imageCapture;
    m_imageCapture = nullptr;

    // Build and wire up the new camera
    m_camera = new QCamera(device, this);
    m_captureSession.setCamera(m_camera);

    m_imageCapture = new QImageCapture(this);
    m_captureSession.setImageCapture(m_imageCapture);

    connect(m_camera, &QCamera::activeChanged, this, [this](bool active) {
        ui->startStopButton->setText(active ? tr("Stop") : tr("Start"));
        ui->statusLabel->setText(active ? tr("Camera active") : tr("Camera stopped"));
        updateCaptureButtonState();
    });

    connect(m_imageCapture, &QImageCapture::readyForCaptureChanged,
            this, [this](bool) { updateCaptureButtonState(); });

    connect(m_imageCapture, &QImageCapture::imageCaptured,
            this, &MainWindow::onImageCaptured);

    connect(m_imageCapture, &QImageCapture::errorOccurred,
            this, [this](int /*id*/, QImageCapture::Error /*error*/,
                         const QString &errorString) {
        QMessageBox::warning(this, tr("Capture Error"), errorString);
    });

    ui->statusLabel->setText(tr("Camera ready"));
    ui->startStopButton->setText(tr("Start"));
    updateCaptureButtonState();

    m_camera->start();
}

void MainWindow::onStartStopClicked()
{
    if (!m_camera)
        return;

    if (m_camera->isActive())
        m_camera->stop();
    else
        m_camera->start();
}

void MainWindow::onTakePhotoClicked()
{
    if (!m_imageCapture || !m_imageCapture->isReadyForCapture())
        return;

    const QString picturesPath =
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QString filename =
        picturesPath + "/CamDemo_"
        + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".jpg";

    m_imageCapture->captureToFile(filename);
    statusBar()->showMessage(tr("Saving photo to: %1").arg(filename), 4000);
}

void MainWindow::onImageCaptured(int /*id*/, const QImage &preview)
{
    statusBar()->showMessage(
        tr("Photo captured (%1 x %2)").arg(preview.width()).arg(preview.height()),
        3000);
}

void MainWindow::updateCaptureButtonState()
{
    const bool canCapture = m_camera && m_camera->isActive()
                            && m_imageCapture
                            && m_imageCapture->isReadyForCapture();
    ui->captureButton->setEnabled(canCapture);
}
