#include "rollingball.h"
#include <iostream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

RollingBall::RollingBall(double radius, double mass, const Vector3d& startPos)
    : mRadius(radius), mMass(mass), mPosition(startPos),
    mVelocity(0, 0, 0), oldNormal(0, 0, 1), oldTriangleIndex(0) {
    constructSphere();
}

void RollingBall::setSurface(TriangleSurface* surface) {
    mSurface = surface;
}

void RollingBall::move(double dt) {
    if (!mSurface) return;

    int currentTriangle = mSurface->findTriangle(mPosition);

    if (currentTriangle == -1) {
        std::cout << "Ball not on surface!" << std::endl;
        return;
    }

    Vector3d normal = mSurface->getNormal(currentTriangle);

    double nz = normal.z;
    if (std::abs(nz) < 1e-6) nz = 1e-6;

    mAcceleration.x = g * normal.x / nz;
    mAcceleration.y = g * normal.y / nz;
    mAcceleration.z = g * (nz * nz - 1.0);

    mVelocity.x += mAcceleration.x * dt;
    mVelocity.y += mAcceleration.y * dt;
    mVelocity.z += mAcceleration.z * dt;

    mPosition.x += mVelocity.x * dt;
    mPosition.y += mVelocity.y * dt;
    mPosition.z += mVelocity.z * dt;

    if (currentTriangle != oldTriangleIndex) {
        Vector3d collisionNormal = oldNormal.cross(normal);
        collisionNormal.normalize();

        double vn = mVelocity.dot(collisionNormal);
        if (vn < 0) {
            mVelocity = mVelocity - collisionNormal * vn * 0.5;
        }

        oldTriangleIndex = currentTriangle;
    }

    oldNormal = normal;
}

void RollingBall::constructSphere() {
    mSphereVertices.clear();

    int slices = 16;
    int stacks = 16;

    for (int i = 0; i < stacks; ++i) {
        double theta1 = i * M_PI / stacks;
        double theta2 = (i + 1) * M_PI / stacks;

        for (int j = 0; j < slices; ++j) {
            double phi1 = j * 2 * M_PI / slices;
            double phi2 = (j + 1) * 2 * M_PI / slices;

            Vector3d v0(mRadius * sin(theta1) * cos(phi1),
                        mRadius * sin(theta1) * sin(phi1),
                        mRadius * cos(theta1));
            Vector3d v1(mRadius * sin(theta2) * cos(phi1),
                        mRadius * sin(theta2) * sin(phi1),
                        mRadius * cos(theta2));
            Vector3d v2(mRadius * sin(theta2) * cos(phi2),
                        mRadius * sin(theta2) * sin(phi2),
                        mRadius * cos(theta2));
            Vector3d v3(mRadius * sin(theta1) * cos(phi2),
                        mRadius * sin(theta1) * sin(phi2),
                        mRadius * cos(theta1));

            Vector3d color(0.9, 0.1, 0.1);

            mSphereVertices.push_back(Vertex(v0, color));
            mSphereVertices.push_back(Vertex(v1, color));
            mSphereVertices.push_back(Vertex(v2, color));

            mSphereVertices.push_back(Vertex(v0, color));
            mSphereVertices.push_back(Vertex(v2, color));
            mSphereVertices.push_back(Vertex(v3, color));
        }
    }
}
