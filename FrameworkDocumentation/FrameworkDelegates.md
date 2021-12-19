# Introduction
PoseAI Engine framework for iOS includes delegates to help integrate the engine into licensee apps.  Below we describe each.  

## PostCameraDelegate
This delegate requires a single method which receives a UIImage* from the camera each frame in ABGR format a a callback.  
Use this delegate if you need to extract images for manipulation in your app. 
As this delegate stream involves a copy from the GPU buffer, use only when necessary.  For previewing the camera in a UIView, prefer using the camera preview layer as demonstrated in our demo app.

### Set on engine
Once you set up your delegate, you must set your delegate on the PoseAIEngine using the SetPostCameraDelegate method.  You can set the delegate to nil when you no longer need to extract images.

<br><br>
## PostCameraI420vDelegate
This delegate requires a single method which receives a callback of three c-style uint_8 flat arrays pointing to y, u and v channels for the camera in YCrCb420v format.  The method also receives the size of the Y array and the common size of each U and V array. 
The Y channel is at full resolution and the other two channels are at 1/2 width and 1/2 height.  The range of uint values is as per video rather than full-range specification.
Use this delegate if you need to connect a video stream to an api which requires this common video format. 
As this delegate stream involves a copy from the GPU buffer and use of CPU intensive conversion, use only when necessary.

### Set on engine
Once you create your delegate class, use the SetPostCameraI420vDelegate method on the PoseAIEngine to set the delegate.  You can set the delegate to nil when you no longer need to extract images.
When setting the delegate, you must specify additional parameters:
* Specify the width and height of the output stream.  The method will resize the camera image to the specified sizes and the array sizes will reflect the area.
* Decide if you want the conversion to also flip the image horizontally (i.e. to deal with 'mirroring' of the front camera).
* Set halfFps if you want the conversion to only callback every other frame.  This is useful if the pose camera is working at 60fps but you only intend to stream at 30fps, as it will save CPU cycles on the conversion routine.


<br><br>
## PostPoseDelegate
This delegate can be used to accept the engine's JSON formatted pose output.  Use this delegate if you are handling networking yourself instead of using the engine's built in network client.  You can take and parse the JSON yourself and send it as desired.

### Set on engine
Once you create your delegate, you must set the delegate on the PoseAIEngine using the SetPostPoseDelegate method.  By default, the PoseAIEngine is the delegate, and sends the pose camera output via the network settings on the engine.  If you set your own delegate, this engine will no longer transmit via the built-in client.