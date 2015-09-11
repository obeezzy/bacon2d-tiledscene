# bacon2d-tiledscene
Extension for Bacon2D that supports level loading from .TMX files

This is an extension of the Bacon2D game engine (created by Rodrigo Goncalves de Oliveira and Roger Felipe Zanoni da Silva) that supports level loading from .tmx files. This is actually a proposed feature for Bacon2D 2.0, but because of my impatience, I decided to try it out.
Currently, I am working on generating documentation and examples for this project, although I have used it for my personal game development.
It is also important that I acknowledge that some of the code I used was obtained from a particular fork of Bacon2D called "bacon2d-tiledmap". (I can't remember the url.)

=====
BUILD
=====
1.1) Dependencies
* Qt5
  	Qt5 source repository: https://qt.gitorious.org/qt/qt5
* Bacon2D
	Bacon2D source repository: https://github.com/Bacon2D/Bacon2D
* Box2D QML Plugin
	Box2D QML plugin source repository: https://github.com/qml-box2d/qml-box2d/
* Tiled
	Tiled source repository: https://github.com/bjorn/tiled

1.2) Building
To build this repository, complete the following steps:
1. Download, build and install Qt5 (www.qt.io/download/).
2. Download Bacon2D. I will refer to this folder as "Bacon2D" henceforth.
3. Download Box2D QML Plugin. I will refer to this folder as "Box2D" henceforth.
4. Download Tiled. I will refer to this folder as "Tiled" henceforth.
5. Download this repository. I will refer to it as "TiledBacon".
6. Go to the "Bacon2D/3rdparty" directory.
7. Copy "Box2D" to "Bacon2D/3rdparty" directory.
8. Go to "Tiled/src/libtiled" and open the "libtiled.pri" file. Append the following flag to the end of the file:
	DEFINES += TILED_LIBRARY
9. Go to "Tiled/src/libtiled". Rename "layer.cpp" and "imagelayer.cpp" as "tiled_layer.cpp" and "tiled_imagelayer.cpp" respectively. (The reason is stated below.)
10. Go to "Tiled/src" and copy "libtiled" to "Bacon2D/3rdparty".
11. Go to "TiledBacon" and copy the "tmx" folder to "Bacon2D/src".
12. Replace the "src.pro" file in "Bacon2D/src" with the "src.pro" file in "TiledBacon".
13. Go to "Bacon2D/src" and replace the "plugins.cpp" file with "plugins.cpp" file in "TiledBacon/src".
14. Open Command Prompt.
15. Enter the following command:
	$ cd path/to/TiledBacon/folder
	$ mkdir build
	& cd build
	$ qmake
	$ mingw32-make (or nmake or make, depending on your compiler)
	$ mingw-32-make install
16. If the compilation was successful, you should be able to run one of the examples in "Bacon2D/examples".

===========
HOW TO USE
===========
To use this version of Bacon2D is fairly simple. Just create a TiledScene QML object and set the source of the .tmx file. To manipulate layers, append TiledLayer objects (in any order) to the "layers" list property of the TiledScene. To manipulate map objects in a TMX map, append TiledObject objects (in any order) to the "objects" list property of the TiledLayer.

==========
COLLISIONS
==========
For this library extension, I use the word "collision" to refer to two or more map objects in a TMX map that have the same name and type. This is useful when you want to treat each map object in the same manner.

=============
DOCUMENTATION
=============

TiledScene Properties
* url source: The source of the .tmx file.
* list<TiledLayer> layers: List of layers you would like to make use of. You do not have to include all the layers that are present in the .tmx file, only the ones you want to manipulate.

TiledLayer Properties
* string name: The name of the layer.
* list<TiledObject> objects: The objects in this layer you 	would like to make use of. You do not have to include all 	the layers that are present in the .tmx file, only the ones 	you want to manipulate.

TiledLayer Methods
* string getProperty(name): Used to get property of layer with property name "name".

TiledObject Properties
* string name: Name of the object.
* string type: Type of the object.
* Body body [readonly]: Box2D body used for this object (if any).
* list<Fixture> fixtures: Fixtures used to create this body.
* int count: Number of collisions.
* int index: The current position of collision. Can also be set to a new position.

TiledObject Methods
bool next(): Next object in collision list.
bool previous(): Previous object in collision list.
void reset():  Reset collision list current index to -1 (a position before the first object).
string getProperty(name): Used to get property of object with property name "name".

================================
NOTES CONCERNING THIS REPOSITORY
================================
1. Objects in an object layer are allowed to have the same name. You can use the "next" method or the "previous" method of TiledObject to access the various objects.
2. This library was built to only create static bodies. Kinematic bodies and dynamic bodies should be created from Bacon2D, although you can use the .tmx file to set their positions and access them from the TiledObject class.
3. For now, the library ignores layer visibility and opacity flags set from QML. If you don't want a layer to appear, set it from the .tmx file.

===========
WORKAROUNDS
===========
QMake cannot tell the difference between two classes with the same name in two different namespaces. In the case of Bacon2D and libtiled, this would cause clashes with "Layer" and "ImageLayer", because both are defined in both libraries.
In order to resolve that, I had to append "tiled_" to each cpp filename. So instead of layer.cpp and imagelayer.cpp for the libtiled class, it is tiled_layer.cpp and tiled_imagelayer.cpp respectively. This was the only way I could resolve the ANNOYING problem.

=============
GENERAL NOTES
=============
* This library makes use of the Qt framework, the Bacon2D game engine, the qml-box2d library and the libtiled library (all on GitHub).
* This library is limited to only orthogonal maps. Others have not been tested.

====
TODO
====
* Support layer visibility and opacity.
* Show error messages for tiled layers or objects that do not exist.
* Isometric maps
 
Please report all bugs. 
Also, I am FAR FROM perfect. If you see anything that can be done better, please notify me.
IF YOU HAVE ANY QUESTIONS, PLEASE EMAIL ME AT efeoghene.obebeduo@gmail.com.
