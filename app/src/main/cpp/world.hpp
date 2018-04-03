#ifndef INC_2050_WORLD_HPP
#define INC_2050_WORLD_HPP

#include "opengl.hpp"

#include <deque>
#include <list>
#include <memory>

#include <android/asset_manager.h>

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <textogl/font.hpp>
#include <textogl/static_text.hpp>

#include "ball.hpp"

/* TODO:
 * loading screen
 * lighter text on dark colored balls
 * Achievement pop-ups?
 * UI design
 */

class World
{
private:
    const float win_size = 512.0f;
    const std::size_t num_starting_balls = 2;
    std::list<Ball> balls;
    std::deque<float> last_compressions = std::deque<float>(100, 0.0f);
    float med_compression;

    // physics constants
    const float g = -200.0f; // free-fall gravitational acceleration
    const float e = 0.5f; // coefficient of collision restitution
    const float wall_damp = 0.9f; // % velocity lost when colliding with a wall

    enum class State {ONGOING, WIN, LOSE, EXTENDED} state = State::ONGOING;
    bool paused = false;

    int high_score = 0;
    int score = 0;

    glm::vec2 grav_vec{0.0f};

    glm::vec2 screen_size;
    glm::mat3 projection;
    glm::mat3 screen_projection;

    // TODO: wrap into one 'OGL stuff' struct?
    std::unique_ptr<Shader_prog> ball_prog;
    std::unique_ptr<GL_buffer> ball_vbo;

    std::unique_ptr<Shader_prog> bar_prog;
    std::unique_ptr<Quad> quad;

    AAsset * font_asset = nullptr;
    const int initial_text_size = 14;
    int text_size = initial_text_size;
    std::unique_ptr<textogl::Font_sys> font;
    std::vector<textogl::Static_text> ball_texts;

    std::unique_ptr<textogl::Font_sys> msg_font;
    std::unique_ptr<textogl::Font_sys> sub_msg_font;

    glm::vec2 text_coord_transform(const glm::vec2 & coord);

public:
    World(AAssetManager * asset_manager);
    ~World();
    World(const World &) = delete;
    World(World &&) = default;

    World & operator=(const World &) = delete;
    World & operator=(World &&) = default;

    void init();
    void destroy();
    void resize(GLsizei width, GLsizei height);
    void render();
    void pause();
    void physics_step(float dt);

    void fling(float x, float y);
    void tap(float x, float y);

    void new_game();

    void deserialize(const nlohmann::json & data);
    nlohmann::json serialize() const;
};

#endif //INC_2050_WORLD_HPP
