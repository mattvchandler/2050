/// @file
/// @brief Static text object

// Copyright 2018 Matthew Chandler

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

#ifndef STATIC_TEXT_HPP
#define STATIC_TEXT_HPP

#include "font.hpp"

/// OpenGL Font rendering types

/// @ingroup textogl
namespace textogl
{
    /// Object for text which does not change often

    /// Font_sys::render_text will re-build the OpenGL primitives on each call,
    /// which is inefficient if the text doesn't change every frame.
    ///
    /// This class is for text that doesn't need to change frequently.
    /// Text is initially built with the #Static_text constructor and can be
    /// changed with \ref set_text.
    class Static_text
    {
    public:
        /// Create and build text object
        /// @param font Font_sys object containing desired font. This Static_text
        ///        will retain a shared_ptr to the Font_sys, but will not automatically
        ///        rebuild when Font_sys::resize is called. Use Static_text::set_font_sys
        //         to rebuild in that case.
        /// @param utf8_input Text to render, in UTF-8 encoding. For best performance, normalize the string before rendering
        Static_text(Font_sys & font,
                    const std::string & utf8_input
                    );

        /// Recreate text object with new Font_sys

        /// When Font_sys::resize has been called, call this to rebuild this Static_text with the new size

        /// @param font Font_sys object containing desired font.
        void set_font_sys(Font_sys & font);

        /// Recreate text object with new string

        /// @param utf8_input Text to render, in UTF-8 encoding. For best performance, normalize the string before rendering
        void set_text(const std::string & utf8_input);

        /// Render the previously set text
        void render_text(const Color & color,          ///< Text Color
                            const Vec2<float> & win_size, ///< Window dimensions. A Vec2 with X = width and Y = height
                            const Vec2<float> & pos,      ///< Render position, in screen pixels
                            const int align_flags = 0     ///< Text Alignment. Should be #Text_origin flags bitwise-OR'd together
                        );

    private:
        struct Impl; ///< Private internal implementation
        std::unique_ptr<Impl, void (*)(Impl *)> pimpl; ///< Pointer to private internal implementation
    };
}

#endif // STATIC_TEXT_HPP
