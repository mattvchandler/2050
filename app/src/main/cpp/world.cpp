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

World::World(AAssetManager * asset_manager)
{
    __android_log_write(ANDROID_LOG_DEBUG, "World::World", "World object created");
    for(std::size_t i = 0; i < num_starting_balls; ++i)
        balls.emplace_back(win_size);

    font_asset = AAssetManager_open(asset_manager, "DejaVuSansMono.ttf", AASSET_MODE_STREAMING);
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
     R"(attribute vec2 vert_pos;
        attribute float radius;
        attribute vec3 vert_color;

        uniform mat3 projection;
        uniform float screen_size;
        uniform float win_size;

        varying vec3 color;
        varying float border_size;
        varying float pixel_size;

        const float border_thickness = 2.0;

        void main()
        {
            color = vert_color;
            gl_Position = vec4((projection * vec3(vert_pos, 1.0)).xy, 0.0, 1.0);
            gl_PointSize = 2.0 * radius * screen_size / win_size;
            border_size = border_thickness / radius;
            pixel_size = 1.0 / gl_PointSize;
        }
    )";
    const char * ball_fragshader =
     R"(precision mediump float;

        varying vec3 color;
        varying float border_size;
        varying float pixel_size;

        void main()
        {
            vec2 coord = 2.0 * gl_PointCoord - 1.0;
            float r = dot(coord, coord);

            if(r > 1.0)
                discard;

            r = sqrt(r);
            float alpha = 1.0 - smoothstep(1.0 - 4.0 * pixel_size, 1.0, r);
            gl_FragColor = vec4(mix(color, vec3(0.0), smoothstep(1.0 - border_size - 2.0 * pixel_size, 1.0 - border_size + 2.0 * pixel_size, r)), alpha);
        }
    )";
    ball_prog = std::make_unique<Shader_prog>(std::vector<std::pair<std::string, GLenum>>{{ball_vertshader, GL_VERTEX_SHADER}, {ball_fragshader, GL_FRAGMENT_SHADER}},
                                              std::vector<std::string>{"vert_pos", "radius", "vert_color"});
    ball_vbo = std::make_unique<GL_buffer>(GL_ARRAY_BUFFER);
    ball_vbo->bind();
    glBufferData(GL_ARRAY_BUFFER, ball_vbo_alloc * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    const char * bar_vertshader =
     R"(precision mediump float;
        attribute vec2 vert_pos;
        attribute vec2 vert_tex;

        uniform mat3 projection;
        uniform vec2 bar_pos;
        uniform vec2 bar_size;

        varying vec2 tex_coords;

        void main()
        {
            tex_coords = vert_tex;
            gl_Position = vec4((projection * vec3(vert_pos * bar_size + bar_pos + bar_size / 2.0, 1.0)).xy, 0.0, 1.0);
        }
    )";
    const char * bar_fragshader =
     R"(precision mediump float;
        varying vec2 tex_coords;

        uniform vec3 color;
        uniform float screen_size;
        uniform float win_size;
        uniform vec2 bar_size;
        uniform float border_thickness;

        void main()
        {
            float border_size = border_thickness / 2.0 * screen_size / win_size;
            float xmin = border_size / bar_size.x;
            float xmax = 1.0 - border_size / bar_size.x;
            float ymin = border_size / bar_size.y;
            float ymax = 1.0 - border_size / bar_size.y;

            if(tex_coords.x < xmin || tex_coords.x > xmax ||
               tex_coords.y < ymin || tex_coords.y > ymax)
                gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            else
                gl_FragColor = vec4(color, 1.0);
        }
    )";
    bar_prog = std::make_unique<Shader_prog>(std::vector<std::pair<std::string, GLenum>>{{bar_vertshader, GL_VERTEX_SHADER}, {bar_fragshader, GL_FRAGMENT_SHADER}},
                                             std::vector<std::string>{"vert_pos", "tex_coord"});
    quad = std::make_unique<Quad>();

    // font sizes don't matter yet b/c resize should be called immediately after init
    font         = std::make_unique<textogl::Font_sys>((unsigned char *)AAsset_getBuffer(font_asset), AAsset_getLength(font_asset), 0);
    msg_font     = std::make_unique<textogl::Font_sys>((unsigned char *)AAsset_getBuffer(font_asset), AAsset_getLength(font_asset), 0);
    sub_msg_font = std::make_unique<textogl::Font_sys>((unsigned char *)AAsset_getBuffer(font_asset), AAsset_getLength(font_asset), 0);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void World::pause() { paused = true; }
bool World::is_paused() const { return paused; }

