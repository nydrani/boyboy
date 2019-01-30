//
// Created by Victor Zhang on 22/1/19.
//

#include <core/bboycore.hpp>
#include "AABB.hpp"

AABB::AABB(const glm::vec4& min, const glm::vec4& max) {
    this->min = min;
    this->max = max;
}

bool AABB::overlaps(const AABB& other) const {
    if (min.x > other.max.x || max.x < other.min.x) {
//        LOGD("x doesnt overlap: a: %f %f, b: %f %f", min.x, max.x, other.min.x, other.max.x);
        return false;
    } else if (min.y > other.max.y || max.y < other.min.y) {
//        LOGD("y doesnt overlap: a: %f %f, b: %f %f", min.y, max.y, other.min.y, other.max.y);
        return false;
    } else if (min.z > other.max.z || max.z < other.min.z) {
//        LOGD("z doesnt overlap: a: %f %f, b: %f %f", min.z, max.z, other.min.z, other.max.z);
        return false;
    }

    LOGD("everything overlaps: a: %f %f, b: %f %f", min.x, max.x, other.min.x, other.max.x);
    LOGD("everything overlaps: a: %f %f, b: %f %f", min.y, max.y, other.min.y, other.max.y);
    LOGD("everything overlaps: a: %f %f, b: %f %f", min.z, max.z, other.min.z, other.max.z);
    return true;
}
