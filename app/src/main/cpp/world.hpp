#ifndef INC_2050_WORLD_HPP
#define INC_2050_WORLD_HPP

#include "opengl.hpp"

#include <memory>

#include <android/asset_manager.h>

#include <glm/glm.hpp>
#include <textogl/font.hpp>

class World
{
private:
    glm::vec3 bg_color{0.0f};
    glm::vec3 delta{0.2f, 0.3f, 0.5f};

    GLsizei width = 0, height = 0;
    int count = 0;

    std::unique_ptr<Shader_prog> prog;
    std::unique_ptr<Quad> quad;
    std::unique_ptr<textogl::Font_sys> font;

public:
    void init(AAssetManager * asset_manager);
    void resize(GLsizei width, GLsizei height);
    void destroy();
    void render();
    void physics_step(float dt);
};

#endif //INC_2050_WORLD_HPP
