# Introduction
The Pose Camera allows users to use their phone as an input device to stream motion capture animation data in real time to other applications, for use as a motion based controller or even to record basic motion capture.  This repository will include demo programs highlighting how developers can interface with Pose Camera.

The API is straightforward - the app looks for a UDP or TCP server on the IP address / Port number provided in Settings (or uses Bonjour/Zeroconf to scan the local network for bonjour enabled services).  The app expects a "handshake" message in JSON format, which sets the desired configuration, and then begins streaming skeletal pose estimates live at low latency.  The python example demonstrates the various options and format for the handshake.

Skeletal rotations are streamed by the app in JSON format, quaternion form (scalar last, left-hand rule), and can be configured to either the UE4 Mannequin or Mixamo rig configuration (different rigs have different base joint orientations and joint names).

# Unreal Engine plugin
We provide an Unreal Engine LiveLink plugin for connecting with the app.  This will be available on the Unreal Marketplace or here.  This uses UDP communication.

# Python
A simple python server which connects to the app and just prints the pose data to console.   

# Swift 
TBD.  This will also demonstrate using Bonjour for auomatic pairing.

# Troubleshooting connections
1. Make sure you allow the iPhone app to access the local network upon request. If you refused the first time, you may need to change this in settings.
2. As your phone needs to be allowed to speak to the demo application, make sure your firewall does not block the port (typically 8080).  Both your desktop/laptop and your router firewalls may be configured to block ports by default. 
3. Make sure your phone is on the same local network as the connecting computer (i.e. running Unreal Engine).
4. Make sure you have entered the correct local IP address into the mobile app for the connecting machine (often in the form of 192.xxx.x.xxx).  For convenience the UE4 PoseAI plugin will show your local IP address in the LiveLink console and the python demo prints your IP address to console. 
