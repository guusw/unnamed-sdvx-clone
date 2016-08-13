# Unnamed SDVX clone
Trying to create a game based on [KShoot](http://kshoot.client.jp/)/[SDVX](https://www.youtube.com/watch?v=JBHKNl87juA).

[![Demo Video](http://img.youtube.com/vi/finlBLaO9Fw/0.jpg)](http://tdrz.nl/oo9LIJXk)

Only tested to compile with Visual Studio 2015 / Windows Only.

Uses OpenGL 4

### Currently implemented features:
- Fonts
- Basic GUI (Buttons, Sliders, Scroll Boxes)
- jpeg and png image loading
- OGG audio streaming with timing management, WAV samples.
- Loading KShoot format maps (*.ksh)
- Gameplay + Scoring + Autoplay
- Real-time laser sound Filters and button Effects.
- Extended lasers
- Track zoom
- Track roll animations
- Song database (sqlite3)
- Song selection screen

### Features currently in the queue / in progress:
- Linux port
- Customizable controls
- Mouse/Controller input while in game
- Score screen

### How to run:
Run the application
You can also run the application with the full path to a .ksh file as the first argument to play that map immediatly, this can be combined with any of the additional command line switches below.

Additional command line parameters
- '-mute' mutes all audio output
- '-autoplay' no user input required
- '-autoskip' automatically skips song intro's
- '-convertmaps' allows converting a loading maps to a binary format that loads faster (experimental, not feature complete)
- '-debug' used to show relevant debug info in game such as hit timings, and scoring debug info.
- '-test' runs test scene, only used for development purposed without running the game.

### Game Controls (to be customizable soon):
White buttons = [S] [D] [K] [L]

Yellow notes = [C] [M] 

Left Laser = [W] [E] 
 Move left / move right respectively

Right Laser = [O] [P]
