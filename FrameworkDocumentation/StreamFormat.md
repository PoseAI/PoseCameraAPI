# Streaming format
Pose Camera streams skeletal rotations in JSON format. We stream currently in two possible compression formats using the "packet format" field in the handshake and set to 0 and 1.
0 - Verbose setting.  This includes lengthy names for each joint and the quaternions packed as 4 floats (scalar last).  This is human readable but is a large packet, exceeds a single ethernet frame and uses up bandwidth.  We suggest only using this format for debugging and understanding the JSON structure.
1 - compact setting.  This sends the joint rotations as an array using a base64 fixed precision encoding.  This keeps the packet well beneath a single ethernet frame.  We have decoding routines in the UE and Unity code which can serve as examples on how to decode the data.
We may add a non-JSON pure binary packet for further efficiency in the future, although currently the compact setting works well for most use-cases.


## Summary of fields
This can best be seen inspecting the output by using the python example and most fields are self explanatory.
Below we summarise the output structure.  Note that the exact components will depend on which mode the Camera is using - obviously the hand fields will not be included if the camera is in body mode only.  And over time we plan on extending the supplementary information provided, i.e. beyond joint rotations.


NOTE: OUT OF DATE,   NEEDS UPDATE

### Top level JSON
At the top level the JSON record has four fields:
{
 'Body':{},
 'Rig':<string name of rig format such as UE4 or Mixamo>,
 'LeftHand':{},
 'RightHand':{}
}

### The 'Body' field
The 'Body' Field can include:
{
 'Rotations':{<joint name 0 string>:[x, y, z, w], <joint name 1 string>:[x, y, z, w], ...},
 'Scalars':{},
 'Vectors':{}
}
The 'Rotations' field and data is only included if the body is visible/detected.
The 'Scalars' field are all <string>:<number> pairs with extra information, most notably 'VisTorso', 'VisArmL' etc, which will have a value of 1.0 if that body component is detected and 0.0 otherwise.  This corresponds to the in-app icon showing body part detections and can be used to alert the application that the person is all or partially undetected.
The 'Vectors' field has <string>:<number array> pairs with extra information, such as where in the camera 2d field of view the subject's hips and chest are positioned.  

### The hand fields
The 'LeftHand' and 'RightHand' fields includes:
{
 'Rotations':{<joint name 0 string>:[x, y, z, w], <joint name 1 string>:[x, y, z, w], ...},
 'Vectors':{}
}
The hand rotations will also include the forearm twist rotation appropriate to the rig.
The 'Vectors' field currently only has the 'PointScreen' field which indicates the index finger's position in the camera 2D field of view, for use in say virtual "touchscreen" interaction.

### Rig format
Specifying the type of rig changes the joint names to match the rig.  ie "Hip" vs "Pelvis" or "LeftArm" vs "lowerarm_01_l".  It also rebases each joint's rotation from the neutral position for that rig (different rigs have bones pointing in different directions in neutral ).

### Quaternion format
The quaternion are packed in scalar last form (x, y, z, w) and may need to be re-normalized due to decimal truncation.  By default they are in left-hand rule basis as this is used in the Unreal Engine.  Experimental: pose camera can also be set to transmit in right-hand rule basis via the handshake (specify 'coord':'RHR").

### Locomotion and Events
Depending on the poseai engine settings, additional fields may be sent on action recognition and locomotion events (step, hop, jump, etc). 
