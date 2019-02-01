//
// Created by Victor Zhang on 15/1/19.
//
#define GLM_ENABLE_EXPERIMENTAL

#include <vector>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/ext.hpp>
#include <thread>
#include <sstream>
#include "core/bboycore.hpp"
#include "Object.hpp"


Object::Object(glm::vec3 translation, glm::quat rotation, glm::vec3 scale)
                            : translation(translation), rotation(rotation), scale(scale), isActive(true) {
    LOGD("Being constructed");

    // generate vertices
    vertices.emplace_back(-2, 1, 0);
    vertices.emplace_back(2, 1, 0);
    vertices.emplace_back(2, -1, 0);
    vertices.emplace_back(-2, -1, 0);

    // generate indices
    indices.emplace_back(0, 2, 1);
    indices.emplace_back(0, 3, 2);

    bBoxIndices.emplace_back(4);
    bBoxIndices.emplace_back(5);
    bBoxIndices.emplace_back(6);
    bBoxIndices.emplace_back(7);

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
    glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(*vertices.begin()) * vertices.size(), 0, GL_DYNAMIC_DRAW);

    // bind indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(*indices.begin()) * indices.size() + sizeof(*bBoxIndices.begin()) * bBoxIndices.size(), 0, GL_DYNAMIC_DRAW);

    // bind binding box vertices
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(*vertices.begin()) * vertices.size(), vertices.data());
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(*indices.begin()) * indices.size(), indices.data());
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(*indices.begin()) * indices.size(), sizeof(*bBoxIndices.begin()) * bBoxIndices.size(), bBoxIndices.data());

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
    // update aabb vertices
    updateAABBVertices();

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

void Object::Draw(glm::mat4 worldMat, GLint mvpMatrixLoc, GLint colorVecLoc) const {
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

void Object::DrawAABB(glm::mat4 worldMat, GLint mvpMatrixLoc, GLint colorVecLoc) const {
    // draw in respect to world
    glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(worldMat));

    glBindVertexArray(objVAO);

    // buffer data into the array buffer before drawing
    glBindBuffer(GL_ARRAY_BUFFER, objVertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(*vertices.begin()) * vertices.size(), sizeof(*bBoxVertices.begin()) * bBoxVertices.size(), bBoxVertices.data());

    // draw bounding box
    size_t offset = 6;
    glUniform4fv(colorVecLoc, 1, glm::value_ptr(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
    glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_INT, (void*)(offset * sizeof(GLuint)));

    glBindVertexArray(0);

    if (child != nullptr) {
        child->DrawAABB(worldMat, mvpMatrixLoc, colorVecLoc);
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
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

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

void Object::updateAABBVertices() {
    // draw stuff here
    AABB myAABB = getAABB();

    // calculate box for AABB
    glm::vec4 diffVec = myAABB.max - myAABB.min;
    glm::vec3 topLeftPos = glm::vec3(myAABB.min.x, myAABB.min.y + diffVec.y, myAABB.min.z);
    glm::vec3 bottomRightPos = glm::vec3(myAABB.min.x + diffVec.x, myAABB.min.y, myAABB.min.z);

    // store vertices in vector
    bBoxVertices.clear();
    bBoxVertices.emplace_back(topLeftPos);
    bBoxVertices.emplace_back(myAABB.max);
    bBoxVertices.emplace_back(bottomRightPos);
    bBoxVertices.emplace_back(myAABB.min);
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
