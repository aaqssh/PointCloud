#ifndef RENDERWINDOW_H
#define RENDERWINDOW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QElapsedTimer>
#include "trianglesurface.h"
#include "rollingball.h"
#include "camera.h"

class RenderWindow : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    RenderWindow(QWidget *parent = nullptr);
    ~RenderWindow();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    TriangleSurface* mSurface;
    RollingBall* mBall;
    Camera mCamera;

    QTimer* mRenderTimer;
    QElapsedTimer mTimeElapsed;

    void drawTriangleSurface();
    void drawBall();
    void setupViewMatrix();
    void setupProjectionMatrix(int w, int h);

private slots:
    void update();
};

#endif
