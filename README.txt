# README 

## Features beyond the expectations
#### Part I: 
 Infinite terrain. While moving the camera with the keys W and S we can can never step out of the terrain. Instead of moving camera, the terrain is regenerated and gives the impression that the camera is moved.
#### Part II: 
Water/lakes in the terrain. We also implemented water reflection of the
	mountains and the sky as well as water distorsion and transparency. 
#### Part III: 
Jetpack - Move the camera up and down independent of the terrain height. Physically realistic movements of the camera. Objects don't accelerate instantaneously as they have mass. 

Also we've implemented blurred terrain: when the distance from the point of view is too big the mountains are blurred. 

## Key control of the terrain: 
* W,S - move the camera along the view direction 
* A,D - rotate the aim about the up direction (yaw)
* Q,E - pitch the camera up and down
* Y - turn on/off fps mode
* R,F - if not in fps mode, move the camera up/down ("jetpack")
* G  - enable bezier courve mode. 
While in bezier courve mode if W/S is pressed the camera increases/decreases velocity.
* X,Z - move the level of the sea up/down
	
## Work splitted among the group members: 

Natalija Gucevska: Sky box, Water reflection, Water distorsion, Move terrain instead camera, Bezier curves velocity 
Sébastien Chevalley: Infinite terrain, Blurred mountains and water with blending and alpha maps, Bezier curves, Perlin noise (with random seed generated CPU side)
Lucas Pires: Camera movement, First eye experience, Jetpack, Textures

We tried to split work as evenly as possible but often we worked together and helped each other to fix our tasks. 

## Ressources
### Textures

* http://www.textures.com/download/rockgrassy0032/9191
* http://www.textures.com/download/soilbeach0088/32634?q=sand&filter=all
* http://www.textures.com/download/snow0157/122083
* http://opengameart.org/content/cloudy-skyboxes