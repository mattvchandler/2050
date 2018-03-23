/// @file
/// @brief Font loading and text display implementation

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

#include "font_impl.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <system_error>

/// Convert a UTF-8 string to a UTF-32 string

/// Minimal verification is done to ensure that the input is valid UTF-8. If any
/// invalid byte sequences are detected, they will be replaced by the replacement
/// character: '�'
/// @param utf8 UTF-8 string to convert
/// @returns UTF-32 string corresponding to the input
std::u32string utf8_to_utf32(const std::string & utf8)
{
    std::u32string utf32;

    uint32_t code_pt = 0;
    int expected_bytes = 0;

    for(const uint8_t & byte: utf8)
    {
        // detect invalid bytes
        if(byte == 0xC0 || byte == 0xC1 || byte >= 0xF5)
        {
            utf32.push_back(U'�');
            expected_bytes = 0;
        }
        // 0b0xxxxxxx: single-byte char (ASCII)
        else if((byte & 0x80) == 0)
        {
            if(expected_bytes != 0)
            {
                // previous sequence ended prematurely. add replacement char
                utf32.push_back(U'�');
                expected_bytes = 0;
            }
            utf32.push_back(byte);
        }
        // 0b11xxxxxx: leading byte
        else if((byte & 0xC0) == 0xC0)
        {
            if(expected_bytes != 0)
            {
                // previous sequence ended prematurely. add replacement char
                utf32.push_back(U'�');
            }

            // 2-byte char
            if((byte & 0xE0) == 0xC0)
            {
                code_pt = byte & 0x1F;
                expected_bytes = 1;
            }
            // 3-byte char
            else if((byte & 0xF0) == 0xE0)
            {
                code_pt = byte & 0x0F;
                expected_bytes = 2;
            }
            // 4-byte char
            else if((byte & 0xF8) == 0xF0)
            {
                code_pt = byte & 0x07;
                expected_bytes = 3;
            }
            else
            {
                // invalid value. insert the replacement char
                utf32.push_back(U'�');
                expected_bytes = 0;
            }
        }
        // 0b10xxxxxx: continuation byte
        else // (byte & 0xC0) == 0x80
        {
            if(expected_bytes == 0)
            {
                // continuation byte w/o leader. replace w/ replacement char
                utf32.push_back(U'�');
            }
            else
            {
                code_pt <<= 6;
                code_pt |= byte & 0x3F;

                if(--expected_bytes == 0)
                {
                    utf32.push_back(code_pt);
                }
            }
        }
    }

    if(expected_bytes > 0)
    {
        // end of string but still expecting continuation bytes. use the replacement char
        utf32.push_back(U'�');
    }

    return utf32;
}

namespace textogl
{
    Font_sys::Font_sys(const std::string & font_path, const unsigned int font_size,
            const unsigned int v_dpi, const unsigned int h_dpi):
        pimpl(new Impl(font_path, font_size, v_dpi, h_dpi), [](Impl * impl) { delete impl; })
    {}
    Font_sys::Impl::Impl(const std::string & font_path, const unsigned int font_size,
            const unsigned int v_dpi, const unsigned int h_dpi)
    {
        std::ifstream font_file(font_path, std::ios_base::binary);
        _font_data.insert(std::begin(_font_data), std::istreambuf_iterator<char>(font_file), std::istreambuf_iterator<char>());
        init(font_size, v_dpi, h_dpi);
    }

    Font_sys::Font_sys(const std::vector<unsigned char> & font_data, const unsigned int font_size,
            const unsigned int v_dpi, const unsigned int h_dpi):
        pimpl(new Impl(font_data, font_size, v_dpi, h_dpi), [](Impl * impl) { delete impl; })
    {}
    Font_sys::Impl::Impl(const std::vector<unsigned char> & font_data, const unsigned int font_size,
            const unsigned int v_dpi, const unsigned int h_dpi): _font_data(font_data)
    {
        init(font_size, v_dpi, h_dpi);
    }

