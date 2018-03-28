#ifndef INC_2050_BALL_HPP
#define INC_2050_BALL_HPP

#include <glm/glm.hpp>

class Ball
{
private:
    int size;
    float radius;
    float mass;
    glm::vec2 pos;
    glm::vec2 vel;
    glm::vec3 color;

    void update_size();
    glm::vec3 color_func();

public:
    Ball(float win_size);

    int get_size() const { return size; }
    float get_radius() const { return radius; }
    glm::vec2 get_pos() const { return pos; }
    glm::vec3 get_color() const { return color; }

    void grow();
    void physics_step(float dt, float win_size, const glm::vec2 & grav_vec, float wall_damp);

    struct Collision { bool collided = false; bool merged = false; float compression = 0.0f; };
    friend Collision collide_balls(Ball & ball, Ball & other, float e);
};


#endif //INC_2050_BALL_HPP
