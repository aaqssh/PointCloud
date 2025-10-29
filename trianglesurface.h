#ifndef TRIANGLESURFACE_H
#define TRIANGLESURFACE_H

#include <vector>
#include <array>
#include <string>
#include "vector3d.h"
#include "vertex.h"

struct Triangle {
    std::array<int, 3> vertices;
    std::array<int, 3> neighbors;
};

class TriangleSurface {
private:
    std::vector<Vector3d> mVertices;
    std::vector<Triangle> mTriangles;
    std::vector<Vertex> mDrawVertices;

public:
    TriangleSurface();
    void readFromFile(const std::string& filename);
    void constructDrawVertices();

    Vector3d getNormal(int triangleIndex) const;
    int findTriangle(const Vector3d& point) const;

    const std::vector<Vector3d>& getVertices() const { return mVertices; }
    const std::vector<Triangle>& getTriangles() const { return mTriangles; }
    const std::vector<Vertex>& getDrawVertices() const { return mDrawVertices; }
    Vector3d getVertex(int index) const;
};

#endif
