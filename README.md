# FAST Lost & Found Management System

A simple desktop application made in **C** using **Raylib**. It helps
FAST students report lost or found items and connect with other students
to return belongings.

## Features

-   Report a lost item or register a found item.
-   Add item details such as category, description, location, date and
    colour.
-   Browse found items and search by item name or location.
-   View possible matches for a lost item.
-   Send verification requests and exchange messages.
-   Track item status, including verification and return.
-   Save reports and messages locally in the `data/` folder.
-   Switch between three demo student profiles.

## Requirements

-   C compiler (GCC / MinGW)
-   [Raylib](https://www.raylib.com/) installed and configured
-   The project source files: `main.c`, `sanjay.c` and `sanjay.h`

## Build and Run

### Linux

Compile from the project folder:

``` bash
gcc main.c -o fast_lost_found -lraylib -lm -lpthread -ldl -lrt -lX11
```

Run:

``` bash
./fast_lost_found
```

### Windows (MinGW)

Compile from the project folder, with Raylib installed and its library
paths configured:

``` bash
gcc main.c -o fast_lost_found.exe -lraylib -lopengl32 -lgdi32 -lwinmm
```

Run:

``` bash
fast_lost_found.exe
```

> `main.c` includes `sanjay.c`, so compile `main.c` only. Keep
> `sanjay.c` and `sanjay.h` in the project folder.

## Data Storage

The application creates a `data/` folder and stores item reports and
chat messages in local text files. Keep this folder with the application
if you want to retain your saved data.

## Note

This is a local desktop prototype. The included student profiles are
demo profiles; it is not connected to a live university database or
online service.
