#include "world.hpp"

#include <algorithm>
#include <numeric>
#include <string>

#include <android/log.h>

#include "jni.hpp"

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

std::string World::get_str(const std::string & id)
{
    auto str = resource_strings.find(id);
    if(str == std::end(resource_strings))
    {
        str = resource_strings.insert({id, get_res_string(id)}).first;
    }

    return str->second;
}

World::World(AAssetManager * asset_manager)
{
    __android_log_write(ANDROID_LOG_DEBUG, "World::World", "World object created");

    font_asset = AAssetManager_open(asset_manager, "DejaVuSansMono.ttf", AASSET_MODE_STREAMING);

    for(auto & i: {"pressure"})
    {
        resource_strings[i] = get_res_string(i);
    }

    auto color_array = get_res_int_array("ball_colors");
    for(auto & color: color_array)
    {
       ball_colors.emplace_back(
           static_cast<float>((color >> 16) & 0xFF) / 256.0f,
           static_cast<float>((color >>  8) & 0xFF) / 256.0f,
           static_cast<float>((color)       & 0xFF) / 256.0f
       );
    }

    new_game();
}
World::~World()
{
    __android_log_write(ANDROID_LOG_DEBUG, "World::~World", "World object destroyed");
    AAsset_close(font_asset);
}

void World::init()
{
    __android_log_write(ANDROID_LOG_DEBUG, "World::init", "initializing opengl objects");
    const char * ball_vertshader =
     R"(precision mediump float;

        attribute vec2 vert_pos;
        attribute vec2 ball_pos;
        attribute float radius;
        attribute vec3 vert_color;

        uniform mat3 projection;
        uniform float win_size;
        varying float border_size;
        varying float pixel_size;

        varying vec3 color;
        varying vec2 center;
        varying float frag_radius;

        const float border_thickness = 2.0;

        void main()
        {
            color = vert_color;
            gl_Position = vec4((projection * vec3(vert_pos * 2.0 * radius + ball_pos, 1.0)).xy, 0.0, 1.0);
            center = ball_pos;
            frag_radius = radius;
            border_size = border_thickness / frag_radius;
            pixel_size = 1.0 / win_size;
        }
    )";
    const char * ball_fragshader =
     R"(precision mediump float;

        varying vec3 color;
        varying vec2 center;
        varying float frag_radius;
        varying float border_size;
        varying float pixel_size;

        uniform mat3 inv_projection;
        uniform vec2 screen_size;
        uniform float win_size;

        void main()
        {
            vec2 coord = (inv_projection * vec3(2.0 * gl_FragCoord.xy / screen_size - vec2(1.0), 1.0)).xy;
            float r = distance(coord, center);

            if(r >= frag_radius)
                discard;

            r /= frag_radius;

            float alpha = 1.0 - smoothstep(1.0 - 4.0 * pixel_size, 1.0, r);
            gl_FragColor = vec4(mix(color, vec3(0.0), smoothstep(1.0 - border_size - 2.0 * pixel_size, 1.0 - border_size + 2.0 * pixel_size, r)), alpha);
        }
    )";
    ball_prog = std::make_unique<Shader_prog>(std::vector<std::pair<std::string, GLenum>>{{ball_vertshader, GL_VERTEX_SHADER}, {ball_fragshader, GL_FRAGMENT_SHADER}},
                                              std::vector<std::string>{"vert_pos", "ball_pos", "radius", "vert_color"});
    ball_vbo = std::make_unique<GL_buffer>(GL_ARRAY_BUFFER);
    ball_vbo->bind();
    glBufferData(GL_ARRAY_BUFFER, std::size(ball_data) * sizeof(decltype(ball_data)::value_type), NULL, GL_DYNAMIC_DRAW);

    // font sizes don't matter yet b/c resize should be called immediately after init
    font         = std::make_unique<textogl::Font_sys>((unsigned char *)AAsset_getBuffer(font_asset), AAsset_getLength(font_asset), 0);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void World::pause()
{
    __android_log_print(ANDROID_LOG_DEBUG, "World::pause", paused ? "paused" : "unpaused");
    if(!paused && state != State::WIN && state != State::LOSE)
    {
        paused = true;
        game_pause();
    }
}
bool World::is_paused() const { return paused; }
void World::unpause()
{
    __android_log_print(ANDROID_LOG_DEBUG, "World::unpause", paused ? "paused" : "unpaused");
    paused = false;
    if(state == State::WIN)
    {
        state = State::EXTENDED;
    }
}

