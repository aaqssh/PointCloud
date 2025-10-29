#ifndef ROLLINGBALL_H
#define ROLLINGBALL_H

#include "vector3d.h"
#include "trianglesurface.h"
#include "vertex.h"
#include <vector>

class RollingBall {
private:
    Vector3d mPosition;
    Vector3d mVelocity;
    Vector3d mAcceleration;
    double mRadius;
    double mMass;
    TriangleSurface* mSurface;

    Vector3d oldNormal;
    int oldTriangleIndex;

    static constexpr double g = 9.81;

    std::vector<Vertex> mSphereVertices;
    void constructSphere();

public:
    RollingBall(double radius, double mass, const Vector3d& startPos);
    void setSurface(TriangleSurface* surface);
    void move(double dt);

    Vector3d getPosition() const { return mPosition; }
    Vector3d getVelocity() const { return mVelocity; }
    double getRadius() const { return mRadius; }
    const std::vector<Vertex>& getSphereVertices() const { return mSphereVertices; }
};

#endif
