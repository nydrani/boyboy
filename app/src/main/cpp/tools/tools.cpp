//
// Created by Victor Zhang on 13/9/18.
//

#include <limits>
#include <cmath>
#include "core/bboycore.hpp"
#include "tools.hpp"

float degToRads(float degrees) {
    return degrees * M_PI_FLOAT / 180;
}

bool almostEquals(float a, float b) {
    return std::fabs(a - b) < std::numeric_limits<double>::epsilon();
}