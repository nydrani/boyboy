//
// Created by Victor Zhang on 11/9/18.
//

#ifndef BOYBOY_VECTOR3_H
#define BOYBOY_VECTOR3_H

#include <cmath>

struct Vector3
{
    union
    {
        struct
        {
            float x,y,z;
        };
        float v[3];
    };

    Vector3(float x, float y, float z) : x(x), y(y), z(z) {};
    Vector3() : Vector3(0.0f, 0.0f, 0.0f) {};
};

#endif //BOYBOY_VECTOR3_H
