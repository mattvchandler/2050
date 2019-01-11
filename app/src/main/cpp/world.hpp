// Copyright 2019 Matthew Chandler

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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

    enum class State {ONGOING, LOSE, EXTENDED} state = State::ONGOING;
    bool paused = false;

    int high_score = 0;
    int score = 0;
    int next_achievement_size = 3;

    bool gravity_mode = false;
    glm::vec2 grav_vec{0.0f};
    float grav_ref_angle = 0.0f;

    glm::vec2 screen_size;
    glm::mat3 projection;

    std::unique_ptr<Shader_prog> ball_prog;
    std::unique_ptr<GL_buffer> ball_vbo;

    const std::size_t num_ball_attrs = 8;
    std::vector<float> ball_data = std::vector<float>(64 * num_ball_attrs * 3); // scratch buffer for ball data

    AAsset * font_asset = nullptr;
    AAsset * vert_shader_asset = nullptr;
    AAsset * frag_shader_asset = nullptr;
    const int initial_text_size = 14;
    int text_size = initial_text_size;
    std::unique_ptr<textogl::Font_sys> font;
    std::vector<textogl::Static_text> ball_texts;

    glm::vec2 text_coord_transform(const glm::vec2 & coord);

    glm::vec4 bg_color;
    std::vector<glm::vec4> ball_colors;

    void render_balls();

public:
    World(AAssetManager * asset_manager, bool gravity_mode);
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
    void physics_step(float dt, const glm::vec2 & grav_sensor_vec);

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

    void deserialize(const nlohmann::json & data, bool first_run);
    nlohmann::json serialize() const;
};

#endif //INC_2050_WORLD_HPP
