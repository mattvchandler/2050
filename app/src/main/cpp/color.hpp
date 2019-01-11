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
