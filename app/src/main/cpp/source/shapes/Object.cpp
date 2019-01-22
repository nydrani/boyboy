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

    // set color to black
    color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

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

glm::mat4 Object::TRS(glm::mat4 curWorldModel) const {
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
    //this->rotation = glm::rotate(this->rotation, glm::radians(1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //this->rotation = glm::normalize(this->rotation);

    // LOGD("%s", glm::to_string(glm::axis(this->rotation)).c_str());
    // LOGD("%f", glm::angle(this->rotation));

    if (child != nullptr) {
        child->Update();
    }
}

void Object::Draw(glm::mat4 worldMat, GLint mvpMatrixLoc, GLint colorVecLoc) {
    // rotate stuff around
    glm::mat4 localMat = TRS(worldMat);
    glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(localMat));
    glUniform4fv(colorVecLoc, 1, glm::value_ptr(color));

    // draw a square of 2x2 unit size at the current spot
    glBindVertexArray(objVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // draw children
    if (child != nullptr) {
        child->Draw(localMat, mvpMatrixLoc, colorVecLoc);
    }
}

glm::mat4 Object::getWorldTRS() const {
    glm::mat4 worldTRS = TRS(glm::mat4(1.0f));

    if (this->parent != nullptr) {
        worldTRS = this->parent->getWorldTRS() * worldTRS;
    }

    return worldTRS;
}

AABB Object::getAABB() const {
    // generate bounding box
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::min();
    float maxY = std::numeric_limits<float>::min();
    float maxZ = std::numeric_limits<float>::min();

    glm::mat4 worldMat = getWorldTRS();

    for (auto& vertex : vertices) {
        // generate min bounding box
        glm::vec4 transformedVec = worldMat * glm::vec4(vertex.x, vertex.y, vertex.z, 1.0f);
        if (transformedVec.x < minX) {
            minX = transformedVec.x;
        }
        if (transformedVec.y < minY) {
            minY = transformedVec.y;
        }
        if (transformedVec.z < minZ) {
            minZ = transformedVec.z;
        }

        // generate max bounding box
        if (transformedVec.x > maxX) {
            maxX = transformedVec.x;
        }
        if (transformedVec.y > maxY) {
            maxY = transformedVec.y;
        }
        if (transformedVec.z > maxZ) {
            maxZ = transformedVec.z;
        }
    }
    return AABB(glm::vec4(minX, minY, minZ, 1.0f), glm::vec4(maxX, maxY, maxZ, 1.0f));
}

bool Object::checkCollision(const Object& other) const {
    if (this == &other) {
        return false;
    }

    AABB worldAABB = getAABB();
    bool collided = worldAABB.overlaps(other.getAABB());

    if (collided) {
        //LOGD("collided with something else: %p", &other);
    }

    return collided;
}