    void Font_sys::Impl::init(const unsigned int font_size, const unsigned int v_dpi, const unsigned int h_dpi)
    {
        // load freetype, and text shader - only once
        if(_common_ref_cnt == 0)
            _common_data.reset(new Font_common);

        // open the font file
        FT_Open_Args args
        {
            FT_OPEN_MEMORY,
            _font_data.data(),
            static_cast<FT_Long>(_font_data.size()),
            nullptr,
            nullptr,
            nullptr,
            0,
            nullptr
        };
        FT_Error err = FT_Open_Face(_common_data->ft_lib, &args, 0, &_face);

        if(err != FT_Err_Ok)
        {
            if(_common_ref_cnt == 0)
                _common_data.reset();

            if(err == FT_Err_Unknown_File_Format)
            {
                throw std::system_error(err, std::system_category(), "Unknown format for font file");
            }
            else
            {
                throw std::ios_base::failure("Error reading font file");
            }
        }

        // select unicode charmap (should be default for most fonts)
        if(FT_Select_Charmap(_face, FT_ENCODING_UNICODE) != FT_Err_Ok)
        {
            FT_Done_Face(_face);

            if(_common_ref_cnt == 0)
                _common_data.reset();

            throw std::runtime_error("No unicode charmap in font file");
        }

        try
        {
            resize(font_size, v_dpi, h_dpi);
        }
        catch(std::runtime_error &)
        {
            FT_Done_Face(_face);

            if(_common_ref_cnt == 0)
                _common_data.reset();

            throw;
        }

        // we're not going to throw now, so increment library ref count
        ++_common_ref_cnt;

        // create and set up vertex array and buffer
#ifndef USE_OPENGL_ES
        glGenVertexArrays(1, &_vao);
        glBindVertexArray(_vao);
#endif

        glGenBuffers(1, &_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);

#ifndef USE_OPENGL_ES
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(Vec2<float>), NULL);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(Vec2<float>), (const GLvoid *)sizeof(Vec2<float>));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
#endif

        // get shader uniform locations
        glUseProgram(_common_data->prog);
        glUniform1i(_common_data->uniform_locations["font_page"], 14);
        glUseProgram(0);
    }

    Font_sys::Impl::~Impl()
    {
        FT_Done_Face(_face);

        // only deallocate shared libs if this is the last Font_sys obj
        if(--_common_ref_cnt == 0)
            _common_data.reset();

        // destroy VAO/VBO
        glDeleteBuffers(1, &_vbo);
#ifndef USE_OPENGL_ES
        glDeleteVertexArrays(1, &_vao);
#endif

        // destroy textures
        for(auto & i: _page_map)
        {
            glDeleteTextures(1, &i.first);
        }
    }

    Font_sys::Impl::Impl(Impl && other):
        _face(other._face),
        _has_kerning_info(other._has_kerning_info),
        _cell_bbox(std::move(other._cell_bbox)),
        _line_height(other._line_height),
        _tex_width(other._tex_width),
        _tex_height(other._tex_height),
        _page_map(std::move(other._page_map)),
#ifndef USE_OPENGL_ES
        _vao(other._vao),
