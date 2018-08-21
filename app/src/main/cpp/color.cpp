#include "color.hpp"

// convert 0xAARRGGBB to {R,G,B,A}
glm::vec4 color_int_to_vec(const int color) noexcept
{
    return
            {
                    static_cast<float>((color >> 16) & 0xFF) / 255.0f, // R
                    static_cast<float>((color >>  8) & 0xFF) / 255.0f, // G
                    static_cast<float>((color)       & 0xFF) / 255.0f, // B
                    static_cast<float>((color >> 24) & 0xFF) / 255.0f  // A
            };
}

// convert {R, G, B, A} to 0xAARRGGBB
int color_vec_to_int(const glm::vec4 & color) noexcept
{
    return
            (static_cast<unsigned int>(color.a * 255.0f) & 0xFF) << 24 | // A
                    (static_cast<unsigned int>(color.r * 255.0f) & 0xFF) << 16 | // R
                    (static_cast<unsigned int>(color.g * 255.0f) & 0xFF) <<  8 | // G
                    (static_cast<unsigned int>(color.b * 255.0f) & 0xFF);        // B
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
