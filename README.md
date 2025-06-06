# Info
This is a simple 300 line code renderer ,which uses a ray tracing technique that computes a 2D picture of a 3D model with a default of bronze color.
High-end funcionality lacks as the idea is to purely showcase. 
To be more complex and easily accessed - an .obj file loading was added, using Blender models and exporting it with default (grey) material (thus rendering it bronze).
Colors could be changeed using RGB scheme. 

# How to use 
The photo gets outputted in a .ppm ,which can be opened with **IrfanView** or be converted to .png using:
````
sudo apt install imagemagick
convert output.ppm output.png
````

## To change .obj 
Go to line `289` and change the string. 

## To change camera position
Go to line `286` ,where the position is in xyz (to be changed manually).

## To change photo dimensions
Go to line `279`(width) and `280`(height). The lower - the faster.


>Since there is no GUI and this isn't a 3D modeling space, no 3D model of a camera is accessable to move intuively ,neither is there a color wheel for materials.   
