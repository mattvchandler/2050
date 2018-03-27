#ifndef INC_2050_WORLD_HPP
#define INC_2050_WORLD_HPP

#include <glm/glm.hpp>

class World
{
private:
    glm::vec3 bg_color;
    glm::vec3 delta{0.2f, 0.3f, 0.5f};

public:
    void init();
    void render();
    void physics_step(float dt);
};

#endif //INC_2050_WORLD_HPP
