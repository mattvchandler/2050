#ifndef INC_2050_WORLD_HPP
#define INC_2050_WORLD_HPP

#include "opengl.hpp"

#include <memory>

#include <glm/glm.hpp>

class World
{
private:
    glm::vec3 bg_color;
    glm::vec3 delta{0.2f, 0.3f, 0.5f};

    std::unique_ptr<Shader_prog> prog;
    std::unique_ptr<Quad> quad;

public:
    void init();
    void resize(GLsizei width, GLsizei height);
    void destroy();
    void render();
    void physics_step(float dt);
};

#endif //INC_2050_WORLD_HPP
