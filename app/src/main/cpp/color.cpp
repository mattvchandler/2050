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

#include "color.hpp"

// convert 0xAARRGGBB to {R,G,B,A}
glm::vec4 color_int_to_vec(const int color) noexcept
{
    return
            {
                static_cast<float>((static_cast<unsigned int>(color) >> 16u) & 0xFFu) / 255.0f, // R
                static_cast<float>((static_cast<unsigned int>(color) >>  8u) & 0xFFu) / 255.0f, // G
                static_cast<float>((static_cast<unsigned int>(color))        & 0xFFu) / 255.0f, // B
                static_cast<float>((static_cast<unsigned int>(color) >> 24u) & 0xFFu) / 255.0f  // A
            };
}

// convert {R, G, B, A} to 0xAARRGGBB
int color_vec_to_int(const glm::vec4 & color) noexcept
{
    return
            (static_cast<unsigned int>(color.a * 255.0f) & 0xFFu) << 24u | // A
            (static_cast<unsigned int>(color.r * 255.0f) & 0xFFu) << 16u | // R
            (static_cast<unsigned int>(color.g * 255.0f) & 0xFFu) <<  8u | // G
            (static_cast<unsigned int>(color.b * 255.0f) & 0xFFu);         // B
}

// get black or white color, depending on which has more contrast w/ passed color
glm::vec4 calc_text_color(const glm::vec4 & color) noexcept
{
    // calculate text color (luminance / contrast  formulas from https://www.w3.org/TR/WCAG20/)
    auto luminance_color = color;

    for(unsigned int i = 0; i < 3; ++i)
    {
        auto & c = luminance_color[i];
        c = (c <= 0.03928f) ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
    }

    auto luminance = 0.2126f * luminance_color.r + 0.7152f * luminance_color.g + 0.0722f * luminance_color.b;

    return (luminance > std::sqrt(0.0525f) - 0.05f) ? glm::vec4{0.0f, 0.0f, 0.0f, color.a} : glm::vec4{1.0f, 1.0f, 1.0f, color.a};
}

int ball_color_index(int size, int num_colors) noexcept
{
    auto index = (size - 1) % (2 * num_colors - 2);
    if(index >= num_colors)
        index = 2 * num_colors - index - 2;

    return index;
}
