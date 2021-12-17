# UnityAPI

We use the free Unity Third Person starter kit as a starting point, and add three custom components to the character to extend the typical Unity setup. 
In order to add this API to your own project, follow these steps:
1. Copy our scripts into your project scripts folder
2. Add a character to the Scene.  
3. Select the character in the Scene.
4. Click on Add Component in the Inspector.   
5. Add each of the components as described below, in order, first adding both base Unity components and then adding the PoseAI components.
6. Configure the settings in the inspector.
7. Play the project in Unity. launch Pose Camera on your phone and set it to connect to the computer's IP address and port. 
8. Begin streaming from the phone.  Step 2-3 meters away so you are visible in the phone preview.


### Required base components (from Unity 3D engine).
1. Animator.  This is the standard Unity Mecanim component and state machine, which controls the animation of the character.  
    Our component will override the animation when necessary and allow you to blend between normal kinematic animations and live motion capture animations.
    In our demo, we use the StarterAssetsThirdPerson controller that comes with the free starter kit provided by Unity.  Make sure to select this controller and enable "IK Pass" in all the layers.
        
2. Character Controller.  This is the standard controller for moving a character.  Our controller will take input from the camera and command this controller to actually move the character.


### PoseAISource (component) 

This is a basic network server script with two subclasses, PoseAISourceDirect and PoseAISourceShared.  The direct subclass is the main class for configuring a connection.  

Attach to a game object (i.e. a character) to listen for an incoming connection, and configure with the exposed properties (edit in the Unity Inspector).  You must use a unique port for each source.

You can also use the PoseAISourceShared node on a character and then link it to a direct source housed in a different object.  They will need to share the same rig type as all parameters are shared.

The important parameters are:
* RigType - this must correspond to your rig (i.e. Unity for the standard robot, Mixamo for one of their newer characters).  If the skeleton is warped, this is usually the problem.
* Mode - this sets the required camera orientation (i.e. portrait or landscape), and whether to perform motion capture on the hands and fingers.  If you don't need hands and fingers we strongly recommend using the body only modes as this will improve FPS from the phone and/or reduce processor usage.


## PoseAICharacterAnimator (component)

This script does the animation overrides for realtime motion capture of a rigged character, using the already attached PoseAISource. It also requires
1. the Animator component to be already added to the character,
2. the Animator's controller to have IK Pass enabled, as we use the IK functions to set the override rotations from the Pose Camera.
3. The character to be set up as a humanoid avatar.

The important parameters are:
* Use Upper Body Only:  This will set the component to only blend in from above the Hips.  This is a good setting when using the character controller for standard movement, triggered by events, as running can be handled by normal animations.
* Move Root Sideways:  This will move the root left or right depending on how the player moves in camera. This will not respect collisions, and should generally be disabled if you are using the character controller (lateral movement would be handled there).
* Rig Height: This is the approximate height of the rig in meters, and will be used to scale the lateral movement (if enabled)
* Joint Name Prefix:  If your rig bone structure has a prefix before each joint name (i.e. mixamorig9:Hips), then specify the prefix (i.e. mixamorig9:).  This allows the script to find the root.


## PoseAICharacterController (component)

This script acts as a character controller and is roughly based on the Unity starter kit third person controller.  However, instead of input from the keyboard or game controllers, all input comes from motion capture.  This is then used to trigger motion or events as if the player had pressed keys.  The input is then passed to the character controller and also can trigger the Mecanim animation state machine.

In our demo, characters can walk or run around, jump, freely rotate, strafe, fly up in their air, glide or choose to plummet.

Many parameters can be configured in the Inspector directly, and more advanced gameplay can be enabled by editing the script to take advantage of the additional information provided by the controller (see in-script comments for more info).



