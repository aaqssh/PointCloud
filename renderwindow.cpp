#include "renderwindow.h"
#include <QDebug>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

RenderWindow::RenderWindow(QWidget *parent) : QOpenGLWidget(parent) {
    mSurface = new TriangleSurface();
    mBall = new RollingBall(0.15, 1.0, Vector3d(0.5, 0.5, 3.0));

    mRenderTimer = new QTimer(this);
    connect(mRenderTimer, &QTimer::timeout, this, QOverload<>::of(&RenderWindow::update));

    setFocusPolicy(Qt::StrongFocus);
}

RenderWindow::~RenderWindow() {
    delete mSurface;
    delete mBall;
}

void RenderWindow::initializeGL() {
    initializeOpenGLFunctions();

    qDebug() << "OpenGL Version:" << (char*)glGetString(GL_VERSION);

    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    mSurface->readFromFile("surface.txt");
    mBall->setSurface(mSurface);

    mTimeElapsed.start();
    mRenderTimer->start(16);
}

void RenderWindow::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void RenderWindow::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    setupProjectionMatrix(width(), height());

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    setupViewMatrix();

    drawTriangleSurface();
    drawBall();
}

void RenderWindow::setupProjectionMatrix(int w, int h) {
    double aspect = (double)w / (double)h;
    double fov = 45.0 * M_PI / 180.0;
    double near = 0.1;
    double far = 100.0;

    double top = near * tan(fov / 2.0);
    double right = top * aspect;

    glFrustum(-right, right, -top, top, near, far);
}

void RenderWindow::setupViewMatrix() {
    Vector3d eye = mCamera.getPosition();
    Vector3d target = mCamera.getTarget();
    Vector3d up = mCamera.getUp();

    Vector3d forward = (target - eye).normalized();
    Vector3d right = forward.cross(up).normalized();
    Vector3d newUp = right.cross(forward);

    double mat[16] = {
        right.x, newUp.x, -forward.x, 0,
        right.y, newUp.y, -forward.y, 0,
        right.z, newUp.z, -forward.z, 0,
        -right.dot(eye), -newUp.dot(eye), forward.dot(eye), 1
    };

    glMultMatrixd(mat);
}

void RenderWindow::drawTriangleSurface() {
    glBegin(GL_TRIANGLES);
    for (const auto& vertex : mSurface->getDrawVertices()) {
        glColor3d(vertex.color.x, vertex.color.y, vertex.color.z);
        glVertex3d(vertex.position.x, vertex.position.y, vertex.position.z);
    }
    glEnd();
}

void RenderWindow::drawBall() {
    Vector3d pos = mBall->getPosition();

    glPushMatrix();
    glTranslated(pos.x, pos.y, pos.z);

    glBegin(GL_TRIANGLES);
    for (const auto& vertex : mBall->getSphereVertices()) {
        glColor3d(vertex.color.x, vertex.color.y, vertex.color.z);
        glVertex3d(vertex.position.x, vertex.position.y, vertex.position.z);
    }
    glEnd();

    glPopMatrix();
}

void RenderWindow::update() {
    double dt = 0.016;
    mBall->move(dt);

    QOpenGLWidget::update();
}
