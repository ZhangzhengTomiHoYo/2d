#ifndef BOXCOLLIDERCOMPONENT_H
#define BOXCOLLIDERCOMPONENT_H

#include <glm/glm.hpp>

struct BoxColliderComponent {
    int width;
    int height;
    glm::vec2 offset;
    // glm2 如果是0 只允许发送一个
    BoxColliderComponent(int width = 0, int height = 0, glm::vec2 offset = glm::vec2(0)) {
        this->width = width;
        this->height = height;
        this->offset = offset;
    }
};

#endif