//
// Created by Victor Zhang on 22/1/19.
//

#ifndef BOYBOY_AABB_HPP
#define BOYBOY_AABB_HPP

#include <glm/glm.hpp>

class AABB {
public:
    AABB() = default;
    AABB(const glm::vec4&, const glm::vec4&);
    ~AABB() = default;

    bool overlaps(const AABB&) const;

    glm::vec4 min;
    glm::vec4 max;
};

#endif //BOYBOY_AABB_HPP