#endif
        _vbo(other._vbo)
    {
        other._face = nullptr;
#ifndef USE_OPENGL_ES
        other._vao = 0;
#endif
        other._vbo = 0;
        ++_common_ref_cnt;
    }
    Font_sys::Impl & Font_sys::Impl::operator=(Impl && other)
    {
        if(this != &other)
        {
            _face = other._face;
            _has_kerning_info = other._has_kerning_info;
            _cell_bbox = std::move(other._cell_bbox);
            _line_height = other._line_height;
            _tex_width = other._tex_width;
            _tex_height = other._tex_height;
            _page_map = std::move(other._page_map);
#ifndef USE_OPENGL_ES
            _vao = other._vao;
#endif
            _vbo = other._vbo;

            other._face = nullptr;
#ifndef USE_OPENGL_ES
            other._vao = 0;
#endif
            other._vbo = 0;
        }
        return *this;
    }

    void Font_sys::resize(const unsigned int font_size, const unsigned int v_dpi, const unsigned int h_dpi)
    {
        pimpl->resize(font_size, v_dpi, h_dpi);
    }
    void Font_sys::Impl::resize(const unsigned int font_size, const unsigned int v_dpi, const unsigned int h_dpi)
    {
        // select font size
        if(FT_Set_Char_Size(_face, font_size * 64, font_size * 64, h_dpi, v_dpi) != FT_Err_Ok)
            throw std::runtime_error("Can't set font size: " + std::to_string(font_size));

        // get bounding box that will fit any glyph, plus 2 px padding
        // some glyphs overflow the reported box (antialiasing?) so at least one px is needed
        _cell_bbox.ul.x = FT_MulFix(_face->bbox.xMin, _face->size->metrics.x_scale) / 64 - 2;
        _cell_bbox.ul.y = FT_MulFix(_face->bbox.yMax, _face->size->metrics.y_scale) / 64 + 2;
        _cell_bbox.lr.x = FT_MulFix(_face->bbox.xMax, _face->size->metrics.x_scale) / 64 + 2;
        _cell_bbox.lr.y = FT_MulFix(_face->bbox.yMin, _face->size->metrics.y_scale) / 64 - 2;

        // get newline height
        _line_height = FT_MulFix(_face->height, _face->size->metrics.y_scale) / 64;

        _tex_width = _cell_bbox.width() * 16;
        _tex_height = _cell_bbox.height() * 16;

        _has_kerning_info = FT_HAS_KERNING(_face);

        _page_map.clear();
    }

    void Font_sys::render_text(const std::string & utf8_input, const Color & color,
            const Vec2<float> & win_size, const Vec2<float> & pos, const int align_flags)
    {
        pimpl->render_text(utf8_input, color, win_size, pos, align_flags);
    }
    void Font_sys::Impl::render_text(const std::string & utf8_input, const Color & color,
            const Vec2<float> & win_size, const Vec2<float> & pos, const int align_flags)
    {
        // build text buffer objs
        std::vector<Vec2<float>> coords;
        std::vector<Font_sys::Impl::Coord_data> coord_data;
        Font_sys::Impl::Bbox<float> text_box;
        std::tie(coords, coord_data, text_box) = build_text(utf8_input);

        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
#ifndef USE_OPENGL_ES
        glBindVertexArray(_vao);
#endif

        // load text into buffer object
        // call glBufferData with NULL first - this is apparently faster for dynamic data loading
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vec2<float>) * coords.size(), NULL, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec2<float>) * coords.size(), coords.data());

        render_text_common(color, win_size, pos, align_flags, text_box, coord_data,
#ifndef USE_OPENGL_ES
                    _vao,
#endif
                    _vbo);
    }

    void Font_sys::Impl::render_text_common(const Color & color, const Vec2<float> & win_size,
            const Vec2<float> & pos, const int align_flags, const Bbox<float> & text_box,
            const std::vector<Coord_data> & coord_data,
#ifndef USE_OPENGL_ES
             GLuint vao,
#endif
             GLuint vbo
            )
    {
        Vec2<float> start_offset = pos;

        // offset origin to align to text bounding box
        int horiz_align = align_flags & 0x3;
        switch(horiz_align)
        {
            case ORIGIN_HORIZ_BASELINE:
                break;
            case ORIGIN_HORIZ_LEFT:
                start_offset.x -= text_box.ul.x;
                break;
            case ORIGIN_HORIZ_RIGHT:
                start_offset.x -= text_box.lr.x;
                break;
            case ORIGIN_HORIZ_CENTER:
                start_offset.x -= text_box.ul.x + text_box.width() / 2.0f;
                break;
        }

        int vert_align = align_flags & 0xC;
        switch(vert_align)
        {
            case ORIGIN_VERT_BASELINE:
                break;
            case ORIGIN_VERT_TOP:
                start_offset.y -= text_box.ul.y;
                break;
            case ORIGIN_VERT_BOTTOM:
                start_offset.y -= text_box.lr.y;
                break;
            case ORIGIN_VERT_CENTER:
                start_offset.y -= text_box.lr.y + text_box.height() / 2.0f;
                break;
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
#ifndef USE_OPENGL_ES
        glBindVertexArray(vao);
#else
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(Vec2<float>), NULL);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(Vec2<float>), (const GLvoid *)sizeof(Vec2<float>));
        glEnableVertexAttribArray(1);
#endif

        // set up shader uniforms
        glUseProgram(_common_data->prog);
        glUniform2fv(_common_data->uniform_locations["start_offset"], 1, &start_offset[0]);
        glUniform2fv(_common_data->uniform_locations["win_size"], 1, &win_size[0]);
        glUniform4fv(_common_data->uniform_locations["color"], 1, &color[0]);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glActiveTexture(GL_TEXTURE14);

        // draw text, per page
        for(const auto & cd: coord_data)
        {
            // bind the page's texture
            glBindTexture(GL_TEXTURE_2D, _page_map[cd.page_no].tex);
            glDrawArrays(GL_TRIANGLES, cd.start, cd.num_elements);
        }

#ifndef USE_OPENGL_ES
        glBindVertexArray(0);
