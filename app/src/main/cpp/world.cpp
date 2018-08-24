// Copyright 2018 Matthew Chandler

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

#include "world.hpp"

#include <algorithm>
#include <numeric>
#include <string>

#include "color.hpp"
#include "jni.hpp"
#include "log.hpp"

glm::mat3 ortho3x3(float left, float right, float bottom, float top)
{
    return
    {
        2.0f / (right - left),            0.0f,                             0.0f,
        0.0f,                             2.0f / (top - bottom),            0.0f,
        -(right + left) / (right - left), -(top + bottom) / (top - bottom), 1.0f
    };
}

glm::vec2 World::text_coord_transform(const glm::vec2 & coord)
{
    float scale = std::min(screen_size.x, screen_size.y) / win_size;
    glm::vec2 offset
    {
        (screen_size.x > screen_size.y) ? (screen_size.x - screen_size.y) / 2.0f : 0.0f,
        (screen_size.x < screen_size.y) ? (screen_size.y - screen_size.x) / 2.0f : 0.0f
    };
    return scale * coord + offset;
}

World::World(AAssetManager * asset_manager)
{
    LOG_DEBUG_WRITE("World::World", "World object created");

    font_asset = AAssetManager_open(asset_manager, "DejaVuSansMono_ascii.ttf", AASSET_MODE_STREAMING);
    vert_shader_asset = AAssetManager_open(asset_manager, "2050.vert", AASSET_MODE_STREAMING);
    frag_shader_asset = AAssetManager_open(asset_manager, "2050.frag", AASSET_MODE_STREAMING);

    bg_color = color_int_to_vec(get_res_color("bg_color"));

    auto color_array = get_res_int_array("ball_colors");
    for(auto & color: color_array)
    {
       ball_colors.emplace_back(color_int_to_vec(color));
    }

    new_game();
}
World::~World()
{
    LOG_DEBUG_WRITE("World::~World", "World object destroyed");
    AAsset_close(font_asset);
    AAsset_close(vert_shader_asset);
    AAsset_close(frag_shader_asset);
}

