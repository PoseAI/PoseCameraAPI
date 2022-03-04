# Version revision log


## Pose Camera 1.1 and PoseAI Engine Framework 1.1
Released March 2022. [Note Pose Camera update is still going through Apple process. We expect public release during week of March 7th]

#### Updated AI and smoothing
We have added our Version 2 AI model for mobile.  This model should be more accurate in most situations than our V1 model
* better depth perception
* better tracking for faster moving subjects
* better recognition of on-floor movements.  This is still an area of development as our main use case is standing or seated (in chair/sofa) subject.
* excess forward lean corrected.  Version 1 often add the subject leaning forward more than natural, this version should address
* improved real-time smoothing.  We have revised our approach to real-time animation smoothing and the new algorithm should generate smoother more natural looking animations, particularly noticeable in the hip and shoulder motions.  The smoothing should not generally add any lag to the animations although a very quick, very sharp movement (i.e. snap kick) may get truncated, and may require reducing the smoothing factor.  
* NEW API CALL: the degree of smoothing can be customized by calling PoseSkeletonWrapper::SetBodySmoothing(alpha).   Setting smoothing to zero would remove all smoothing.  We suggest keeping the default of 0.6 and potentially reducing for situations where the player will be performing very fast movements.
* NEW API CALL: The framework and app will default to the V2 model going forward but V1 can still be used by calling SetModelVersion(1) on the PoseAIEngine.

#### Revised secondary parameters
* Previous versions provided inconsistent calculations for parameters such as chest yaw, stance yaw, screen hips and screen chest depending on the camera (front/back) and mirror mode (on/off).  In the revised version, these values should be the same for all permutations and reflect the position of the player or user relative to the camera.  This makes it easier to use these parameters for character control, such as turning or using the screen position to generate root motion.
* We also made the base rotation of the entire rig in the different modes consistent.
* Screen positions and body height are now scaled by the longest dimension - for a 16:9 view the x would range -1 to +1 and the y -9/16ths to +9/16ths. (Previously they were scaled to always be +/-1 for y).
* Fixed: avatar would jump when using screen position to adjust root movement and when using compressed packet format.  This was due to overflow and the scaling issue above.
* Hand zones calculation adjusted to more naturally trigger the different zones.  Corrected relative adjustment so zones should now be relative to the user's torso, compensatig if the user twists to face another direction.
* Flapping arm event should trigger more easily and consistently.  Event 50 is less likely however (double arm flap) as single arm flaps are likely to be triggered first.
* Jumping event has additional considtions which should reduce spurious jumps.  This is still work in progress and we plan a broad revision to event recognition in the future.




  
  _This may require adjustments to blueprints (UE4) or scripts if they were adapted to deal with a particular permutation of camera setting._
  
  

#### Other changes
* The engine will now use the Ultrawide Lens if available on device (for example on an iPhone 12 when using the back camera).  This will allow the subject to be considerably closer to the phone, reducing space requirements. 
* Increased camera resolution.  We use a higher initial resolution before downscaling, which should improve hand tracking when further from the camera, without a noticable performance impact.
* Unity: modified utility.cs script to better limit for loops, as interupted packets could potentially lead to an index overflow error
