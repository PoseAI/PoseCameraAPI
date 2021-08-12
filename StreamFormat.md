# Streaming format
Pose Camera streams skeletal rotations in JSON format, and this is best seen by inspecting the output by using the python example.  Below wel summarise the output structure.  Note that the exact components will depend on which mode the Camera is using - obviously the hand fields will not be included if the camera is in body mode only.  And over time we plan on extending the supplementary information provided, i.e. beyond joint rotations.

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


