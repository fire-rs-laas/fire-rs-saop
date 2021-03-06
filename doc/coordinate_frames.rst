Fire-RS coordinate frame description
====================================

## UAV body frame

ENU
	x: from the tail to the head
	y: parallel to the wings
	z: pointing to the sky

Rotation order 1: z, 2: y, 3: x

	rot z: Yaw, heading (Psi)
	rot y: Pitch (phi)
	rot x: Roll (theta)

## Projected coordinate system

EPSG:2154 - Lambert 93

	x: pointing to the east
	y: pointing to the north
	z: undefined (pointing up to follow the right-hand rule)
	angles: counter-clock-wise

	 ↑ north, y
	 |
	 |  ENU
	 |
	0⭯―――→ east, x
	 0
	 
Same as: Dubins 2d library coordinate system

## Dubins 3D

ENU like the main projected coordinate system.

Spirals in polar coordinates:

Radius:

Lambda: direction of rotation. +1: To the left, -1: To the right

Chi: Angle for the polar coordinate that determines the starting position / portion of the circle?. This is perpendicular to the heading angle of the UAV at this position.

## Raster files

	x: east 
	y: ¿?
	z: ¿?
	angles: ¿?

	↑ ¿?
	|
	|
	|
	?―――→ ¿?

## Geodata coordinate system

	x: ¿?
	y: ¿?
	z: 
	angles: counter-clock-wise

	↑ north, y
	|
	|
	|
	⭯―――→ east, x
