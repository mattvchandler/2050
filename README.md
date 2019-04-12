# 2050

## About

This is a game loosely based on *2048*, but with circles instead of squares.

Much like the original, you start with small numbered pieces and
combine like-numbered pieces into progressively larger ones, working toward
2048.

Unlike the original, the pieces are free to move in any direction, bounce off of
the walls and each other, are subject to gravity, and grow progressively larger.

![gameplay screenshot](/metadata/en-US/images/phoneScreenshots/scrn_phone_land_1.png?raw=true)

## Gameplay

Swipe (or use arrow keys) to change the direction of gravity. For example, to
make the balls move downward, swipe down. To make them fall upwards, swipe up.
Gravity can be changed to any angle (unless playing with arrow keys, in which
case you are constrained to horizontal, vertical, and diagonals if your keypad
has them). A new ball is generated each time you swipe (or press an arrow key)

Alternatively, you can use your device's accelerometer (if it has one) to change
gravity by rotating your device. In-game gravity will point toward real gravity
(note: may not work correctly in space, other planets, free-fall, etc...)
New balls are generated as the device is turned more than a few degrees.

Manipulate the direction of gravity to touch and merge balls of the same size /
color / number together. For example, two '2' balls will make a '4' ball, two
'4's make an '8' ...

You win once you achieve a '2048' ball, but you have the option to continue
playing afterwards. Can you get to '4096'? It is possible. '8192'? I don't know
(but let me know if you do!)

As the number and size of balls increases, they will start to become tightly
packed together. Once the pressure reaches a certain threshold (indicated on a
pressure meter) the game is over.

## Why the name?

2050: it's 2048, but *rounded*.

## Credits

3rd party libraries (under 'libraries' dir):

FreeType copyright © 2019 The FreeType Project ([www.freetype.org](www.freetype.org)) under the FreeType License

GLM copyright © 2019 G-Truc Creation ([http://glm.g-truc.net/](http://glm.g-truc.net/)) under the MIT License

Textogl copyright © 2019 Matthew Chandler ([www.github.com/mattvchandler/textogl](www.github.com/mattvchandler/textogl)) under the MIT License
