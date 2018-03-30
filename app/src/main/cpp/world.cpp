#include "world.hpp"

#include <string>

#include <android/log.h>
glm::mat3 ortho3x3(float left, float right, float bottom, float top)
{
    return
    {
        2.0f / (right - left),            0.0f,                             0.0f,
        0.0f,                             2.0f / (top - bottom),            0.0f,
        -(right + left) / (right - left), -(top + bottom) / (top - bottom), 1.0f
    };
}

glm::mat3 scale(const glm::mat3 & m, const glm::vec2 v)
{
    return m * glm::mat3
    {
        v.x,  0.0f, 0.0f,
        0.0f, v.y,  0.0f,
        0.0f, 0.0f, 1.0f
    };
}

glm::mat3 translate(const glm::mat3 & m, const glm::vec2 v)
{
    return m * glm::mat3
    {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        v.x,  v.y,  1.0f
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
    const char * vertshader =
     R"(attribute vec2 vert_pos;
        attribute vec2 vert_tex;

        uniform mat3 modelview_projection;

        varying vec2 tex_coords;

        void main()
        {
            tex_coords = vert_tex;
            gl_Position = vec4(modelview_projection * vec3(vert_pos, 1.0), 1.0);
        }
    )";
    const char * fragshader =
     R"(precision mediump float;
        varying vec2 tex_coords;
        uniform sampler2D tex;

        uniform vec3 color;

        void main()
        {
            gl_FragColor = vec4(color, texture2D(tex, tex_coords).a);
        }
    )";
    prog = std::make_unique<Shader_prog>(std::vector<std::pair<std::string, GLenum>>{{vertshader, GL_VERTEX_SHADER}, {fragshader, GL_FRAGMENT_SHADER}},
                                         std::vector<std::pair<std::string, GLuint>>{{"vert_pos", 0}});

    quad = std::make_unique<Quad>();
    circle_tex = std::make_unique<Texture2D>(Texture2D::gen_circle_tex(512));
    rect_tex = std::make_unique<Texture2D>(Texture2D::gen_1pix_tex());

    // font sizes don't matter yet b/c resize should be called immediately after init
    font         = std::make_unique<textogl::Font_sys>((unsigned char *)AAsset_getBuffer(font_asset), AAsset_getLength(font_asset), 0);
    msg_font     = std::make_unique<textogl::Font_sys>((unsigned char *)AAsset_getBuffer(font_asset), AAsset_getLength(font_asset), 0);
    sub_msg_font = std::make_unique<textogl::Font_sys>((unsigned char *)AAsset_getBuffer(font_asset), AAsset_getLength(font_asset), 0);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void World::destroy()
{
    __android_log_write(ANDROID_LOG_DEBUG, "World::destroy", "destroying opengl objects");
    prog.reset();
    quad.reset();
    circle_tex.reset();
    rect_tex.reset();

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

    screen_projection = ortho3x3(0.0f, static_cast<float>(screen_size.x), static_cast<float>(screen_size.y), 0.0f);

    auto aspect = static_cast<float>(screen_size.x) / static_cast<float>(screen_size.y);
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

    auto scale_factor = static_cast<float>(std::min(screen_size.x, screen_size.y)) / static_cast<float>(win_size);
    auto new_text_size = static_cast<int>(scale_factor * initial_text_size);
    __android_log_print(ANDROID_LOG_DEBUG, "World::resize", "font resized from %d to %d", text_size, new_text_size);
    text_size = new_text_size;
    font->resize(text_size);
    for(auto & t: ball_texts)
        t.set_font_sys(*font);

    msg_font->resize(static_cast<int>(text_size * 72.0f / initial_text_size));
    sub_msg_font->resize(static_cast<int>(text_size * 32.0f / initial_text_size));
}

