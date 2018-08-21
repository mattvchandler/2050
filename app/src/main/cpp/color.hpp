#ifndef INC_2050_COLOR_HPP
#define INC_2050_COLOR_HPP

#include <vector>

#include <glm/glm.hpp>

// convert 0xAARRGGBB to {R,G,B,A}
glm::vec4 color_int_to_vec(const int color);


// convert {R, G, B, A} to 0xAARRGGBB
int color_vec_to_int(const glm::vec4 & color);

// get black or white color, depending on which has more contrast w/ passed color
glm::vec4 calc_text_color(const glm::vec4 & color);

glm::vec4 ball_color_func(int size, const std::vector<glm::vec4> ball_colors);

#endif //INC_2050_COLOR_HPP
