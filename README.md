# Introduction
Pose Camera turns your phone into a realtime motion capture device, streaming animation data with low-latency to other applications.  This can be used as a motion based controller for gaming or fitness apps, or even to capture and record animations.  All processing is performed on the phone itself - no streaming videos to servers, no tying up desktop or gaming console resources.  Instead the phone uses state-of-the-art AI to identify human poses seen by the camera, and streams smooth skeletal animation data to your paired application.  The app can be configured for a variety of skeletal animation rigs - currently including the UE4 Mannequin, the UE4 MetaHuman or Mixamo configurations.  Visit us on [www.pose-ai.com](https://www.pose-ai.com) to learn more

This repository includes tools for interfacing with Pose Camera.  

# Unreal Engine plugin
We provide an Unreal Engine LiveLink plugin for connecting with the app.  We are currently working to make this available on the Unreal Marketplace.  The beta version can be downloaded from this repository.  The plugin can connect over both IPv4 and IPv6 local networks.

# Python demo
We provde a simple python example server which connects to the app and just prints the pose data to console.  This can be used as a basis for incorporating Pose Camera in a variety of applications.  The API is straightforward - the app looks for a UDP or TCP server on the IP address / Port number provided in Settings (or uses Bonjour/Zeroconf to scan the local network for bonjour enabled services).  The app expects a "handshake" message in JSON format, which sets the desired configuration, and then begins streaming skeletal pose estimates live at low latency.  The python example demonstrates the various options and format for the handshake.  Skeletal rotations are streamed by the app in JSON format, quaternion form (scalar last, left-hand rule).

# Latency and Unreal LiveLink
Pose Camera can operate with very low latency, particularly on faster phones.  Note that the speed of the machine running LiveLink can have a significant impact on the latency of the animation.  On a "gaming" desktop, Unreal is able to process the incoming animations almost instantly, with little lag between real world action and on-screen movement. When tested using the same phone but instead paired with a laptop, without a dedicated GPU, significant lag was introduced, several hundred milliseconds.

# Frames per second, heating and phone performance
Better embedded chip hardware in newer phones can significantly improve the performance.  Also, iPhones throttle the speed when the battery is worn or the phone runs hot.  
- An iPhone 12 should routinely be able to process 60 camera frames per second into motion capture data for sustained periods of time. 
- An iPhone 8 or X should be able to process 30 camera frames per second for sustained periods of time.  For shorter times these phones can process approximately 50 out of the camera's 60 frames per second - however, the high computation load will likely cause the phone to heat up after a few minutes and the operating system is likely to throttle the speed of the app at some point to maintain a cooler temperature. 
- While we do not officially support older phones, we have achieved 30 frames per second on a test iPhone 7, although some testers have reported their old phones struggled with the app.

Depending on use case, more camera frames is not always better.  Aiming for a 30 frame per second rate, rather than 60, will save considerable computation and reduce battery drain and phone heating, and animations are often pruned to keyframes for memory efficiency.  This can be configured through the LiveLink or python API.

If hand animations are unnecessary, use the body only modes to reduce computations (can be selected in LiveLink or configured in the Python handshake).

The app can stream at a user specified speed, interpolating the frames to deliver a smooth performance.  For example, the user can capture at a 30 fps camera speed but stream interpolated motion capture data at 60 fps.  However, higher streaming rates more heavily utilize the wireless chip on the phone.  If interpolation is not necessary or is being handled on the paired application, consider reducing the stream rate to save battery and heating.

On older phones try turning off the camera preview once propery positioned (tap the yellow person icon at the edge of the screen). This may help reduce the strain on the phone.


# Troubleshooting connections
1. Make sure you allow the iPhone app to access the local network upon request. If you refused the first time, you may need to change this in settings.
2. As your phone needs to be allowed to speak to the demo application, make sure your firewalls (both desktop and potentially router) does not block the port (typically 8080 but can be set from the LiveLink menu or in the python code).
3. Make sure your phone is on the same local network as the connecting computer (i.e. the one running Unreal Engine).
4. Make sure you have entered the correct local IP address into the mobile app for the connecting machine (often in the form of 192.xxx.x.xxx using IPv4).  For convenience the UE4 PoseAI plugin will show a local IP address in the LiveLink console and the python demo prints it to console.   However, if you have multiple adapters this may not be your computer's hosting address so you may need to check network settings.
5. On an IPv6 network try using the local-link address (starting with FE80: and, at least on our network, stopping before the % sign in the address)
