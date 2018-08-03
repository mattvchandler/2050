#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

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
        std::cerr<<"no font specified"<<std::endl;
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

    textogl::Static_text static_text(font, u8"Static ASDF! ø∅Ø💩‽");
    textogl::Static_text static_text2(font2, "GIANT TEXT IS THE\nBEST KIND OF TEXT");
    textogl::Static_text static_text3(font, "More text");

    std::vector<textogl::Font_sys> font_arr;
    std::vector<textogl::Static_text> static_arr;
    for(int i = 0; i < 10; ++i)
    {
        font_arr.emplace_back(argv[2], 72);
        static_arr.emplace_back(font_arr[i], std::to_string(i));
    }

    bool running = true;
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
            fps_format<<std::setprecision(3)<<std::fixed<<fps<<" fps";
        }
        ++frame_count;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        font.render_text(fps_format.str(), textogl::Color{1.0f, 0.0f, 0.0f, 1.0f},
                textogl::Vec2<float>{(float)win.getSize().x, (float)win.getSize().y},
                textogl::Vec2<float>{0.0f, 0.0f}, textogl::ORIGIN_VERT_TOP | textogl::ORIGIN_HORIZ_LEFT);

        static_text.render_text(textogl::Color{0.0f, 1.0f, 0.0f, 1.0f}, textogl::Vec2<float>{(float)win.getSize().x, (float)win.getSize().y},
                textogl::Vec2<float>{0.0f, 100.0f}, textogl::ORIGIN_VERT_TOP | textogl::ORIGIN_HORIZ_LEFT);

        font2.render_text("ASDF", textogl::Color{0.0f, 0.0f, 1.0f, 1.0f},
                textogl::Vec2<float>{(float)win.getSize().x, (float)win.getSize().y},
                textogl::Vec2<float>{0.0f, 200.0f}, textogl::ORIGIN_VERT_TOP | textogl::ORIGIN_HORIZ_LEFT);

        static_text2.render_text(textogl::Color{0.0f, 0.0f, 0.0f, 1.0f}, textogl::Vec2<float>{(float)win.getSize().x, (float)win.getSize().y},
                textogl::Vec2<float>{0.0f, 300.0f}, textogl::ORIGIN_VERT_TOP | textogl::ORIGIN_HORIZ_LEFT);

        static_text3.render_text(textogl::Color{0.0f, 1.0f, 1.0f, 1.0f}, textogl::Vec2<float>{(float)win.getSize().x, (float)win.getSize().y},
                textogl::Vec2<float>{0.0f, 550.0f}, textogl::ORIGIN_VERT_TOP | textogl::ORIGIN_HORIZ_LEFT);

        for(std::size_t i = 0; i < font_arr.size(); ++i)
            static_arr[i].render_text(textogl::Color{0.0f, 0.0f, 0.0f, 1.0f}, textogl::Vec2<float>{(float)win.getSize().x, (float)win.getSize().y},
                textogl::Vec2<float>{win.getSize().x, i * 60}, textogl::ORIGIN_VERT_TOP | textogl::ORIGIN_HORIZ_RIGHT);

        win.display();
    }

    win.close();

    return EXIT_SUCCESS;
}
