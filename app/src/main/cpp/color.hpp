#ifndef INC_2050_COLOR_HPP
#define INC_2050_COLOR_HPP

#include <vector>

#include <glm/glm.hpp>

// convert 0xAARRGGBB to {R,G,B,A}
glm::vec4 color_int_to_vec(const int color) noexcept;

// convert {R, G, B, A} to 0xAARRGGBB
int color_vec_to_int(const glm::vec4 & color) noexcept;

// get black or white color, depending on which has more contrast w/ passed color
glm::vec4 calc_text_color(const glm::vec4 & color) noexcept;

int ball_color_index(int size, int num_colors) noexcept;

#endif //INC_2050_COLOR_HPP
