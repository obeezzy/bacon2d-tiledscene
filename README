# Bacon2D Tiled Scene

This is an extension of the Bacon2D game engine (created by Rodrigo Goncalves de Oliveira and Roger Felipe Zanoni da Silva) that supports level loading from TMX files. This feature was actually proposed for Bacon2D 2.0, but because of my impatience, I decided to try it out.

Currently, I am working on writing examples and improving the documentation, although I have used it for my personal game development and it works so far.

It is also important that I acknowledge that some of the code I used was obtained from a particular fork of Bacon2D called "bacon2d-tiledmap". (I can't remember the url.)

## Build
### 1.1) Dependencies
- [Qt 5] framework
- [Bacon2D] game engine
- [Box2D] QML plugin 
- [Tiled] map editor

### 1.2) Building
The dependencies are already in this repository, so all you have to do is build. (Update if necessary.)

From the terminal (or command prompt), run the following commands:

    $ cd path/to/Bacon2D/folder
	$ mkdir build
	& cd build
	$ qmake
	$ mingw32-make (or nmake or make, depending on your compiler)
	$ mingw-32-make install
	
## How to use
To use this version of Bacon2D is fairly simple. Just create a **TiledScene**  and set the source of the TMX file. To manipulate layers, append **TiledLayer** objects (in any order) to the **layers** property of the **TiledScene**. To manipulate map objects in a TMX map, append **TiledObject** objects (in any order) to the **objects** property of the **TiledLayer**. See the documentation for details and examples.

## Notes
- For now, the **TiledLayer** class ignores layer visibility and opacity flags set from QML. These properties must be modified from the TMX file.

## Workarounds
QMake cannot tell the difference between two classes with the same name in two different namespaces. This caused clashes for the **Layer** and **ImageLayer**, because Bacon2D and Tiled have those classes.
In order to resolve that, I had to append the prefix **_tiled_** to each of the cpp filenames of the libtiled library. So instead of **layer.cpp** and **imagelayer.cpp**, I used **tiled_layer.cpp** and **tiled_imagelayer.cpp** respectively.

## General notes
* This library makes use of the Qt framework, the Bacon2D game engine, the qml-box2d library and the libtiled library (all on GitHub).
* This library is limited to only orthogonal maps. Others have not been tested.
* You can use any property name for image layers, tile layers and object layers in your TMX file, with the exception of _x, y, width, height, rotation, opacity, visible,_ and _id_.

## Todo
* Support TMX layer visibility and opacity.
* Show error messages for tiled layers or objects that do not exist.
* Isometric maps
 
**Please report all bugs. 
Also, I am FAR FROM perfect. If you see anything that can be done better, please notify me.**

License
----

MIT

[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job. There is no need to format nicely because it shouldn't be seen. Thanks SO - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)

   [Qt 5]: <https://qt.gitorious.org/qt/qt5>
   [Bacon2D]: <https://github.com/Bacon2D/Bacon2D>
   [Box2D]: <https://github.com/qml-box2d/qml-box2d/>
   [Tiled]: <https://github.com/bjorn/tiled>