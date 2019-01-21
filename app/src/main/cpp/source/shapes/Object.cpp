//
// Created by Victor Zhang on 15/1/19.
//
#define GLM_ENABLE_EXPERIMENTAL

#include <vector>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/ext.hpp>
#include "core/bboycore.hpp"
#include "Object.hpp"


Object::Object(glm::vec3 translation, glm::quat rotation, glm::vec3 scale)
                            : translation(translation), rotation(rotation), scale(scale) {
    LOGD("Being constructed");

    // generate vertices
    vertices.emplace_back(-2, 1, 0);
    vertices.emplace_back(2, 1, 0);
    vertices.emplace_back(2, -1, 0);
    vertices.emplace_back(-2, -1, 0);

    // generate indices
    indices.emplace_back(0, 2, 1);
    indices.emplace_back(0, 3, 2);

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

Object::~Object() {
    for (std::unique_ptr<Object> current = std::move(child);
         current;
         current = std::move(current->child));
}

void Object::addChild(std::unique_ptr<Object> child) {
    child->parent = this;
    this->child = std::move(child);
}

glm::mat4 Object::TRS(glm::mat4 curWorldModel) {
    glm::mat4 orig = curWorldModel;

    float angle = glm::angle(this->rotation);
    glm::vec3 axis = glm::axis(this->rotation);

    glm::mat4 TMat = glm::translate(this->translation);
    glm::mat4 RMat = glm::rotate(angle, axis);
//    glm::mat4 RMat = glm::toMat4(this->rotation);
    glm::mat4 SMat = glm::scale(this->scale);

    orig = orig * TMat * RMat * SMat;

    return orig;
}

void Object::Update() {
    // update the rotation here
    // @TODO may need to normalise the rotation here (maths in glm is dodge)
    this->rotation = glm::rotate(this->rotation, glm::radians(1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    this->rotation = glm::normalize(this->rotation);

    LOGD("%s", glm::to_string(glm::axis(this->rotation)).c_str());
    LOGD("%f", glm::angle(this->rotation));

    if (child != nullptr) {
        child->Update();
    }
}

void Object::Draw(glm::mat4 worldMat, GLint mvpMatrixLoc) {
    // rotate stuff around
    glm::mat4 localMat = TRS(worldMat);
    glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(localMat));

    // draw a square of 2x2 unit size at the current spot
    glBindVertexArray(objVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // draw children
    if (child != nullptr) {
        child->Draw(localMat, mvpMatrixLoc);
    }
}