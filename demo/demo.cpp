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

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <cmath>

#include <GL/glew.h>

#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/System.hpp>

#include "textogl/font.hpp"
#include "textogl/static_text.hpp"

int main(int argc, char * argv[])
{
    if(argc < 3)
    {
        std::cerr<<"no font specified (need 2)"<<std::endl;
        return EXIT_FAILURE;
    }

    sf::Window win(sf::VideoMode(1024, 786), "Textogl Demo", sf::Style::Default, sf::ContextSettings(24, 8, 0, 3, 0));
    win.setKeyRepeatEnabled(false);

    // initialize glew
    if(glewInit() != GLEW_OK)
    {
        std::cerr<<"Error loading glew"<<std::endl;
        return EXIT_FAILURE;
    }

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    sf::Event ev;

    textogl::Font_sys font(argv[1], 32);
    textogl::Font_sys font2(argv[2], 72);

    textogl::Static_text static_text(font, u8"Static Text, with unicode: ø∅Ø💩‽");
    textogl::Static_text static_text2(font2, "Multiple fonts");
    textogl::Static_text rotating_text(font, "Rotation");
    textogl::Static_text text_3d(font2, "3D");

    std::vector<textogl::Static_text> static_arr;
    for(int i = 0; i < 10; ++i)
        static_arr.emplace_back(font2, std::to_string(i));

    const auto f = 1.0f / std::tan(static_cast<float>(M_PI) / 12.0f);
    const auto aspect = static_cast<float>(win.getSize().x) / static_cast<float>(win.getSize().y);
    const float near = 0.1f, far = 10.0f;
    const textogl::Mat4<float> projection
    {
        f / aspect, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, (far + near) / (near - far), -1.0f,
        0.0f, 0.0f, (2.0f * far * near) / (near - far), 0.0f
    };

    const textogl::Mat4<float> view
    {
         0.001f, 0.0f,    0.0f, 0.0f,
         0.0f,  -0.001f,  0.0f, 0.0f,
         0.0f,   0.0f,    1.0f, 0.0f,
        -0.05f,  0.0f,   -0.25f, 1.0f
    };

    bool running = true;
    float angle = 0.0f;
    while(running)
    {
        if(win.pollEvent(ev))
        {
            switch(ev.type)
            {
                // close window
                case sf::Event::Closed:
                    running = false;
                    break;

                case sf::Event::Resized:
                    glViewport(0, 0, win.getSize().x, win.getSize().y);
                    break;

                default:
                    break;
            }
        }

        static int frame_count = 0;
        static std::ostringstream fps_format;

        static auto last_frame = std::chrono::high_resolution_clock::now();
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<float, std::ratio<1, 1>>(now - last_frame);

        static float fps = 0.0f;

        if(duration > std::chrono::milliseconds(100))
        {
            fps = (float)frame_count / duration.count();
            last_frame = now;
            frame_count = 0;

            fps_format.str("");
            fps_format<<"Dynamic text: "<<std::setprecision(3)<<std::fixed<<fps<<" fps";
        }
        ++frame_count;

        angle += 0.05f * duration.count();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Red dynamic text, in UL corner
        font.render_text(fps_format.str(), textogl::Color{1.0f, 0.0f, 0.0f, 1.0f},
                textogl::Vec2<float>{(float)win.getSize().x, (float)win.getSize().y},
                textogl::Vec2<float>{0.0f, 0.0f}, textogl::ORIGIN_VERT_TOP | textogl::ORIGIN_HORIZ_LEFT);

        // Green static text, @ 0, 75
        static_text.render_text(textogl::Color{0.0f, 1.0f, 0.0f, 1.0f}, textogl::Vec2<float>{(float)win.getSize().x, (float)win.getSize().y},
                textogl::Vec2<float>{0.0f, 75.0f}, textogl::ORIGIN_VERT_TOP | textogl::ORIGIN_HORIZ_LEFT);

        // blue larger font @ 0, 150
        static_text2.render_text(textogl::Color{0.0f, 0.0f, 1.0f, 1.0f}, textogl::Vec2<float>{(float)win.getSize().x, (float)win.getSize().y},
                textogl::Vec2<float>{0.0f, 150.0f}, textogl::ORIGIN_VERT_TOP | textogl::ORIGIN_HORIZ_LEFT);

        // vertical array of numbers along right edge
        for(std::size_t i = 0; i < static_arr.size(); ++i)
            static_arr[i].render_text(textogl::Color{0.0f, 0.0f, 0.0f, 1.0f}, textogl::Vec2<float>{(float)win.getSize().x, (float)win.getSize().y},
                textogl::Vec2<float>{(float)win.getSize().x, (float)(i * 60)}, textogl::ORIGIN_VERT_TOP | textogl::ORIGIN_HORIZ_RIGHT);

        // 2D rotating text centered on 100, 300
        rotating_text.render_text_rotate(textogl::Color{0.0f, 0.0f, 0.0f, 1.0f}, textogl::Vec2<float>{(float)win.getSize().x, (float)win.getSize().y},
                textogl::Vec2<float>{100.0f, 300.0f}, angle, textogl::ORIGIN_VERT_CENTER | textogl::ORIGIN_HORIZ_CENTER);

        // 3D rotating / orbiting text
        textogl::Mat4<float> rotation
        {
            std::cos(angle), 0.0f, -std::sin(angle), 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            std::sin(angle), 0.0f, std::cos(angle), 0.0f,
            0.0f, 0.0f, -1.0f, 1.0f
        };

        text_3d.render_text_mat(textogl::Color{0.0, 0.0f, 0.0f, 1.0f}, projection * rotation * view);

        win.display();
    }

    win.close();

    return EXIT_SUCCESS;
}
