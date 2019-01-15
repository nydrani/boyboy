//
// Created by Victor Zhang on 15/1/19.
//

#include <vector>
#include "core/bboycore.hpp"

#include "Object.hpp"

Object::Object(glm::vec3 translation, glm::quat rotation, glm::vec3 scale) {
    // setup obj
    this->translation = translation;
    this->rotation = rotation;
    this->scale = scale;

    // generate vertices
    vertices.push_back({-2, 2, 0});
    vertices.push_back({2, 2, 0});
    vertices.push_back({2, -2, 0});
    vertices.push_back({-2, -2, 0});

    // generate indices
    indices.push_back({0, 2, 1});
    indices.push_back({0, 3, 2});

    // setup opengl
    // generate buffers
    glGenBuffers(1, &objIndexBuffer);
    glGenBuffers(1, &objVertexBuffer);
    glGenVertexArrays(1, &objVAO);

    // Setup VAO
    glBindVertexArray(objVAO);

    // bind vertices
    glBindBuffer(GL_ARRAY_BUFFER, objVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(*vertices.begin()) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    // bind indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(*indices.begin()) * indices.size(), indices.data(), GL_STATIC_DRAW);

    // @TODO (find out what this does)
    // no idea what this does (seems to enable the attribute (vPosition)in the shader) [to allow binding into it]
    glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(POS_ATTRIB);

    // finish VAO
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Object::Draw() {
    // draw a square of 2x2 unit size at the current spot
    glBindVertexArray(objVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}