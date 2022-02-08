// Copyright 2022 Matthew Chandler

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

#ifndef INC_2050_BALL_HPP
#define INC_2050_BALL_HPP

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

class Ball
{
private:
    int size;
    float radius;
    float mass;
    glm::vec2 pos;
    glm::vec2 vel;
    glm::vec4 color;
    glm::vec4 text_color;

    void update_size();
    const std::vector<glm::vec4> & ball_colors;

public:
    Ball(float win_size, const std::vector<glm::vec4> & ball_colors, const nlohmann::json & data = {});

    int get_size() const { return size; }
    float get_radius() const { return radius; }
    glm::vec2 get_pos() const { return pos; }
    glm::vec4 get_color() const { return color; }
    glm::vec4 get_text_color() const { return text_color; }

    void grow();
    void physics_step(float dt, float win_size, const glm::vec2 & grav_vec, float wall_damp);

    void deserialize(const nlohmann::json & data);
    nlohmann::json serialize() const;

    struct Collision { bool collided = false; bool merged = false; float compression = 0.0f; };
    friend Collision collide_balls(Ball & ball, Ball & other, float e);
};

#endif //INC_2050_BALL_HPP
