#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

// Minimal glm-like header replacement (vec2/vec3, dot, cross, normalize, length)
namespace glm {
    struct vec2 {
        float x, y;
        vec2(): x(0), y(0) {}
        vec2(float a, float b): x(a), y(b) {}
    };
    struct vec3 {
        float x, y, z;
        vec3(): x(0), y(0), z(0) {}
        vec3(float a, float b, float c): x(a), y(b), z(c) {}
        vec3 operator+(const vec3& o) const { return vec3(x+o.x, y+o.y, z+o.z); }
        vec3 operator-(const vec3& o) const { return vec3(x-o.x, y-o.y, z-o.z); }
        vec3 operator*(float s) const { return vec3(x*s, y*s, z*s); }
        vec3 operator/(float s) const { assert(s!=0); return vec3(x/s, y/s, z/s); }
        vec3& operator+=(const vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }
        vec3& operator-=(const vec3& o) { x-=o.x; y-=o.y; z-=o.z; return *this; }
    };
    inline float dot(const vec3& a, const vec3& b) {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    }
    inline vec3 cross(const vec3& a, const vec3& b) {
        return vec3(a.y*b.z - a.z*b.y,
                    a.z*b.x - a.x*b.z,
                    a.x*b.y - a.y*b.x);
    }
    inline float length(const vec3& v) {
        return std::sqrt(dot(v,v));
    }
    inline vec3 normalize(const vec3& v) {
        float L = length(v);
        if (L == 0.0f) return vec3(0,0,0);
        return v / L;
    }
    // allow glm::length(vec3) syntax used in code
} // namespace glm

using namespace std;
using glm::vec3;
using glm::vec2;
using glm::normalize;
using glm::dot;
using glm::cross;

// Constants
const float G = 9.81f;        // Gravity acceleration
const float FRICTION = 0.5f;  // Friction coefficient
const float BALL_RADIUS = 0.1f;
const float TIME_STEP = 0.01f;
const int MAX_ITERATIONS = 10000;

// Triangle structure
struct Triangle {
    int vertices[3];  // Vertex indices
    int neighbors[3]; // Neighbor triangle indices (-1 if edge)
};

// Vertex structure
struct Vertex {
    vec3 position;
};

// Ball state structure (from book section 9.2.4)
struct BallState {
    vec3 position;
    vec3 velocity;
    int currentTriangle;
};

class BallSimulation {
private:
    vector<Vertex> vertices;
    vector<Triangle> triangles;
    BallState ball;

public:
    BallSimulation() {
        initializeTriangulation();
        initializeBall();
    }

    void initializeTriangulation() {
        // Initialize vertices
        vertices.resize(12);
        vertices[0].position = vec3(0.0f, 0.0f, 0.0f);
        vertices[1].position = vec3(2.0f, 0.0f, 0.0f);
        vertices[2].position = vec3(4.0f, 0.0f, 0.0f);
        vertices[3].position = vec3(0.0f, 2.0f, 0.0f);
        vertices[4].position = vec3(2.0f, 2.0f, 0.0f);
        vertices[5].position = vec3(4.0f, 2.0f, 0.0f);
        vertices[6].position = vec3(0.0f, 4.0f, 0.0f);
        vertices[7].position = vec3(2.0f, 4.0f, 0.0f);
        vertices[8].position = vec3(4.0f, 4.0f, 0.0f);
        vertices[9].position = vec3(1.0f, 1.0f, 0.5f);
        vertices[10].position = vec3(3.0f, 1.0f, 0.5f);
        vertices[11].position = vec3(2.0f, 3.0f, 0.3f);

        // Initialize triangles with indexing and neighbor information
        triangles.resize(4);

        // Triangle 0
        triangles[0].vertices[0] = 0;
        triangles[0].vertices[1] = 1;
        triangles[0].vertices[2] = 3;
        triangles[0].neighbors[0] = -1;  // No neighbor across edge 0-1
        triangles[0].neighbors[1] = 1;   // Triangle 1 across edge 1-3
        triangles[0].neighbors[2] = -1;  // No neighbor across edge 3-0

        // Triangle 1
        triangles[1].vertices[0] = 1;
        triangles[1].vertices[1] = 4;
        triangles[1].vertices[2] = 3;
        triangles[1].neighbors[0] = 0;   // Triangle 0
        triangles[1].neighbors[1] = 2;   // Triangle 2
        triangles[1].neighbors[2] = -1;  // Boundary

        // Triangle 2
        triangles[2].vertices[0] = 1;
        triangles[2].vertices[1] = 2;
        triangles[2].vertices[2] = 5;
        triangles[2].neighbors[0] = -1;  // Boundary
        triangles[2].neighbors[1] = 3;   // Triangle 3
        triangles[2].neighbors[2] = 1;   // Triangle 1

        // Triangle 3
        triangles[3].vertices[0] = 1;
        triangles[3].vertices[1] = 5;
        triangles[3].vertices[2] = 4;
        triangles[3].neighbors[0] = 2;   // Triangle 2
        triangles[3].neighbors[1] = -1;  // Boundary
        triangles[3].neighbors[2] = 1;   // Triangle 1
    }

    void initializeBall() {
        ball.position = vec3(1.0f, 0.5f, 0.3f);
        ball.velocity = vec3(0.1f, 0.0f, 0.0f);
        ball.currentTriangle = 0;
    }

