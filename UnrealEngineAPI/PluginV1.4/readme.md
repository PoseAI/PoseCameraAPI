# Revised version 1.4 plugin for Unreal
This version includes a significant refactoring of the LiveLink source files. 
We believe these changes should create a more stable, easier-to-maintain plugin with safer memory management.

We plan to refactor the network code and json processing in the future.  
 
## "Breaking" change - Enums in Handshake
We have replaced the human-error prone string fields in our handshakes with enums. 
This will likely cause existing blueprints to show an error on compile. 
 
Right click on the node and select "refresh node" and the errors should clear.  Check the mode and rig fields in particular as they will be reset to the defaults post update!

Apologies for the disruption but this is better for most users in the medium to long term.

## New IK node
We have added a new animation blueprint node for better arm placement on arbitrary avatars, without requiring calibration with the real life user.
We are adding them to the demo project.  

To set the node up yourself, you need to specify the four joints (left/right upper arms, lowest spine joint and the wrist joint) from your rig and then plug in the left or right hand ik vector from our stream (with the supplementary data in live values).  

See our video on youtube or our updated example for a functional setup.  


## Sync FPS setting
Sync Fps sets the on-device engine to interpolate and stream at a steady rate, potentially different then the speed of the camera or erratic mobile OS processing might provide.  This comes at a cost: buffering of around 0.5-1.0 camera frames which introduces 15-30ms of motion capture lag.  

Setting the Sync FPS to zero should disable the interpolation, although this setting was not working as intended in app/engine versions prior to 1.3.

We would currently recommend setting Sync Fps to 0 on newer versions of the engine, to minimize lag. You can instead use Sync Fps if you find the camera device too erratic. 


## Integrated camera plugin
This version also adds functionality needed for our supplementary Windows media player plugin to use our engine in Window and in-engine with a webcam or video player.
 
Note: the integrated camera supplementary plugin and functionality is not on general release as of yet.


