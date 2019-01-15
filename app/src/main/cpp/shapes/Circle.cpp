//
// Created by Victor Zhang on 11/9/18.
//

#include <cmath>
#include "core/bboycore.hpp"
#include "tools/tools.hpp"

#include "Circle.hpp"

Circle::Circle(Vector3 origin, float radius, int partitions)
{
    // setup circle
    this->origin = origin;
    this->radius = radius;
    this->numPartitions = partitions;

    // generate vertices with partitions
    // start at [x, y, z]
    // for 0 to partitions -> x = radius * sin(partition angle) + origin.x
    //                        y = radius * cos(partition angle) + origin.y
    //                        z = z
    // add to partition list

    // x = r * sin(theta)
    // y = r * cos(theta)

    float partAngle = 360.0f / numPartitions;

    for (int i = 0; i < numPartitions; ++i) {
        float angle = degToRads(i * partAngle);
        float x = radius * sin(angle);
        float y = radius * cos(angle);

        vertices.push_back(Vector3(x, y, 0.0f));
    }


    // setup opengl
    // generate buffers
    glGenBuffers(1, &circleBuffer);
    glGenVertexArrays(1, &circleVAO);

    // bind circle
    glBindVertexArray(circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, circleBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(*vertices.begin()) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(POS_ATTRIB);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Circle::Draw()
{
    // draw circle loop at (0, 0)
    glBindVertexArray(circleVAO);
    glDrawArrays(GL_LINE_LOOP, 0, numPartitions);
    glBindVertexArray(0);
}