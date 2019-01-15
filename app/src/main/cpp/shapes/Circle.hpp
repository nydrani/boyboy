//
// Created by Victor Zhang on 11/9/18.
//

#ifndef BOYBOY_CIRCLE_H
#define BOYBOY_CIRCLE_H

#include <vector>
#include <GLES3/gl32.h>
#include "Vector3.hpp"
#include "../glm/ext.hpp"

#define CIRCLE_DEFAULT_PARTITIONS 100

class Circle {
public:
    Circle(Vector3, float, int);
    Circle(Vector3 origin, float radius) : Circle(origin, radius, CIRCLE_DEFAULT_PARTITIONS) {}
    Circle() : Circle(Vector3(), 1.0f) {}
    ~Circle() {}
    void Draw();
private:
    std::vector<Vector3> vertices;
    Vector3 origin;
    float radius;
    int numPartitions;
    GLuint circleBuffer, circleVAO;
};


#endif //BOYBOY_CIRCLE_H