void World::init()
{
    LOG_DEBUG_WRITE("World::init", "initializing opengl objects");

    std::string_view ball_vertshader{static_cast<const char *>(AAsset_getBuffer(vert_shader_asset)), static_cast<std::size_t>(AAsset_getLength(vert_shader_asset))};
    std::string_view ball_fragshader{static_cast<const char *>(AAsset_getBuffer(frag_shader_asset)), static_cast<std::size_t>(AAsset_getLength(frag_shader_asset))};

    ball_prog = std::make_unique<Shader_prog>(std::vector<std::pair<std::string_view, GLenum>>{{ball_vertshader, GL_VERTEX_SHADER}, {ball_fragshader, GL_FRAGMENT_SHADER}},
                                              std::vector<std::string>{"vert_pos", "ball_pos", "radius", "vert_color"});
    ball_vbo = std::make_unique<GL_buffer>(GL_ARRAY_BUFFER);
    ball_vbo->bind();
    glBufferData(GL_ARRAY_BUFFER, std::size(ball_data) * sizeof(decltype(ball_data)::value_type), NULL, GL_DYNAMIC_DRAW);

    // font sizes don't matter yet b/c resize should be called immediately after init
    font = std::make_unique<textogl::Font_sys>((unsigned char *)AAsset_getBuffer(font_asset), AAsset_getLength(font_asset), 0);

    glClearColor(bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void World::pause(bool show_dialog)
{
    LOG_DEBUG_WRITE("World::pause", "paused");
    if(!paused && state != State::WIN && state != State::LOSE)
    {
        paused = true;
        if(show_dialog)
            game_pause();
    }
}
bool World::is_paused() const { return paused; }
void World::unpause()
{
    LOG_DEBUG_WRITE("World::unpause", "unpaused");
    paused = false;
    if(state == State::WIN)
    {
        state = State::EXTENDED;
    }
}

void World::destroy()
{
    LOG_DEBUG_WRITE("World::destroy", "destroying opengl objects");
    ball_prog.reset();
    ball_vbo.reset();

    ball_texts.clear();
    font.reset();
}

void World::resize(GLsizei width, GLsizei height)
{
    LOG_DEBUG_PRINT("World::resize", "resize with %d x %d", width, height);

    glViewport(0, 0, width, height);
    screen_size = {width, height};

    auto aspect = screen_size.x / screen_size.y;
    float left = 0.0f, right = win_size, bottom = win_size, top = 0.0f;
    if(screen_size.x > screen_size.y)
    {
        left = -win_size * (aspect - 1.0f) / 2.0f;
        right = win_size - left;
    }
    else if(screen_size.x < screen_size.y)
    {
        top = -win_size * (1.0f - aspect) / (2.0f * aspect);
        bottom = win_size - top;
    }

    projection = ortho3x3(left, right, bottom, top);

    auto scale_factor = std::min(screen_size.x, screen_size.y) / win_size;
    auto new_text_size = static_cast<int>(scale_factor * initial_text_size);
    LOG_DEBUG_PRINT("World::resize", "font resized from %d to %d", text_size, new_text_size);
    text_size = new_text_size;
    font->resize(text_size);
    for(auto & t: ball_texts)
        t.set_font_sys(*font);

    auto inv_projection = glm::inverse(projection);

    ball_prog->use();
    glUniformMatrix3fv(ball_prog->get_uniform("projection"), 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix3fv(ball_prog->get_uniform("inv_projection"), 1, GL_FALSE, &inv_projection[0][0]);
    glUniform2fv(ball_prog->get_uniform("screen_size"), 1, &screen_size[0]);
    glUniform1f(ball_prog->get_uniform("win_size"), win_size);

    GL_CHECK_ERROR("World::resize");
}

void World::render_balls()
{
    const std::vector<glm::vec2> verts =
    {
        {-0.5f * std::sqrt(3.0f), -0.5f},
        { 0.0f,                    1.0f},
        { 0.5f * std::sqrt(3.0f), -0.5f}
    };

    // load up a buffer with vertex data. unfortunately GL ES 2.0 is pretty limited, so lots of duplication here
    // if we had instanced rendering or geometry shaders, this would be much easier
    auto data_size = std::size(balls) * std::size(verts) * num_ball_attrs;
    auto old_ball_data_size = std::size(ball_data);

    while(std::size(ball_data) < data_size)
        ball_data.resize(2 * std::size(ball_data));

    auto ball = std::begin(balls);
    for(std::size_t ball_i = 0; ball_i < std::size(balls); ++ball_i)
    {
        const auto & pos = ball->get_pos();
        const auto & color = ball->get_color();

        for(std::size_t vert_i = 0; vert_i < std::size(verts); ++vert_i)
        {
            std::size_t data_i = (ball_i * std::size(verts) + vert_i) * num_ball_attrs;
            ball_data[data_i + 0] = verts[vert_i].x;
            ball_data[data_i + 1] = verts[vert_i].y;
            ball_data[data_i + 2] = pos.x;
            ball_data[data_i + 3] = pos.y;
            ball_data[data_i + 4] = ball->get_radius();
            ball_data[data_i + 5] = color.r;
            ball_data[data_i + 6] = color.g;
            ball_data[data_i + 7] = color.b;
        }

        ++ball;
    }

    ball_prog->use();
    ball_vbo->bind();

    if(std::size(ball_data) > old_ball_data_size)
    {
        LOG_DEBUG_PRINT("World::render_balls", "ball_data buffer resized from %d to %d", static_cast<int>(old_ball_data_size), static_cast<int>(std::size(ball_data)));
        glBufferData(GL_ARRAY_BUFFER, std::size(ball_data) * sizeof(decltype(ball_data)::value_type), std::data(ball_data), GL_DYNAMIC_DRAW);
    }
    else
    {
        glBufferSubData(GL_ARRAY_BUFFER, 0, data_size * sizeof(decltype(ball_data)::value_type), std::data(ball_data));
    }

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, num_ball_attrs * sizeof(decltype(ball_data)::value_type), reinterpret_cast<GLvoid *>(0 * sizeof(decltype(ball_data)::value_type)));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, num_ball_attrs * sizeof(decltype(ball_data)::value_type), reinterpret_cast<GLvoid *>(2 * sizeof(decltype(ball_data)::value_type)));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, num_ball_attrs * sizeof(decltype(ball_data)::value_type), reinterpret_cast<GLvoid *>(4 * sizeof(decltype(ball_data)::value_type)));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, num_ball_attrs * sizeof(decltype(ball_data)::value_type), reinterpret_cast<GLvoid *>(5 * sizeof(decltype(ball_data)::value_type)));

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLint>(std::size(balls) * std::size(verts)));

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
}

bool World::render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    render_balls();

    for(auto & ball: balls)
    {
        while(ball.get_size() >= static_cast<std::size_t>(std::size(ball_texts)))
            ball_texts.emplace_back(*font, std::to_string(1 << std::size(ball_texts)));

        ball_texts[ball.get_size()].render_text(ball.get_text_color(), screen_size, text_coord_transform(ball.get_pos()), textogl::ORIGIN_HORIZ_CENTER | textogl::ORIGIN_VERT_CENTER);
    }

    GL_CHECK_ERROR("World::render");

    return !paused;
}

