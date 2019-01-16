//
// Created by Victor Zhang on 11/9/18.
//

#ifndef BOYBOY_OBJECT_H
#define BOYBOY_OBJECT_H

#include <vector>
#include <GLES3/gl32.h>
#include <glm/detail/type_quat.hpp>
#include <glm/ext.hpp>

class Object {
public:
    Object(glm::vec3 t, glm::quat r, glm::vec3 s);
    Object() : Object(glm::vec3(), glm::quat(), glm::vec3()) {}
    ~Object() {}

    void Draw();

    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
private:
    std::vector<glm::vec3> vertices;
    std::vector<glm::uvec3> indices;
    GLuint objIndexBuffer, objVertexBuffer, objVAO;
};


#endif //BOYBOY_OBJECT_H
