#include "trianglesurface.h"
#include <fstream>
#include <iostream>
#include <cmath>

TriangleSurface::TriangleSurface() {}

void TriangleSurface::readFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return;
    }

    int numVertices, numTriangles;
    file >> numVertices;

    for (int i = 0; i < numVertices; i++) {
        double x, y, z;
        file >> x >> y >> z;
        mVertices.push_back(Vector3d(x, y, z));
    }

    file >> numTriangles;

    for (int i = 0; i < numTriangles; i++) {
        Triangle tri;
        file >> tri.vertices[0] >> tri.vertices[1] >> tri.vertices[2];
        file >> tri.neighbors[0] >> tri.neighbors[1] >> tri.neighbors[2];
        mTriangles.push_back(tri);
    }

    file.close();
    std::cout << "Loaded " << mVertices.size() << " vertices and "
              << mTriangles.size() << " triangles" << std::endl;

    constructDrawVertices();
}

void TriangleSurface::constructDrawVertices() {
    mDrawVertices.clear();

    for (const auto& tri : mTriangles) {
        Vector3d v0 = mVertices[tri.vertices[0]];
        Vector3d v1 = mVertices[tri.vertices[1]];
        Vector3d v2 = mVertices[tri.vertices[2]];

        Vector3d edge1 = v1 - v0;
        Vector3d edge2 = v2 - v0;
        Vector3d normal = edge1.cross(edge2);
        normal.normalize();

        double colorFactor = (normal.z + 1.0) * 0.5;
        Vector3d color(colorFactor * 0.5, colorFactor * 0.8, colorFactor * 0.3);

        mDrawVertices.push_back(Vertex(v0, color));
        mDrawVertices.push_back(Vertex(v1, color));
        mDrawVertices.push_back(Vertex(v2, color));
    }
}

Vector3d TriangleSurface::getNormal(int triangleIndex) const {
    if (triangleIndex < 0 || triangleIndex >= (int)mTriangles.size()) {
        return Vector3d(0, 0, 1);
    }

    const Triangle& tri = mTriangles[triangleIndex];
    Vector3d v0 = mVertices[tri.vertices[0]];
    Vector3d v1 = mVertices[tri.vertices[1]];
    Vector3d v2 = mVertices[tri.vertices[2]];

    Vector3d edge1 = v1 - v0;
    Vector3d edge2 = v2 - v0;
    Vector3d normal = edge1.cross(edge2);
    normal.normalize();

    return normal;
}

int TriangleSurface::findTriangle(const Vector3d& point) const {
    for (size_t i = 0; i < mTriangles.size(); i++) {
        const Triangle& tri = mTriangles[i];
        Vector3d v0 = mVertices[tri.vertices[0]];
        Vector3d v1 = mVertices[tri.vertices[1]];
        Vector3d v2 = mVertices[tri.vertices[2]];

        double x = point.x, y = point.y;
        double x0 = v0.x, y0 = v0.y;
        double x1 = v1.x, y1 = v1.y;
        double x2 = v2.x, y2 = v2.y;

        double denom = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
        if (std::abs(denom) < 1e-6) continue;

        double u = ((y1 - y2) * (x - x2) + (x2 - x1) * (y - y2)) / denom;
        double v = ((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) / denom;
        double w = 1.0 - u - v;

        if (u >= -0.01 && v >= -0.01 && w >= -0.01) {
            return i;
        }
    }
    return -1;
}

Vector3d TriangleSurface::getVertex(int index) const {
    if (index >= 0 && index < (int)mVertices.size()) {
        return mVertices[index];
    }
    return Vector3d(0, 0, 0);
}