#endif
    }

    std::unordered_map<uint32_t, Font_sys::Impl::Page>::iterator Font_sys::Impl::load_page(const uint32_t page_no)
    {
        // this assumes the page has not been created yet
        auto page_i = _page_map.emplace(std::make_pair(page_no, Page())).first;
        Page & page = page_i->second;

        // greyscale pixel storage
        std::vector<char> tex_data(_tex_width * _tex_height, 0);

        FT_GlyphSlot slot = _face->glyph;

        // load each glyph in the page (256 per page)
        for(uint32_t code_pt = page_no << 8; code_pt < ((page_no + 1) << 8); code_pt++)
        {
            unsigned short tbl_row = (code_pt >> 4) & 0xF;
            unsigned short tbl_col = code_pt & 0xF;

            // have freetype render the glyph
            FT_UInt glyph_i = FT_Get_Char_Index(_face, code_pt);
            if(FT_Load_Glyph(_face, glyph_i, FT_LOAD_RENDER) != FT_Err_Ok)
            {
                std::cerr<<"Err loading glyph for: "<<std::hex<<std::showbase<<code_pt;
                continue;
            }

            FT_Bitmap * bmp = &slot->bitmap;
            Char_info & c = page.char_info[code_pt & 0xFF];

            // set glyph properties
            c.origin.x = -_cell_bbox.ul.x + slot->bitmap_left;
            c.origin.y = _cell_bbox.ul.y - slot->bitmap_top;
            c.bbox.ul.x = slot->bitmap_left;
            c.bbox.ul.y = slot->bitmap_top;
            c.bbox.lr.x = (int)bmp->width + slot->bitmap_left;
            c.bbox.lr.y = slot->bitmap_top - (int)bmp->rows;
            c.advance.x = slot->advance.x;
            c.advance.y = slot->advance.y;
            c.glyph_i = glyph_i;

            // copy glyph from freetype to texture storage
            // TODO: we are assuming bmp->pixel_mode == FT_PIXEL_MODE_GRAY here.
            // We will probably want to allow other formats at some point
            for(std::size_t y = 0; y < (std::size_t)bmp->rows; ++y)
            {
                for(std::size_t x = 0; x < (std::size_t)bmp->width; ++x)
                {
                    long tbl_img_y = tbl_row * _cell_bbox.height() + _cell_bbox.ul.y - slot->bitmap_top + y;
                    long tbl_img_x = tbl_col * _cell_bbox.width() - _cell_bbox.ul.x + slot->bitmap_left + x;

                    tex_data[tbl_img_y * _tex_width + tbl_img_x] = bmp->buffer[y * bmp->width + x];
                }
            }
        }

        // copy data to a new opengl texture
        glGenTextures(1, &page.tex);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, page.tex);
