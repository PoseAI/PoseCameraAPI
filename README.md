# Introduction
Pose Camera turns your phone into a realtime motion capture device, streaming animation data with low-latency to other applications.  This can be used as a motion based controller for gaming or fitness apps, or even to capture and record animations.  All processing is performed on the phone itself - no streaming videos to servers, no tying up desktop or gaming console resources.  Instead the phone uses state-of-the-art AI to identify human poses seen by the camera, and streams smooth skeletal animation data to your paired application.  The app can be configured for a variety of skeletal animation rigs - currently including the UE4 Mannequin, the UE4 MetaHuman, Mixamo of Daz(UE import) configurations.  Visit us on [www.pose-ai.com](https://www.pose-ai.com) to learn more

This repository includes tools for interfacing with Pose Camera. 

For the latest information, FAQ, Troubleshooting and performance tips, please see the documentation sections on our [website](https://www.pose-ai.com)


### Unreal Engine plugin
This is now available (free) on the Unreal Marketplace.  Check there for compiled binaries and the latest updates.

### Unity scripts
We currently host scripts and examples for Unity here on Github, and we will add them to the Unity marketplace soon.


### Python demo
We provde a simple python example server which connects to the app and just prints the pose data to console.  This can be used as a basis for incorporating Pose Camera in a variety of applications.  The API is straightforward - the app looks for a UDP or TCP server on the IP address / Port number provided in Settings (or uses Bonjour/Zeroconf to scan the local network for bonjour enabled services).  The app expects a "handshake" message in JSON format, which sets the desired configuration, and then begins streaming skeletal pose estimates live at low latency.  The python example demonstrates the various options and format for the handshake.  Skeletal rotations are streamed by the app in JSON format, quaternion form (scalar last, left-hand rule).  We describe this in more detail: [here](StreamFormat.md)


### Documentation
We also host a version log and some more detailed technical information for developers in our FrameworkDocumentation folder
