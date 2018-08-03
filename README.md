# Textogl - A simple OpenGL text rendering library.

Textogl renders text in OpenGL from nearly any font (any format supported by
Freetype).

### [textogl on Github](https://github.com/mattvchandler/textogl)

### [Documentation](https://mattvchandler.github.io/textogl/index.html)

## Usage

1. Create a textogl::Font_sys object for the desired font.
2. Call textogl::Font_sys::render_text() with the desired text, position, and
   color.
3. If the text will not change each frame, consider using textogl::Static_text
   object. This will prevent needing to rebuild quads for each rendering call

## Building & Installation

### Dependencies

* [Freetype](https://www.freetype.org/)
* OpenGL 3.3 + OR OpenGL ES 2.0+
* GLM (Optional - Allows passing glm vectors to textogl for colors and positions)
* Compiler supporting c++11

Textogl uses CMake to build

    $ mkdir build && cd build
    $ cmake .. -DCMAKE_INSTALL_PREFIX=<prefix path> # add -DBUILD_SHARED_LIBS=1 for a shared libary, add -DUSE_GLM=0 to skip checking for GLM
    $ make
    # make install

#### Debian & derivatives
Sqlitepp is configured to generate a .deb package file. To do so, substitute the
above with the following:

    $ mkdir build && cd build
    $ cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_SHARED_LIBS=1
    $ make
    $ cpack
    # dpkg -i libtextogl-dev*.deb

#### Documentation
If doxygen is installed, library documentation can be generated with: `$ make doc`