#ifndef USE_OPENGL_ES
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, _tex_width, _tex_height, 0, GL_RED, GL_UNSIGNED_BYTE, tex_data.data());
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, _tex_width, _tex_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, tex_data.data());
#endif

        glGenerateMipmap(GL_TEXTURE_2D);
        // set params
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        return page_i;
    }

    std::tuple<std::vector<Vec2<float>>, std::vector<Font_sys::Impl::Coord_data>, Font_sys::Impl::Bbox<float>>
    Font_sys::Impl::build_text(const std::string & utf8_input)
    {
        Vec2<float> pen{0.0f, 0.0f};

        // verts by font page
        std::unordered_map<uint32_t, std::vector<Vec2<float>>> screen_and_tex_coords;

        Bbox<float> font_box;

        font_box.ul.x = std::numeric_limits<float>::max();
        font_box.ul.y = std::numeric_limits<float>::max();
        font_box.lr.x = std::numeric_limits<float>::min();
        font_box.lr.y = std::numeric_limits<float>::min();

        FT_UInt prev_glyph_i = 0;

        for(auto & code_pt : utf8_to_utf32(utf8_input))
        {
            // handle newlines
            if(code_pt == '\n')
            {
                pen.x = 0;
                pen.y += _line_height;
                prev_glyph_i = 0;
                continue;
            }

            // get font page struct
            uint32_t page_no = code_pt >> 8;
            auto page_i = _page_map.find(page_no);

            // load page if not already loaded
            if(page_i == _page_map.end())
            {
                page_i = load_page(code_pt >> 8);
            }

            Font_sys::Impl::Page & page = page_i->second;
            Font_sys::Impl::Char_info & c = page.char_info[code_pt & 0xFF];

            // add kerning if necessary
            if(_has_kerning_info && prev_glyph_i && c.glyph_i)
            {
                FT_Vector kerning = {0, 0};
                if(FT_Get_Kerning(_face, prev_glyph_i, c.glyph_i, FT_KERNING_DEFAULT, &kerning) != FT_Err_Ok)
                {
                    std::cerr<<"Can't load kerning for: "<<std::hex<<std::showbase<<code_pt;
                }
                pen.x += kerning.x / 64;
                pen.y -= kerning.y / 64;
            }

            std::size_t tex_row = (code_pt >> 4) & 0xF;
            std::size_t tex_col = code_pt & 0xF;

            // texture coord of glyph's origin
            Vec2<float> tex_origin = {(float)(tex_col * _cell_bbox.width() - _cell_bbox.ul.x),
                (float)(tex_row * _cell_bbox.height() + _cell_bbox.ul.y)};

            // push back vertex coords, and texture coords, interleaved, into a map by font page
            // 1 unit to pixel scale
            // lower left corner
            screen_and_tex_coords[page_no].push_back({pen.x + c.bbox.ul.x,
                    pen.y - c.bbox.lr.y});
            screen_and_tex_coords[page_no].push_back({(tex_origin.x + c.bbox.ul.x) / _tex_width,
                    (tex_origin.y - c.bbox.lr.y) / _tex_height});
            // lower right corner
            screen_and_tex_coords[page_no].push_back({pen.x + c.bbox.lr.x,
                    pen.y - c.bbox.lr.y});
            screen_and_tex_coords[page_no].push_back({(tex_origin.x + c.bbox.lr.x) / _tex_width,
                    (tex_origin.y - c.bbox.lr.y) / _tex_height});
            // upper left corner
            screen_and_tex_coords[page_no].push_back({pen.x + c.bbox.ul.x,
                    pen.y - c.bbox.ul.y});
            screen_and_tex_coords[page_no].push_back({(tex_origin.x + c.bbox.ul.x) / _tex_width,
                    (tex_origin.y - c.bbox.ul.y) / _tex_height});

            // upper left corner
            screen_and_tex_coords[page_no].push_back({pen.x + c.bbox.ul.x,
                    pen.y - c.bbox.ul.y});
            screen_and_tex_coords[page_no].push_back({(tex_origin.x + c.bbox.ul.x) / _tex_width,
                    (tex_origin.y - c.bbox.ul.y) / _tex_height});
            // lower right corner
            screen_and_tex_coords[page_no].push_back({pen.x + c.bbox.lr.x,
                    pen.y - c.bbox.lr.y});
            screen_and_tex_coords[page_no].push_back({(tex_origin.x + c.bbox.lr.x) / _tex_width,
                    (tex_origin.y - c.bbox.lr.y) / _tex_height});
            // upper right corner
            screen_and_tex_coords[page_no].push_back({pen.x + c.bbox.lr.x,
                    pen.y - c.bbox.ul.y});
            screen_and_tex_coords[page_no].push_back({(tex_origin.x + c.bbox.lr.x) / _tex_width,
                    (tex_origin.y - c.bbox.ul.y) / _tex_height});

            // expand bounding box for whole string
            font_box.ul.x = std::min(font_box.ul.x, pen.x + c.bbox.ul.x);
            font_box.ul.y = std::min(font_box.ul.y, pen.y - c.bbox.ul.y);
            font_box.lr.x = std::max(font_box.lr.x, pen.x + c.bbox.lr.x);
            font_box.lr.y = std::max(font_box.lr.y, pen.y - c.bbox.lr.y);

            // advance to next origin
            pen.x += c.advance.x / 64;
            pen.y -= c.advance.y / 64;

            prev_glyph_i = c.glyph_i;
        }

        // reorganize texture data into a contiguous array
        std::vector<Vec2<float>> coords;
        std::vector<Font_sys::Impl::Coord_data> coord_data;


        for(const auto & page: screen_and_tex_coords)
        {
            coord_data.emplace_back();
            Font_sys::Impl::Coord_data & c = coord_data.back();

            c.page_no = page.first;

            c.start = coords.size() / 2;
            coords.insert(coords.end(), page.second.begin(), page.second.end());
            c.num_elements = coords.size() / 2 - c.start;
        }

        return std::make_tuple(coords, coord_data, font_box);
    }
}
