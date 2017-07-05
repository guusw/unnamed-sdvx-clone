# Unnamed SDVX clone
A game based on [KShoot](http://kshoot.client.jp/) and [SDVX](https://www.youtube.com/watch?v=JBHKNl87juA).

[![Build](https://ci.appveyor.com/api/projects/status/github/guusw/unnamed-sdvx-clone?branch=master&svg=true&retina=true)](https://ci.appveyor.com/project/guusw/unnamed-sdvx-clone)

Demo Videos (Song Select/ Gameplay / Realtime effects):

[![Song Select Video](http://img.youtube.com/vi/GYOHy5CY2pU/1.jpg)](https://www.youtube.com/watch?v=GYOHy5CY2pU)
[![Gameplay Video](http://img.youtube.com/vi/dfEbmBzdvYA/1.jpg)](https://www.youtube.com/watch?v=dfEbmBzdvYA)
[![Realtime Effects Video](http://img.youtube.com/vi/PVIAENg13OU/1.jpg)](https://www.youtube.com/watch?v=PVIAENg13OU)

### Current features:
- Basic GUI (Buttons, Sliders, Scroll Boxes)
- OGG/MP3 Audio streaming (with preloading for gameplay performance)
- Loading KShoot format maps (*.ksh) (1.6 supported)
- Functional gameplay and scoring
- Autoplay
- Basic controller support (Still some issues with TC+)
- Changable settings and key mapping in configuration file
- Real time sound effect and effect track support
- Song database for near-instant game startup (sqlite3)
- Song database searching
- Linux/Windows support

### Features currently on hold / in progress:
- GUI System rewrite
- Saving of local scores
- Song select UI/Controls to change hispeed and other game settings

### How to build:
- Install CMake (https://cmake.org/download/)
- **linux** check 'build.linux' for libraries to install
- Run 'cmake .' from the root of the project
- **windows** build the generated visual studio project 'FX.sln'
- **linux** run 'make' from the project root
- Binaries get build to the 'bin' folder, run the excutable from there
- **windows** to run from visual studio, goto properties for Main > Debugging > Working Directory and set it to $(OutDir) or ..\bin

### How to run:
- Just run Main_Release or Main_Debug from within the bin folder
- You can also run the application with the full path to a .ksh file as the first argument to play that map immediatly, this can be combined with any of the additional command line switches below.

Additional command line parameters:
- '-mute' mutes all audio output
- '-autoplay' no user input required
- '-autoskip' automatically skips song intro's
- '-convertmaps' allows converting a loading maps to a binary format that loads faster (experimental, not feature complete)
- '-debug' used to show relevant debug info in game such as hit timings, and scoring debug info.
- '-test' runs test scene, only used for development purposed without running the game.
- '-autobuttons' makes all BT/FX object be played automatically. so you only have to control the lasers

### Game Controls (Customizable, read **Readme_Input.txt**):
- White buttons = [S] [D] [K] [L]
- Yellow notes = [C] [M] 
- Left Laser = [W] [E] 
 Move left / move right respectively
- Right Laser = [O] [P]
- Back to song select [Esc]

### Song Select Controls:
- Use the arrow keys to select a song and difficulty
- Use [Page Down]/[Page Up] to scroll faster
- Press [Enter] to start a song
- Press [Ctrl]+[Enter] to start autoplay
- Use the Search bar on the top to search for songs.

The folder that is scanned for songs can be changed in the "Main.cfg" file (songfolder = "path to my song folder").
If anything breaks in the song database delete "maps.db"