void World::physics_step(float dt, bool gravity_mode, const glm::vec3 & grav_sensor_vec)
{
    if(paused || state == State::LOSE || state == State::WIN)
        return;

    if(gravity_mode)
    {
        auto grav_vec_3 = -g * glm::normalize(grav_sensor_vec);
        grav_vec = {grav_vec_3.y, grav_vec_3.x};

        float grav_angle = std::atan2(grav_vec.x, -grav_vec.y);
        auto diff = std::abs(grav_angle - grav_ref_angle);
        if(diff > M_PI)
            diff = 2.0f * static_cast<float>(M_PI) - diff;

        if(diff > M_PI / 4.0f)
        {
            grav_ref_angle = grav_angle;
            balls.emplace_back(win_size, ball_colors);
        }
    }

    float compression = 0.0f;
    for(auto ball = std::begin(balls); ball != std::end(balls); ++ball)
    {
        if(state != State::EXTENDED && ball->get_size() >= 11) // 2^11 = 2048
        {
            state = State::WIN;
            game_win(score, score == high_score);
        }

        ball->physics_step(dt, win_size, grav_vec, wall_damp);

        // check for collision
        for(auto other = std::next(ball); other != std::end(balls); ++other)
        {
            auto collision = collide_balls(*ball, *other, e);

            if(collision.collided)
            {
                if(collision.merged)
                {
                    other = std::prev(balls.erase(other));
                    score += 1 << (ball->get_size());
                    high_score = std::max(high_score, score);
                    if(ball->get_size() >= next_achievement_size)
                    {
                        achievement(next_achievement_size);
                        ++next_achievement_size;
                    }
                } else
                {
                    compression += collision.compression;
                }
            }
        }
    }

    last_compressions.push_back(compression / std::size(balls));
    last_compressions.pop_front();

    std::vector<float> sorted_compressions(std::begin(last_compressions), std::end(last_compressions));
    std::sort(std::begin(sorted_compressions), std::end(sorted_compressions));
    med_compression = sorted_compressions[std::size(sorted_compressions) / 2];

    if(med_compression > 10.0f)
    {
        state = State::LOSE;

        game_over(score, score == high_score);
    }
}

void World::fling(float x, float y)
{
    if(!paused)
    {
        auto fling = -glm::normalize(glm::vec2(x, y));
        grav_vec = fling * g;
        balls.emplace_back(win_size, ball_colors);
    }
}

void World::new_game()
{
    balls.clear();
    for(std::size_t i = 0; i < num_starting_balls; ++i)
        balls.emplace_back(win_size, ball_colors);

    last_compressions = std::deque<float>(100, 0.0f);
    med_compression = 0.0f;
    state = State::ONGOING;
    paused = false;
    score = 0;
    grav_vec = {0.0f, 0.0f};
}

World::UI_data World::get_ui_data()
{
    return {score, high_score, std::atan2(grav_vec.x, -grav_vec.y), static_cast<int>(std::lround(med_compression * 10.0f))};
}

void World::deserialize(const nlohmann::json & data, bool first_run)
{
    if(data.find("balls") != std::end(data))
    {
        balls.clear();
        for(auto &b: data["balls"])
            balls.emplace_back(win_size, ball_colors, b);
    }

    if(data.find("last_compressions") != std::end(data))
        last_compressions = std::deque<float>(std::begin(data["last_compressions"]), std::end(data["last_compressions"]));
    if(data.find("med_compression") != std::end(data))
        med_compression = data["med_compression"];

    if(data.find("state") != std::end(data))
    {
        auto state_str = data["state"];
        if(state_str == "ONGOING")
            state = State::ONGOING;
        else if(state_str == "WIN")
            state = State::WIN;
        else if(state_str == "LOSE")
            state = State::LOSE;
        else if(state_str == "EXTENDED")
            state = State::EXTENDED;
    }

    if(data.find("score") != std::end(data))
        score = data["score"];
    if(data.find("high_score") != std::end(data))
        high_score = data["high_score"];
    if(data.find("next_achievement_size") != std::end(data))
        next_achievement_size = data["next_achievement_size"];
    if(data.find("grav_vec") != std::end(data))
        grav_vec = {data["grav_vec"][0], data["grav_vec"][1]};

    if(state == State::WIN)
    {
        game_win(score, score == high_score);
    }
    else if(state == State::LOSE)
    {
        if(first_run)
            new_game();
        else
            game_over(score, score == high_score);
    }
}

nlohmann::json World::serialize() const
{
    using json = nlohmann::json;
    json data;

    data["balls"] = json::array();
    for(auto &b: balls)
        data["balls"].push_back(b.serialize());

    data["last_compressions"] = last_compressions;
    data["med_compression"] = med_compression;

    switch(state)
    {
    case State::ONGOING:
        data["state"] = "ONGOING";
        break;
    case State::WIN:
        data["state"] = "WIN";
        break;
    case State::LOSE:
        data["state"] = "LOSE";
        break;
    case State::EXTENDED:
        data["state"] = "EXTENDED";
        break;
    }

    data["score"] = score;
    data["high_score"] = high_score;
    data["next_achievement_size"] = next_achievement_size;
    data["grav_vec"] = {grav_vec.x, grav_vec.y};

    return data;
}
