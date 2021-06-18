# Introduction
The Pose Camera allows users to use their phone as an input device to stream motion capture animation data in real time to other applications, for use as a motion based controller or even to record basic motion capture.  This repository will include demo programs highlighting how developers can interface with Pose Camera.

The API is straightforward - the app looks for a UDP or TCP server on the IP address / Port number provided in Settings (or uses Bonjour/Zeroconf to scan the local network for bonjour enabled services).  The app expects a "handshake" message in JSON format, which sets the desired configuration, and then begins streaming skeletal pose estimates live at low latency.  The python example demonstrates the various options and format for the handshake.

Skeletal rotations are streamed by the app in JSON format, quaternion form (scalar last, left-hand rule), and can be configured to either the UE4 Mannequin or Mixamo rig configuration (different rigs have different base joint orientations and joint names).

# Unreal Engine plugin
We provide an Unreal Engine LiveLink plugin for connecting with the app.  This will be available on the Unreal Marketplace or here.  This uses UDP communication and can connect over both IPv4 and IPv6 local networks.

# Python demo
We provde a simple python example server which connects to the app and just prints the pose data to console.  This can be used as a basis for incorporating Pose Camera in a variety of applications.

# Troubleshooting connections
1. Make sure you allow the iPhone app to access the local network upon request. If you refused the first time, you may need to change this in settings.
2. As your phone needs to be allowed to speak to the demo application, make sure your firewall does not block the port (typically 8080).  Both your desktop/laptop and your router firewalls may be configured to block ports by default. 
3. Make sure your phone is on the same local network as the connecting computer (i.e. the one running Unreal Engine).
4. Make sure you have entered the correct local IP address into the mobile app for the connecting machine (often in the form of 192.xxx.x.xxx using IPv4).  For convenience the UE4 PoseAI plugin will show your local IP address in the LiveLink console and the python demo prints your IP address to console. 
5. On an IPv6 network try using the local-link address (starting with FE80: and, at least on our network, stopping before the % sign in the address)

# Frames per second, heating and performance
Newer phones: An iPhone 12 should routinely be able to process 60 camera frames per second into motion capture data. 
Moderately aged phone: An iPhone X or iPhone 8 should be able to achieve approximately 50 frames per second for some period of time.  However, the high computation load will likely cause the phone to heat up after a few minutes and the operating system is likely to throttle the speed of the app at some point to maintain a cooler temperature. 
Older phones: while we do not officially support older phones, we have achieved 30 frames per second on a test iPhone 7, although some testers have reported their old phones struggled with the app.

Depending on use case, more frames is not always better.  Aiming for a 30 frame per second rate, rather than 60, will save considerable computation and reduce battery drain and phone heating.  
Using the body only models (i.e. no hands) will also likely improve performance.
The app can stream at a user specified speed, interpolating the frames to deliver a smooth performance.  For example, the user can capture at a 30 fps camera speed but stream interpolated motion capture data at 60 fps.  However, higher streaming rates more heavily utilize the wireless chip on the phone.  If interpolation is not necessary or is being handled on the paired application, consider reducing the stream rate to save battery and heating.
