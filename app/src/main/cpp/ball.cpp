#include "ball.hpp"

#include <vector>
#include <random>

#include "color.hpp"

thread_local std::default_random_engine prng(std::random_device{}());

const auto pi = std::acos(-1.0f);
glm::vec2 rand_circle(float radius)
{
    std::uniform_real_distribution<float> rnd(0.0f, 1.0f);
    auto t = 2.0f * pi * rnd(prng);
    auto u = rnd(prng) + rnd(prng);
    auto r = (u > 1.0f) ? 2.0f - u : u;
    return radius * glm::vec2{r * std::cos(t), r * std::sin(t)};
}

void Ball::update_size()
{
    radius = size * 10.0f;
    mass = 4.0f / 3.0f * pi * std::pow(radius, 3.0f);
    color = ball_colors[ball_color_index(size, std::size(ball_colors))];

    text_color = calc_text_color(color);
}

Ball::Ball(float win_size, const std::vector<glm::vec4> & ball_colors, const nlohmann::json & data): ball_colors(ball_colors)
{
    if(data.empty())
    {
        size = std::uniform_int_distribution(1, 2)(prng);
        pos = {std::uniform_real_distribution<float>(0.0f, win_size)(prng),
               std::uniform_real_distribution<float>(0.0f, win_size)(prng)};
        vel = rand_circle(10.0f);
    }
    else
    {
        deserialize(data);
    }
    update_size();
}

void Ball::grow()
{
    ++size;
    update_size();
}

void Ball::physics_step(float dt, float win_size, const glm::vec2 & grav_vec, float wall_damp)
{
    pos += dt * vel;
    vel += dt * grav_vec;

    // check for collision with walls. Bounce if hit
    if(pos.x - radius < 0.0f) // left wall
    {
        vel.x = std::abs(vel.x) * wall_damp;
        // clamp position so that ball stays on screen even if the window is resized
        pos = {std::max(radius, pos.x - radius), pos.y};
    }

    if(pos.x + radius > win_size) // right wall
    {
        vel.x = -std::abs(vel.x) * wall_damp;
        pos = {std::min(pos.x + radius, win_size - radius), pos.y};
    }

    if(pos.y - radius < 0.0f) // top wall
    {
        vel.y = std::abs(vel.y) * wall_damp;
        pos = {pos.x, std::max(radius, pos.y - radius)};
    }

    if(pos.y + radius > win_size) // bottom wall
    {
        vel.y = -std::abs(vel.y) * wall_damp;
        pos = {pos.x, std::min(pos.y + radius, win_size - radius)};
    }
}

Ball::Collision collide_balls(Ball & ball, Ball & other, float e)
{
    glm::vec2 pos_diff = ball.pos - other.pos;

    float dist = glm::length(pos_diff);

    if(dist < 1.0e-4) // prevent div by 0
        return {};

    glm::vec2 n = pos_diff / dist; // normalized direction vector

    if(dist <= ball.radius + other.radius) // collided
    {
        Ball::Collision collision;
        collision.collided = true;

        // calc compression
        collision.compression += ball.radius + other.radius - dist;

        float c = glm::dot(n, ball.vel - other.vel);
        // merge
        // TODO: merge aninmation
        if(ball.size == other.size)
        {
            collision.merged = true;

            float ball_mag = ((other.mass * c) / (other.mass + ball.mass));
            ball.pos = (ball.pos + other.pos) / 2.0f;
            ball.vel -= glm::vec2{ball_mag, ball_mag} * n;
            ++ball.size;
            ball.update_size();
        }
        else
        {
            // elastic collision
            float ball_mag = ((other.mass * c) / (other.mass + ball.mass)) * (1.0f + e);
            float other_mag = ((ball.mass * c) / (other.mass + ball.mass)) * (1.0f + e);

            ball.vel -= ball_mag * n;
            other.vel += other_mag * n;

            // force to not intersect
            ball.pos += n * glm::vec2{(ball.radius + other.radius - dist) * 0.5f,
                    (ball.radius + other.radius - dist) * 0.5f};
            other.pos -= n * glm::vec2{(ball.radius + other.radius - dist) * 0.5f,
                    (ball.radius + other.radius - dist) * 0.5f};
        }

        return collision;
    }

    return {};
}

void Ball::deserialize(const nlohmann::json & data)
{
    if(data.find("size") != std::end(data))
        size = data["size"];
    if(data.find("pos") != std::end(data))
        pos = {data["pos"][0], data["pos"][1]};
    if(data.find("vel") != std::end(data))
        vel = {data["vel"][0], data["vel"][1]};
}
nlohmann::json Ball::serialize() const
{
    using json = nlohmann::json;
    json data;

    data["size"] = size;
    data["pos"] = {pos.x, pos.y};
    data["vel"] = {vel.x, vel.y};

    return data;
}
