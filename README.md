# Readme

## Todo

- [ ] Bug: Lava bleeds over to the other side of the map if you add it over the map edge.
- [x] Add a version number.
- [x] Make transparent things sample the background framebuffer.
- [ ] Make map buffers allocated.
  - [ ] Make map resizable.
  - [ ] Make starting camera position adjust to center over map regardless of size.
- [x] Move zooming camera to a function so the same code isn't everywhere.
- [ ] Try adding particles that spawn foam. use these as spawners for foam to get better looking foam transport.
- [-] Move rendering to different layers.
- [x] Make sure colors cannot overflow.
- [x] Move simulation functions into its own .c file
- [-] Move project to Clion
- [x] Add back lava.
- [x] Add back mist.
- [ ] Add all camera controls to mouse.
- [ ] Add touch support.
- [ ] Foam and sand transport doesn't work very good, mass isn't constant. Something to do with the advection, fix that.
- [ ] Get nicer colors for stuff.
- [ ] Make the html look nicer.
- [ ] Make sediment have color that gets transported with the sediment.
- [x] Move drawPoint and drawLine into window.c
- [ ] Split map into chunks, each chunk should keep track of what sort of fluids/sand it contains so it only need to update what is present.

## Done

- [x] Add back foam.

## Ideas

### Multithreading

Drawcolumn borde funkar bra att multithreada, problem kanske med map data som finns i flera kolumner samtidigt. DÅ kan jag dela in skärmen i grupper med kolumner och så kör jag förs en batch med trådar som tar varannan grupp och sedan när dom är klara tar jag resten. På så sätt borde inte någon data hanteras av flera trådar samtidigt.
Samma princip borde gå att applicera på simuleringen, Jag delar in kartan i segment och kör dom i två omgångar på så sätt att ingen tråd nuddar samma data samtidigt.

### sand optimization

Istället för att gå igenom varje cell och kolla efter celler som är lägre (Då måste vi kolla 9 celler för varje cell). Så kan vi gå igenom varje cell och kolla efter celler som är högre

## Notes

Build by running make with command:
C:\msys64\usr\bin\make.exe

Visa källkod i firefox, lägg i länkarn
-gsource-map --source-map-base <http://127.0.0.1:5500/>

från gammalt projekt
//emcc main.c -std=c17 -s WASM=1 -s USE_SDL=2 -s -s USE_SDL_IMAGE=2 -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0 -s SDL2_IMAGE_FORMATS='["png"]' -s TOTAL_MEMORY=536870912 -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=4 ^C-preload-file assets -O3 --profiling -o index.js

J. O’Brien and J. K. Hodgins. Dynamic simulation of splash-
ing fluids. In Proceedings of Computer Animation’95, pages
198–205, 1995.
<https://smartech.gatech.edu/bitstream/handle/1853/3599/94-32.pdf>

Fast Hydraulic Erosion Simulation and Visualization on
GPU
Xing Mei, Philippe Decaudin, Bao-Gang Hu
<https://hal.inria.fr/file/index/docid/402079/filename/FastErosion_PG07.pdf>

### launch.json

Paste this in launch.json, this disables CORS as well as enables multithreading in wasm

{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            "type": "chrome",
            "request": "launch",
            "name": "Launch Chrome against localhost",
            "url": "http://127.0.0.1:5500/index.html",
            "webRoot": "${workspaceFolder}",
            "runtimeArgs": ["--disable-web-security","--user-data-dir=c:\\chrome-browser", "--js-flags=--experimental-wasm-threads", "--enable-features=WebAssembly,SharedArrayBuffer"]
        }
    ]
}
