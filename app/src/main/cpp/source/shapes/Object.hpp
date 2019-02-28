//
// Created by Victor Zhang on 11/9/18.
//

#ifndef BOYBOY_OBJECT_H
#define BOYBOY_OBJECT_H

#include <vector>
#include <GLES3/gl32.h>

#include "core/bboycore.hpp"
#include "AABB.hpp"

class Object {
public:
    Object(glm::vec3, glm::quat, glm::vec3);
    Object() : Object(glm::vec3(0.0f, 0.0f, 0.0f), glm::identity<glm::quat>(), glm::vec3(1.0f, 1.0f, 1.0f)) {
        LOGD("Being default constructed");
    }

    // move constructor
    Object(Object&& other) noexcept : translation(other.translation), rotation(other.rotation), scale(other.scale), velocity(other.velocity), color(other.color) {
        LOGD("Being moved constructed");

        isActive = other.isActive;

        child = std::move(other.child);
        parent = other.parent;

        vertices = other.vertices;
        indices = other.indices;
        objIndexBuffer = other.objIndexBuffer;
        objVertexBuffer = other.objVertexBuffer;
        objVAO = other.objVAO;
    }

    // copy constructor
    Object(const Object&) = delete;

    // copy assignment
    Object& operator=(const Object& other) = delete;

    // move assignment
    Object& operator=(Object&& other) noexcept {
        LOGD("Being moved assigned");

        if (this == &other) {
            return *this;
        }

        translation = other.translation;
        rotation = other.rotation;
        scale = other.scale;
        velocity = other.velocity;
        color = other.color;

        isActive = other.isActive;

        child = std::move(other.child);
        parent = other.parent;

        vertices = other.vertices;
        indices = other.indices;
        objIndexBuffer = other.objIndexBuffer;
        objVertexBuffer = other.objVertexBuffer;
        objVAO = other.objVAO;

        return *this;
    }
    ~Object();

    void addChild(std::unique_ptr<Object>);
    glm::mat4 TRS(glm::mat4) const;
    void Update();
    void Draw(glm::mat4, GLint, GLint) const;
    void DrawAABB(glm::mat4, GLint, GLint) const;
    void updateAABBVertices();
    bool checkCollision(const Object&) const;
    glm::mat4 getWorldTRS() const;
    AABB getAABB() const;

    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    glm::vec3 velocity;
    glm::vec4 color;
    bool isActive;
    Object* parent = nullptr;
    std::unique_ptr<Object> child;

private:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> bBoxVertices;
    std::vector<glm::uvec3> indices;
    std::vector<uint32_t> bBoxIndices;
    GLuint objIndexBuffer, objVertexBuffer, objVAO;
};


#endif //BOYBOY_OBJECT_H