void World::destroy()
{
    __android_log_write(ANDROID_LOG_DEBUG, "World::destroy", "destroying opengl objects");
    ball_prog.reset();
    ball_vbo.reset();

    ball_texts.clear();
    font.reset();
}

void World::resize(GLsizei width, GLsizei height)
{
    __android_log_print(ANDROID_LOG_DEBUG, "World::resize", "resize with %d x %d", width, height);

    glViewport(0, 0, width, height);
    screen_size = {width, height};

    screen_projection = ortho3x3(0.0f, screen_size.x, screen_size.y, 0.0f);

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
    __android_log_print(ANDROID_LOG_DEBUG, "World::resize", "font resized from %d to %d", text_size, new_text_size);
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
    if(std::size(ball_data) < data_size)
        ball_data.resize(data_size);

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

    if(data_size > std::size(ball_data))
    {
        glBufferData(GL_ARRAY_BUFFER, data_size * sizeof(decltype(ball_data)::value_type), std::data(ball_data), GL_DYNAMIC_DRAW);
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
    static std::vector<float> frame_times;
    auto start = std::chrono::high_resolution_clock::now();
    const glm::vec3 black(0.0f);
    const glm::vec3 white(1.0f); // TODO: unused

    glClear(GL_COLOR_BUFFER_BIT);

    render_balls();

    for(auto & ball: balls)
    {
        while(ball.get_size() >= static_cast<std::size_t>(std::size(ball_texts)))
            ball_texts.emplace_back(*font, std::to_string(1 << std::size(ball_texts)));

        ball_texts[ball.get_size()].render_text({ball.get_text_color(), 1.0f}, screen_size, text_coord_transform(ball.get_pos()), textogl::ORIGIN_HORIZ_CENTER | textogl::ORIGIN_VERT_CENTER);
    }

    auto end = std::chrono::high_resolution_clock::now();
    frame_times.push_back(std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(end - start).count());

    static float avg_frame_time;
    if(std::size(frame_times) >= 100)
    {
        avg_frame_time = std::accumulate(std::begin(frame_times), std::end(frame_times), 0.0f) / static_cast<float>(std::size(frame_times));
        frame_times.clear();
    }

    static float avg_physx_time;
    if(std::size(physx_times) >= 100)
    {
        avg_physx_time = std::accumulate(std::begin(physx_times), std::end(physx_times), 0.0f) / static_cast<float>(std::size(physx_times));
        physx_times.clear();
    }

    font->render_text(paused ? "paused" : "unpaused", {black, 1.0}, screen_size, text_coord_transform({win_size, win_size}),
        textogl::ORIGIN_HORIZ_RIGHT | textogl::ORIGIN_VERT_BOTTOM);

    font->render_text("avg render time: " + std::to_string(avg_frame_time) + "ms (" + std::to_string(1000.0f / avg_frame_time) + " fps)" +
                    "\navg physic time: " + std::to_string(avg_physx_time) + "ms (" + std::to_string(1000.0f / avg_physx_time) + " fps)",
        {black, 1.0f}, screen_size, text_coord_transform({0.0f, win_size}), textogl::ORIGIN_HORIZ_LEFT | textogl::ORIGIN_VERT_BOTTOM);

    GL_CHECK_ERROR("World::render");

    return !paused;
}

void World::physics_step(float dt)
{
    auto start = std::chrono::high_resolution_clock::now();
    if(paused || state == State::LOSE || state == State::WIN)
        return;

    float compression = 0.0f;
    for(auto ball = std::begin(balls); ball != std::end(balls); ++ball)
    {
        if(state != State::EXTENDED && ball->get_size() >= 11) // 2048
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

    auto end = std::chrono::high_resolution_clock::now();
    physx_times.push_back(std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(end - start).count());
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
    data["grav_vec"] = {grav_vec.x, grav_vec.y};

    return data;
}
