# Unnamed SDVX clone
Trying to create a game based on [KShoot](http://kshoot.client.jp/)/[SDVX](https://www.youtube.com/watch?v=JBHKNl87juA).

[![Demo Video](http://img.youtube.com/vi/finlBLaO9Fw/0.jpg)](http://tdrz.nl/oo9LIJXk)

Only tested to compile with Visual Studio 2015 / Windows Only.

Uses OpenGL 4

### Currently implemented features:
- Fonts
- jpeg and png image loading
- Ogg audio streaming with timing management.
- Loading KShoot format maps (*.ksh)
- Basic Gameplay
- Real-time laser sound Filters and button Effects.
- Track zoom
- Track roll animations

### How to run:
Just run the application with the full path to a .ksh file as the first argument

Additional command line parameters
- '-mute' mutes all audio output
- '-autoplay' no user input required
- '-autoskip' automatically skips song intro's
- '-convertmaps' allows converting a loading maps to a binary format that loads faster (experimental, not feature complete)

### Game Controls:
White buttons = [S] [D] [K] [L]

Yellow notes = [C] [M] 

Left Laser = [W] [E] 
 Move left / move right respectively

Right Laser = [O] [P]
