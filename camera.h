#ifndef CAMERA_H
#define CAMERA_H

#include "vector3d.h"

class Camera {
private:
    Vector3d position;
    Vector3d target;
    Vector3d up;

public:
    Camera()
        // Changed from (5, 5, 8) to show triangles properly
        : position(3, 3, 4),      // CHANGED: Different viewing angle
        target(1, 1, 1.5),      // CHANGED: Look at pyramid center
        up(0, 0, 1)
    {}

    Vector3d getPosition() const { return position; }
    Vector3d getTarget() const { return target; }
    Vector3d getUp() const { return up; }

    void setPosition(const Vector3d& pos) { position = pos; }
    void setTarget(const Vector3d& tgt) { target = tgt; }
};

#endif