void World::destroy()
{
    __android_log_write(ANDROID_LOG_DEBUG, "World::destroy", "destroying opengl objects");
    ball_prog.reset();
    ball_vbo.reset();

    bar_prog.reset();
    quad.reset();

    ball_texts.clear();
    font.reset();

    msg_font.reset();
    sub_msg_font.reset();
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

    msg_font->resize(static_cast<int>(text_size * 72.0f / initial_text_size));
    sub_msg_font->resize(static_cast<int>(text_size * 32.0f / initial_text_size));

    ball_prog->use();
    glUniformMatrix3fv(ball_prog->get_uniform("projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform1f(ball_prog->get_uniform("screen_size"), std::min(screen_size.x, screen_size.y));
    glUniform1f(ball_prog->get_uniform("win_size"), win_size);

    bar_prog->use();
    glUniformMatrix3fv(bar_prog->get_uniform("projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform1f(bar_prog->get_uniform("screen_size"), std::min(screen_size.x, screen_size.y));
    glUniform1f(bar_prog->get_uniform("win_size"), win_size);

    GL_CHECK_ERROR("World::resize");
}

bool World::render()
{
    static std::vector<float> frame_times;
    auto start = std::chrono::high_resolution_clock::now();
    const glm::vec3 black(0.0f);
    const glm::vec3 white(1.0f);

    glClear(GL_COLOR_BUFFER_BIT);

    std::vector<float> data(std::size(balls) * 6);
    auto ball_it = std::begin(balls);
    for(std::size_t i = 0; i < std::size(data); i += 6)
    {
        const auto & pos = ball_it->get_pos();
        data[i + 0] = pos.x;
        data[i + 1] = pos.y;

        data[i + 2] = ball_it->get_radius();

        const auto & color = ball_it->get_color();
        data[i + 3] = color.r;
        data[i + 4] = color.g;
        data[i + 5] = color.b;

        ++ball_it;
    }
    ball_prog->use();

    ball_vbo->bind();
    if(std::size(data) > ball_vbo_alloc)
    {
        ball_vbo_alloc = std::size(data);
        glBufferData(GL_ARRAY_BUFFER, std::size(data) * sizeof(decltype(data)::value_type), std::data(data), GL_DYNAMIC_DRAW);
    }
    else
    {
        glBufferSubData(GL_ARRAY_BUFFER, 0, std::size(data) * sizeof(decltype(data)::value_type), std::data(data));
    }

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(decltype(data)::value_type), reinterpret_cast<GLvoid *>(0 * sizeof(decltype(data)::value_type)));
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(decltype(data)::value_type), reinterpret_cast<GLvoid *>(2 * sizeof(decltype(data)::value_type)));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(decltype(data)::value_type), reinterpret_cast<GLvoid *>(3 * sizeof(decltype(data)::value_type)));

    glDrawArrays(GL_POINTS, 0, static_cast<GLint>(std::size(balls)));

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    for(auto & ball: balls)
    {
        while(ball.get_size() >= static_cast<std::size_t>(std::size(ball_texts)))
            ball_texts.emplace_back(*font, std::to_string(1 << std::size(ball_texts)));

        glm::vec3 color = ball.get_color();
        // calculate luminance / contrast (formulae from https://www.w3.org/TR/WCAG20/)
        for(unsigned int i = 0; i < 3; ++i)
        {
            auto & c = color[i];
            c = (c <= 0.03928f) ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
        }
        auto luminance = 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
        auto text_color = (luminance > std::sqrt(0.0525f) - 0.05f) ? black : white;

        ball_texts[ball.get_size()].render_text({text_color, 1.0f}, screen_size, text_coord_transform(ball.get_pos()), textogl::ORIGIN_HORIZ_CENTER | textogl::ORIGIN_VERT_CENTER);
    }

    using namespace std::string_literals;

    // TODO: get text from strings (HOW?)

    // draw compression bar
    if(med_compression > 1.0f)
    {
        const glm::vec2 bar_pos {10.0f, 10.0f};
        const glm::vec2 bar_size {200.0f, 30.0f};
        const float border_thickness = 2.0f;

        bar_prog->use();

        // frame
        glUniform3fv(bar_prog->get_uniform("color"), 1, &white[0]);
        glUniform2fv(bar_prog->get_uniform("bar_pos"), 1, &bar_pos[0]);
        glUniform2fv(bar_prog->get_uniform("bar_size"), 1, &bar_size[0]);
        glUniform1f(bar_prog->get_uniform("border_thickness"), border_thickness);
        quad->draw();

        // color fill
        glm::vec3 fill_color {glm::clamp(0.2f * med_compression, 0.0f, 1.0f), glm::clamp(0.2f * (10.0f - med_compression), 0.0f, 1.0f), 0.0f};
        const glm::vec2 fill_size {std::min((bar_size.x  - 2.0 * border_thickness) * med_compression * 0.1f, bar_size.x - 2.0 * border_thickness), bar_size.y - 2.0 * border_thickness};
        const glm::vec2 fill_pos = bar_pos + glm::vec2(border_thickness);

        glUniform3fv(bar_prog->get_uniform("color"), 1, &fill_color[0]);
        glUniform2fv(bar_prog->get_uniform("bar_pos"), 1, &fill_pos[0]);
        glUniform2fv(bar_prog->get_uniform("bar_size"), 1, &fill_size[0]);
        glUniform1f(bar_prog->get_uniform("border_thickness"), 0.0f);
        quad->draw();

        font->render_text("Pressure", {black, 1.0f}, screen_size, text_coord_transform(bar_pos + 0.5f * bar_size), textogl::ORIGIN_HORIZ_CENTER | textogl::ORIGIN_VERT_CENTER);
    }

    font->render_text("Score: "s + std::to_string(score) + " High Score: "s + std::to_string(high_score), {black, 1.0f}, screen_size, text_coord_transform(glm::vec2(win_size - 10.0f, 10.0f)), textogl::ORIGIN_HORIZ_RIGHT | textogl::ORIGIN_VERT_TOP);

    if(paused)
    {
        msg_font->render_text("Paused", {black, 1.0f}, screen_size, text_coord_transform({win_size / 2.0f, win_size * 0.2}), textogl::ORIGIN_HORIZ_CENTER | textogl::ORIGIN_VERT_CENTER);

        sub_msg_font->render_text("Tap anywhere to continue", {black, 1.0f}, screen_size, text_coord_transform({win_size / 2.0f, win_size * 0.6}), textogl::ORIGIN_HORIZ_CENTER | textogl::ORIGIN_VERT_CENTER);
    }

    auto end = std::chrono::high_resolution_clock::now();
    frame_times.push_back(std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(end - start).count());

    static float avg_frame_time;
    if(std::size(frame_times) >= 100)
    {
        avg_frame_time = std::accumulate(std::begin(frame_times), std::end(frame_times), 0.0f) / static_cast<float>(std::size(frame_times));
        frame_times.clear();
    }
    font->render_text("avg render time: " + std::to_string(avg_frame_time) + "ms (" + std::to_string(1000.0f / avg_frame_time) + " fps)", {black, 1.0f}, screen_size, text_coord_transform({0.0f, win_size}), textogl::ORIGIN_HORIZ_LEFT | textogl::ORIGIN_VERT_BOTTOM);

    GL_CHECK_ERROR("World::render");

    return !paused;
}

void World::physics_step(float dt)
{
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
}

void World::fling(float x, float y)
{
    if(!paused)
    {
        auto fling = -glm::normalize(glm::vec2(x, y));
        grav_vec = fling * g;
        balls.emplace_back(win_size);
    }
}
void World::tap(float x, float y)
{
    if(paused)
    {
        paused = false;
    }
    if(state == State::WIN)
    {
        state = State::EXTENDED;
    }
}

void World::new_game()
{
    balls.clear();
    for(std::size_t i = 0; i < num_starting_balls; ++i)
        balls.emplace_back(win_size);

    last_compressions = std::deque<float>(100, 0.0f);
    med_compression = 0.0f;
    state = State::ONGOING;
    paused = false;
    score = 0;
    grav_vec = {0.0f, 0.0f};
}

void World::deserialize(const nlohmann::json & data)
{
    if(data.find("balls") != std::end(data))
    {
        balls.clear();
        for(auto &b: data["balls"])
            balls.emplace_back(win_size, b);
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

    if(data.find("paused") != std::end(data))
        paused = data["paused"];
    if(data.find("score") != std::end(data))
        score = data["score"];
    if(data.find("high_score") != std::end(data))
        high_score = data["high_score"];
    if(data.find("grav_vec") != std::end(data))
        grav_vec = {data["grav_vec"][0], data["grav_vec"][1]};

    if(state == State::WIN)
        game_win(score, score == high_score);
    else if(state == State::LOSE)
        new_game();
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

    data["paused"] = paused;
    data["score"] = score;
    data["high_score"] = high_score;
    data["grav_vec"] = {grav_vec.x, grav_vec.y};

    return data;
}
