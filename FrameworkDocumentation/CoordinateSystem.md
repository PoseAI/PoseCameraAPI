# Coordinate system overview and mirroring

This note explains and clarifies our approach to coordinate system directions and the "mirroring" feature in our framework, and explains expected behaviour.
Game engines, modelling software and cameras use different direction schemes and coordinate systems.

Our engine should handle most use cases seamlessly, but for advanced use developers may need to understand the underlying basis.  As our tool is used on multiple platforms we have been working to develop a consistent approach that is intuitive to our customers and here we explain expected behavior. 

### Four reference frames
The crux of the complexity is there are four frames of reference (i.e. four definitions of 'left'): player left, avatar left, game camera left and on-device camera left.
The on-device camera is typically in front and facing the player, while the game-engine camera is typically behind the avatar.  The avatar may or may not be facing game-engine camera. 

### Assumed game orientation
We assume third person camera in game, with the character object in front of the camera and the mesh inside the character object facing forward (same direction camera is looking).

### Unreal Engine
While in the Unreal engine world space X is forward, Y is right and Z is up, in character space X is left and Y is forward - usually the avatar is rotated 270 degrees along Z axis in the character controller, accomodating this difference. Root motion would be applied in character space.

### Unity
In both character and world space, X is right, Y is up and Z is forward

### PoseAIEngine - Extra parameters
In our stream from the device to the engine or server, we provide extra parameters such as ScreenHips, ScreenChest and ScreenPoints (for the index finger of the hands) as 2D vectors.  
We define XY for these vectors from the perspective of a player directly facing their device (and presumably the tv screen or monitor).  So positive X is the player right, Y is up.

### Mirroring
We define mirroring as transforming the avatar to create the effect of looking into a mirror - when the player raises their left hand, the mirrored avatar raises their right hand, and when the players turns to its left the avatar turns to its right.  Our engine correctly swaps left/right limbs and the rotation stream also incprorates a 180 degree rotation to have the avatar face the game-engine camera.  This does not change the character controller's rotation.
As we only rotate the mesh, not the character controller, the same rules apply for lateral motion using ScreenHips.  This option can be selected

Note: If using upper body animations only, the developer may need to rotate the root themselves by 180 degrees to have the avatar face the game camera.

### Lateral Motion
The screenhips parameter is used in our plugin and demo project to add root or lateral motion to an avatar in free movement modes.  This screen value is converted to player height (based on estimated player height at current camera distance) and then scaled by the height of the avatar.
In Unity we add the delta change in the screenhips X for side-to-side movement.  In Unreal, we must subtract the delta change as character X is reversed. Mirror mode works the same: in all modes when the player slides to their left, the avatar should slide to the left side of the screen. 

