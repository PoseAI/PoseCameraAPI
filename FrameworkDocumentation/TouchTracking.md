The Framework includes an option to track touch gestures on the device and stream them to the server as part of the JSON packets.
This can be used to provide direct input to a paired application from an app using the Pose Engine.  In our Unity example, we provide sample code to convert this stream into touch objects and track.  Below we explain the streaming format and how to set up touches using our mobile Framework.

## Setup
To have touches sent to the app, use the RegisterViewForNetworkedTouches method on the PoseAIEngine and pass all of the UIViews from your app where touch data will be broadcast.
  
Set UseLocalViewCoord to YES if you want touch data to be relative to the view, and NO if you want touch data to be relative to the mobile device.

Enable or disable the broadcast of touch data using the BroadcastTouches method on the PoseAIEngine.  You can separately enable and disable the stream of events, "AllTouches", and the touch state fields (both explaind below).
Registering a view will automatically enable broadcasting of both fields, so use this method after registering any view.

In order to enable multitouch tracking, you must still enable multitouch for the view in the App code itself.  For example, in Obj-C something like:

[<the view object> setMultipleTouchEnabled:YES];
<br><br>
## Receiving touch data
We send the data in JSON format in two fields, one as an event array and one as a state array, plus an orientation field.
### Touches array
Our JSON format includes a field called 'Touches' with a variable length string value containing any touch Events recorded since the last packet.  Each individual touch is sent as a 5 character string.

'Touches' : '_ _ _ _ _ _ _ _ _ _' <br>
...................0 1 2 3 4 5 6 7 8 9.........

 * The first character is an index from 0-9 tracking the contact for multitouch input.  Each new touch contact will use the first available free index and release the index when the touch ends.
 * the second and third character encode the X coordinate in fixed precision float equivalent, with a range of -1.0 to +1.0, with 0.0 being the center of the screen.  
* the fourth and fifth character encode the Y coordinate.
* A value of '====' encodes the touch has ended.  A value of '=0=0' encodes the touch was cancelled.  Numerical values are encoded with fixed precision using base64 Please see our Unity example for a function to convert the two character encoding back to floating point.

The string can include a variable number of touches, one for each touch Event registered by the device since the last packet.  New events are triggered when a user first touches the screen, when the user moves their touch, when the user releases, or when the touch was cancelled (for example due to a phone call received).

A single finger can send multiple touches in one packet if it moves quickly, and they will be in order received.

Important: this is UI *Event* logic, not *polling*, so holding a finger in place will not keep sending events - another event for that touch index will only be sent when the finger is moved or released. 

<br><br>
### TouchState
A second field is called 'TouchState' which sends a 40-character value string.  Each four characters represents the current state of the touch for that multitouch index, i.e. the first four characters record the state of index 0, the next four characters for index 1.

'TouchState' : '_ _ _ _ _ _ _ _ _ _ _ _.......' <br>
.......................X 0 Y 0 X 1 Y 1 X 2 Y 2 .....

The format of the four characters is the same as above in Touches Array.  When no touches have been recorded, the value will be "====", i.e. the touch has ended.  Use our Unity example as a template on how to convert the string data into XY coordinates.

This is UI polling logic, and will only show the latest state.  Rapid movements would be dropped in favor of the final position.  However this may be a safer field to use if dropped packets are a concern.  

<br><br>
### Orientation
The XY coordinates of a touch are specified by iOS in the device frame of reference.  This means the upper left corner from the user's perspective changes XY as the phone rotates.  This can be corrected, if desired, by taking into account the orientation field.
We send the current orientation of the phone.  For iOS this is a UIDeviceOrientation enum.

