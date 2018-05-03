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
 * Achievement pop-ups?
 * gravity sensor mode
 * ball colors set in XML
 * background changes w/ theme
 * icon
 * * Kotlin?
 * don't pause on start-up
 * remove (or at least disable logging)
 * surface disp seems to have crept up again / really wrong on landscape
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

    std::unique_ptr<Shader_prog> ball_prog;
    std::unique_ptr<GL_buffer> ball_vbo;

    const std::size_t num_ball_attrs = 8;
    std::vector<float> ball_data = std::vector<float>(64 * num_ball_attrs * 3); // scratch buffer for ball data

    AAsset * font_asset = nullptr;
    const int initial_text_size = 14;
    int text_size = initial_text_size;
    std::unique_ptr<textogl::Font_sys> font;
    std::vector<textogl::Static_text> ball_texts;

    glm::vec2 text_coord_transform(const glm::vec2 & coord);

    std::unordered_map<std::string, std::string> resource_strings;
    std::string get_str(const std::string & id);

    std::vector<float> physx_times;

    void render_balls();

public:
    World(AAssetManager * asset_manager);
    ~World();
    World(const World &) = delete;
    World(World &&) = default;

    World & operator=(const World &) = delete;
    World & operator=(World &&) = default;

    void init();
    void destroy();
    void pause();
    bool is_paused() const;
    void unpause();
    void resize(GLsizei width, GLsizei height);
    bool render();
    void physics_step(float dt);

    void fling(float x, float y);

    void new_game();

    struct UI_data
    {
        int score;
        int high_score;
        float grav_angle;
        int pressure;
    };
    UI_data get_ui_data();

    void deserialize(const nlohmann::json & data);
    nlohmann::json serialize() const;
};

#endif //INC_2050_WORLD_HPP
