#ifndef VERTEX_H
#define VERTEX_H

#include "vector3d.h"

struct Vertex {
    Vector3d position;
    Vector3d color;

    Vertex() {}
    Vertex(Vector3d pos, Vector3d col) : position(pos), color(col) {}
    Vertex(double x, double y, double z, double r, double g, double b)
        : position(x, y, z), color(r, g, b) {}
};

#endif