Key parameters:
* Turning controls - rotates the character direction by the player either leaning to one side, or twisting their upper body away from the camera, in both cases turning faster the further they lean/twist. These controls set how quickly the character rotates (scale), and provides a 'deadzone' where no rotation happens.  Set the scale to 0 to disable one or both of these controls.  Some deadzone is recommended or else the player may find it hard to keep the camera steady.
* Can Arm Pumps trigger walking - in our demo the player can move forward by jogging in place (feet detection) or by pumping their arm similar to a jogger.  Using both makes it easier on the player (just arms is less tiring!) and makes detection more likely if they do both.
* Use Analog Ground Speed - if disabled, the character can only use two fixed speeds, walk and run, based on how they move. If enabled, they move faster directly depending on their speed of input.
* WalkAtStepsPerSecond - if analog is disabled, the threshold of player speed to have the character walk.
* SprintAtStepsPerSecond/SprintAtDistancePerSecond - if analog is disabled, the player will sprint if either of these thresholds are passed.  Distance per second incorporates how high the player steps.
* ForwardSpeedScale - if analog is enabled, this scales the character's forward movement.  The faster and higher they step, the faster the character will run.
* Player Strafe Scale - we allow the player to move their character sideways by moving sideways in the camera. See our script for a discussion of how to do combine this with straffing animations if available (not included in Unity's Starter Kit).

* Player Flight - We enable character flight by having them flap their arms, or potentially do chest expansions at shoulder level.  They can hold their arms out to the side (i.e. T-position) to glide, drop their arms to plummet, and lean or turn to bank turns.  
* Wind resistance - slows the character's forward motion when flying.  Ignored while gliding.
* Fly auto glide time - flapping their arms automatically can give them this a short burst of glide time, even if their arms are not in glide position.  
* Flap height - how much height they gain for flapping both arms.
* Rocket up height - while on the ground, they can rocket up this height by lifting their hands from their sides with an overhead clap (like the upper half of a jumping jack).
* Gliding fall velocity - while gliding they descend at this speed
* Gliding fwd velocity - gliding generates this forward speed

* Cinemachine - set the camera to the Playerfollow Camera.  On the PlayerFollowCamera itself, have it follow the PlayerCameraRoot transform, and Look At one of the upper chest joints on the Skeletal rig itself. 
* Aerial target pitch - this smoothly angles the camera downward while airborn to better see the ground below.  Set to zero if you don't want the camera to behave this way.


- - - -
## Other files
 
#### PoseAIJson
The base classes for parsing the JSON messages and communicating with Pose Camera.
* Handshake.name - this sets the text that will appear in the app.  Change to the name of your application!
* Handshake.Config subclass - this can be used to tweak the sensitivity of the step, jump, arm and crouch detections.

* PoseAIRig.userName - the name set in app for the user (defaults to their iOS phone name).
* PoseAIRig.sessionUUID_ - this will be a unique identifier per session per user

* PoseAIRig.ModelLatency - time (in ms) between Pose Camera receiving the frame and sending the packet. Does not include network transmission time from phone to source.
* PoseAIRig.Timestamp - timestamp from Pose Camera.


#### PoseAIConfig.
Enums and a couple of constants.
 * STALE_TIME_IN_MS: normally once a connection is made to a source on a specific port, a second sender will not be allowed to connect with the same source.  If the current connection is silent for this “timeout”, a second sender will be allowed to take over the connection to that source.
#### PoseAIRigUnity, PoseAIRigUE4 and PoseAIRigMixamo
These all specify the bone names for specific rig formats, mainly used for verbose packets (useful for debugging).


#### PoseAIRigRetarget
This is a utility that allows you to remap from one rig setup to one of our provided standard rig.  This can help you deal with limbs using a different rotation basis in the model.
Follow the instructions in this file to generate a remapping in the editor for each unique rig.


#### ChangeLog
* 17Dec21 - Modified optional Touches field to accomodate multipoint, as framework now transmits a touch index with each touch event.  Added TouchState field to communicate current state of ten touches.  Replaced 2D vector with UITouch class to hold state and position data. 
