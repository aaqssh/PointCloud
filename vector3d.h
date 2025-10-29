#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <cmath>
#include <iostream>

class Vector3d {
public:
    double x, y, z;

    Vector3d(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}

    Vector3d operator+(const Vector3d& v) const {
        return Vector3d(x + v.x, y + v.y, z + v.z);
    }

    Vector3d operator-(const Vector3d& v) const {
        return Vector3d(x - v.x, y - v.y, z - v.z);
    }

    Vector3d operator*(double s) const {
        return Vector3d(x * s, y * s, z * s);
    }

    Vector3d operator/(double s) const {
        return Vector3d(x / s, y / s, z / s);
    }

    double dot(const Vector3d& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    Vector3d cross(const Vector3d& v) const {
        return Vector3d(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
            );
    }

    double length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    void normalize() {
        double len = length();
        if (len > 0) {
            x /= len;
            y /= len;
            z /= len;
        }
    }

    Vector3d normalized() const {
        Vector3d result = *this;
        result.normalize();
        return result;
    }

    friend std::ostream& operator<<(std::ostream& os, const Vector3d& v) {
        os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }
};

#endif