void World::render()
{
    const glm::vec3 black(0.0f);
    const glm::vec3 white(1.0f);

    glClear(GL_COLOR_BUFFER_BIT);

    for(auto & ball: balls)
    {
        prog->use();
        circle_tex->bind();

        // draw border
        auto modelview_projection = scale(translate(projection, ball.get_pos()), glm::vec2(2.0f * ball.get_radius(), 2.0f * ball.get_radius()));
        glUniformMatrix3fv(prog->get_uniform("modelview_projection"), 1, GL_FALSE, &modelview_projection[0][0]);
        glUniform3fv(prog->get_uniform("color"), 1, &black[0]);
        quad->draw();

        // fill center
        modelview_projection = scale(translate(projection, ball.get_pos()), glm::vec2(2.0f * ball.get_radius() - 4.0f, 2.0f * ball.get_radius() - 4.0f));
        glUniformMatrix3fv(prog->get_uniform("modelview_projection"), 1, GL_FALSE, &modelview_projection[0][0]);
        glUniform3fv(prog->get_uniform("color"), 1, &ball.get_color()[0]);
        quad->draw();

        while(ball.get_size() >= static_cast<std::size_t>(std::size(ball_texts)))
            ball_texts.emplace_back(*font, std::to_string(1 << std::size(ball_texts)));

        ball_texts[ball.get_size()].render_text({black, 1.0f}, screen_size, text_coord_transform(ball.get_pos()), textogl::ORIGIN_HORIZ_CENTER | textogl::ORIGIN_VERT_CENTER);
    }

    using namespace std::string_literals;

    // TODO: get text from strings (HOW?)
    font->render_text("Score: "s + std::to_string(score), {black, 1.0f}, screen_size, text_coord_transform(glm::vec2(win_size - 10.0f, 10.0f)), textogl::ORIGIN_HORIZ_RIGHT | textogl::ORIGIN_VERT_TOP);

    if(state == State::WIN)
    {
        msg_font->render_text("You Win!", {black, 1.0f}, screen_size,
                             text_coord_transform({win_size / 2.0f, win_size * 0.2}), textogl::ORIGIN_HORIZ_CENTER | textogl::ORIGIN_VERT_CENTER);

        sub_msg_font->render_text("Press some nonexistent button to restart,\nor tap to continue playing", {black, 1.0f}, screen_size,
                                 text_coord_transform({win_size / 2.0f, win_size * 0.8}), textogl::ORIGIN_HORIZ_CENTER | textogl::ORIGIN_VERT_CENTER);
    }
    else if(state == State::LOSE)
    {
        msg_font->render_text("You Lose!", {black, 1.0f}, screen_size,
                             text_coord_transform({win_size / 2.0f, win_size * 0.2}), textogl::ORIGIN_HORIZ_CENTER | textogl::ORIGIN_VERT_CENTER);

        sub_msg_font->render_text("Press something to restart", {black, 1.0f}, screen_size,
                                 text_coord_transform({win_size / 2.0f, win_size * 0.6}), textogl::ORIGIN_HORIZ_CENTER | textogl::ORIGIN_VERT_CENTER);
    }
    else if(paused)
    {
        msg_font->render_text("Paused", {black, 1.0f}, screen_size,
                             text_coord_transform({win_size / 2.0f, win_size * 0.2}), textogl::ORIGIN_HORIZ_CENTER | textogl::ORIGIN_VERT_CENTER);

        sub_msg_font->render_text("Tap anywhere to continue", {black, 1.0f}, screen_size,
                                 text_coord_transform({win_size / 2.0f, win_size * 0.6}), textogl::ORIGIN_HORIZ_CENTER | textogl::ORIGIN_VERT_CENTER);
    }

    // draw screen borders
    if(screen_size.x != screen_size.y)
    {
        glm::vec2 border_size = {screen_size.x - screen_size.y, screen_size.y};
        prog->use();
        rect_tex->bind();
        glUniform3fv(prog->get_uniform("color"), 1, &black[0]);
        if(screen_size.x > screen_size.y)
        {
            auto border = scale(translate(screen_projection, glm::vec2{0.0f, border_size.y / 2.0f}), border_size);
            glUniformMatrix3fv(prog->get_uniform("modelview_projection"), 1, GL_FALSE, &border[0][0]);
            quad->draw();

            border = scale(translate(screen_projection, glm::vec2{screen_size.x, border_size.y / 2.0f}), border_size);
            glUniformMatrix3fv(prog->get_uniform("modelview_projection"), 1, GL_FALSE, &border[0][0]);
            quad->draw();
        }
        else if(screen_size.x < screen_size.y)
        {
            border_size = {screen_size.x, screen_size.y - screen_size.x};

            auto border = scale(translate(screen_projection, glm::vec2{border_size.x / 2.0f, 0.0f}), border_size);
            glUniformMatrix3fv(prog->get_uniform("modelview_projection"), 1, GL_FALSE, &border[0][0]);
            quad->draw();

            border = scale(translate(screen_projection, glm::vec2{border_size.x / 2.0f, screen_size.y}), border_size);
            glUniformMatrix3fv(prog->get_uniform("modelview_projection"), 1, GL_FALSE, &border[0][0]);
            quad->draw();
        }
    }
    GL_CHECK_ERROR("draw");
}

void World::pause()
{
    paused = true;
}

void World::physics_step(float dt)
{
    float compression = 0.0f;
    for(auto ball = std::begin(balls); ball != std::end(balls); ++ball)
    {
        if(!paused && (state == State::ONGOING || state == State::EXTENDED))
        {
            if(state != State::EXTENDED && ball->get_size() >= 11) // 2048
                state = State::WIN;

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
                    } else
                    {
                        compression += collision.compression;
                    }
                }
            }
        }
    }
}

void World::fling(float x, float y)
{
    auto fling = -glm::normalize(glm::vec2(x, y));
    grav_vec = fling * g;
    balls.emplace_back(win_size);
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

void World::deserialize(const nlohmann::json & data)
{
    balls.clear();
    for(auto &b: data["balls"])
        balls.emplace_back(win_size, b);

    auto state_str = data["state"];
    if(state_str == "ONGOING")
        state = State::ONGOING;
    else if(state_str == "WIN")
        state = State::WIN;
    else if(state_str == "LOSE")
        state = State::LOSE;
    else if(state_str == "EXTENDED")
        state = State::EXTENDED;

    paused = data["paused"];
    score = data["score"];
    grav_vec = {data["grav_vec"][0], data["grav_vec"][1]};
}

nlohmann::json World::serialize() const
{
    using json = nlohmann::json;
    json data;

    data["balls"] = json::array();
    for(auto &b: balls)
        data["balls"].push_back(b.serialize());

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
    data["grav_vec"] = {grav_vec.x, grav_vec.y};

    return data;
}
