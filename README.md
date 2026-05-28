# Sorting Visualizer

Interactive sorting algorithm visualizer built in C with SDL2 and SDL2_ttf. Features 10 algorithms, real-time audio feedback, and a polished dark-themed UI.

## Requirements

- SDL2, SDL2_ttf, and a font (DejaVu Sans / Liberation Sans)

```sh
# Ubuntu / Debian
sudo apt install libsdl2-dev libsdl2-ttf-dev fonts-dejavu

# Arch Linux
sudo pacman -S sdl2 sdl2_ttf ttf-dejavu

# Fedora
sudo dnf install SDL2-devel SDL2_ttf-devel dejavu-sans-fonts
```

## Clone, build, run

```sh
git clone https://github.com/vajradevam/sortvis.git
cd sortvis
make
./sortvis
```

Clean:

```sh
make clean
```

## Controls

| Control | Description |
|---|---|
| **Algo ◄ ►** | Switch sorting algorithm |
| **Items ◄ ►** | Adjust array size (10–300) |
| **Shuffle** | Randomize the array |
| **Sort! / Stop** | Start or interrupt sorting |
| **Delay ◄ ►** | Adjust animation speed (1–500ms) |

### Delay stepping

| Current delay | Step |
|---|---|
| ≥ 200ms | 25ms |
| 100–199ms | 10ms |
| 20–99ms | 5ms |
| 5–19ms | 2ms |
| 1–4ms | 1ms |

At delays ≤ 3ms, rendering is decimated (1 in 50 frames rendered) for maximum speed with large arrays.

## Algorithms

- Bubble Sort
- Selection Sort
- Insertion Sort
- Quick Sort
- Merge Sort
- Heap Sort
- Shell Sort
- Cocktail Shaker Sort
- Gnome Sort
- Radix Sort (LSD)

## Color legend

- **Blue–teal–green–yellow–peach gradient** — unsorted values
- **Orange bar** — currently compared element
- **Magenta bar** — secondary compared element
- **Vibrant green** — sorted portion
- **Pulsing white flash** — sort completion celebration

## Project structure

```
├── Makefile
├── README.md
├── src/
│   ├── main.c       — entry point, window/event loop
│   ├── renderer.c   — SDL rendering, UI, audio, step callback
│   ├── renderer.h
│   ├── sorter.c     — 10 sorting algorithms + state management
│   └── sorter.h
└── .gitignore
```
