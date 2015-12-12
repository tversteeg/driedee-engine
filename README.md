rogueliek
=========
A Duke Nukem 3D-like game engine which will eventually result in a prototype for a possbily roguelike game.

The progress is visually displayed in [this](http://www.youtube.com/playlist?list=PLFxtA9Al8RErx_kSD_-9Hrk9dkJBvwYU8) playlist.

To install you need `libglew-dev libconfig8-dev libpng12-dev`

Pseudocode for renderring using sectors & edges (inspired by the Build Engine and @fabiensanglard's Chocolate Duke Nukem 3D):

	Scan the sector for bunches to possibly draw

	Create empty stack of linked-edges (bunches)
	Create stack of sectors with the first sector
	while sector stack is not empty:
		for each edge in sector:
			if not facing camera (potentially visible):
				Continue
			if wall:
				if there is a bunch in the stack which shares a vertex:
					Add edge to bunch
				else:
					Create new bunch with this edge
			if portal:
				Add neighbor sector to sector stack
		Pop sector from stack

	Sort bunches on distance from player and call draw functions

	while bunch-amount > 0:
		for all bunches defined by the bunch-amount except the first one:
			Compare two bunches by overlap in screen-space perspective
			if no overlap:
				Set other bunch as not closest
			else if this bunch is in front of the other:
				Set oldest bunch-closest flag in temporary buffer as not closest
				Set this one as the closest one

		for all other bunches defined by the bunch-amount:
			if bunch is already not the closest:
				Continue

			Compare two bunches by overlap in screen-space perspective
			if no overlap:
				Set other bunch as not closest
			else if this bunch is in front of the other:
				Set oldest bunch-closest flag in temporary buffer as not closest
				Set this one as the closest one
				Reset for loop (i = 0)

		Call draw function for the closest one
		Decrement bunch-amount
		Put the closest on top of the bunch stack so it won't be iterated over again

	Drawing the edges

	Create two arrays for all the highest and lowest drawn pixels for each x value of the screen