    // Calculate barycentric coordinates (from book section 5.1.7)
    vec3 getBarycentricCoordinates(const vec3& P, int triangleIdx) {
        const Triangle& tri = triangles[triangleIdx];
        const vec3& v0 = vertices[tri.vertices[0]].position;
        const vec3& v1 = vertices[tri.vertices[1]].position;
        const vec3& v2 = vertices[tri.vertices[2]].position;

        vec3 e0 = vec3(v1.x - v0.x, v1.y - v0.y, 0.0f);
        vec3 e1 = vec3(v2.x - v0.x, v2.y - v0.y, 0.0f);
        vec3 e2 = vec3(P.x - v0.x, P.y - v0.y, 0.0f);

        float d00 = dot(e0, e0);
        float d01 = dot(e0, e1);
        float d11 = dot(e1, e1);
        float d20 = dot(e2, e0);
        float d21 = dot(e2, e1);

        float denom = d00 * d11 - d01 * d01;
        if (fabs(denom) < 1e-8f) {
            return vec3(-1.0f,-1.0f,-1.0f);
        }
        float v = (d11 * d20 - d01 * d21) / denom;
        float w = (d00 * d21 - d01 * d20) / denom;
        float u = 1.0f - v - w;

        return vec3(u, v, w);
    }

    // Get normal vector to triangle (from book section 6.2.1)
    vec3 getTriangleNormal(int triangleIdx) {
        const Triangle& tri = triangles[triangleIdx];
        const vec3& v0 = vertices[tri.vertices[0]].position;
        const vec3& v1 = vertices[tri.vertices[1]].position;
        const vec3& v2 = vertices[tri.vertices[2]].position;

        vec3 e0 = vec3(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
        vec3 e1 = vec3(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
        return normalize(cross(e0, e1));
    }

    // Get surface height at ball position
    float getSurfaceHeight(const vec3& pos, int triangleIdx) {
        const Triangle& tri = triangles[triangleIdx];
        const vec3& v0 = vertices[tri.vertices[0]].position;
        const vec3& v1 = vertices[tri.vertices[1]].position;
        const vec3& v2 = vertices[tri.vertices[2]].position;

        vec3 bary = getBarycentricCoordinates(vec3(pos.x, pos.y, 0.0f), triangleIdx);

        return bary.x * v0.z + bary.y * v1.z + bary.z * v2.z;
    }

    // Topological search (from book section 6.2.8)
    int findContainingTriangle(const vec3& P) {
        int current = ball.currentTriangle;
        bool found = false;
        int iterations = 0;

        while (!found && iterations < 100) {
            vec3 bary = getBarycentricCoordinates(P, current);

            if (bary.x >= -0.001f && bary.y >= -0.001f && bary.z >= -0.001f) {
                found = true;
                break;
            }

            // Find minimum barycentric coordinate
            float minBary = bary.x;
            int nextEdge = 0;

            if (bary.y < minBary) {
                minBary = bary.y;
                nextEdge = 1;
            }
            if (bary.z < minBary) {
                minBary = bary.z;
                nextEdge = 2;
            }

            int neighbor = triangles[current].neighbors[nextEdge];
            if (neighbor == -1) {
                // Hit boundary
                break;
            }

            current = neighbor;
            iterations++;
        }

        return current;
    }

    // Update ball position and velocity (from book section 9.5, 9.6)
    void updatePhysics() {
        // Find which triangle the ball is on
        ball.currentTriangle = findContainingTriangle(ball.position);

        // Get surface normal
        vec3 normal = getTriangleNormal(ball.currentTriangle);

        // Gravity acceleration
        vec3 gravity(0.0f, 0.0f, -G);

        // Project gravity onto plane (from book section 9.3)
        vec3 gravityComponent = gravity - normalize(normal) * dot(gravity, normal);

        // Apply friction (from book section 9.2.5)
        vec3 frictionForce = vec3(-FRICTION * gravityComponent.x,
                                  -FRICTION * gravityComponent.y,
                                  -FRICTION * gravityComponent.z);

        // Total acceleration
        vec3 acceleration = vec3(gravityComponent.x + frictionForce.x,
                                 gravityComponent.y + frictionForce.y,
                                 gravityComponent.z + frictionForce.z);

        // Update velocity (Euler method)
        ball.velocity += acceleration * TIME_STEP;

        // Update position
        ball.position += ball.velocity * TIME_STEP;

        // Keep ball above surface
        float surfaceHeight = getSurfaceHeight(ball.position, ball.currentTriangle);
        if (ball.position.z < surfaceHeight + BALL_RADIUS) {
            ball.position.z = surfaceHeight + BALL_RADIUS;

            // Dampen velocity perpendicular to surface
            vec3 velComponent = normalize(normal) * dot(ball.velocity, normal);
            ball.velocity -= velComponent * 0.5f;
        }
    }

    void run(int iterations) {
        cout << "Ball Rolling Simulation (Based on Book Section 9)" << endl;
        cout << "Initial position: (" << ball.position.x << ", "
            << ball.position.y << ", " << ball.position.z << ")" << endl;
        cout << "Gravity: " << G << " m/s^2" << endl;
        cout << "Friction coefficient: " << FRICTION << endl << endl;

        for (int i = 0; i < iterations; ++i) {
            updatePhysics();

            if (i % 100 == 0) {
                cout << "Iteration " << i << ": Position ("
                    << ball.position.x << ", "
                    << ball.position.y << ", "
                    << ball.position.z << ") Velocity magnitude: "
                    << glm::length(ball.velocity) << endl;
            }

            // Stop if velocity becomes negligible
            if (glm::length(ball.velocity) < 0.001f) {
                cout << "Ball has come to rest at iteration " << i << endl;
                break;
            }
        }

        cout << "\nFinal position: (" << ball.position.x << ", "
            << ball.position.y << ", " << ball.position.z << ")" << endl;
        cout << "Final velocity: (" << ball.velocity.x << ", "
            << ball.velocity.y << ", " << ball.velocity.z << ")" << endl;
    }
};

int main() {
    BallSimulation simulation;
    simulation.run(MAX_ITERATIONS);

    return 0;
}
