Animations must be configured to the mesh's skeletal rig in order to get the correct results.  This requires identical names and base rotations for each joint.  Unfortunaly, different rigged mesh providers and creators often use different naming conventions and different base orientations for their rigs.

##Standard rigs
We provide several "standard" configurations within the framework: "UE4" and "Metahuman" which is configured to the Unreal Engine Mannequin and MetaHuman rigs, "Unity" which is configured to the Unity demo robot and "Mixamo" which is configured to many of their characters (their older characters often have different rigs.  If it downloads as CH## it is probably ok).  Ideally your art team can provide rigged characters already compliant with one of these standards.  

In Unity or Unreal there is an option in the editor or handshake to set our Framework to stream in the format of any of these rigs.
<br><br>
##Custom rigs
If your rigs use a different orientation from standard, you can still use them with more setup work.  Your art team could reorient the rigs in their graphics software (like Maya, Autodesk or Blender).  Or you can use our remapping tool to calculate the remapping from one of the standard rigs to your custom rig.

In Unity, the PoseAIRigTarget.cs class holds remappings between rigs for runtime retargetting of the PoseAI animation controller to rigs with arbitrary joint rotation configurations.  These remappings are held in a dictionary in the script.  You can add your own remappings by cutting and pasting the results of the remapping tool into the script dictionary.  You can than specify this remapping in the PoseAIAnimationControler.  

### Creating a retargeting map
To use the tool we provide to solve for remappings, follow these steps:
1. create a unity project with a standard model (such as the Unity robot or a newer mixamo rig)
2. [if necessary] select the standard model, select rig in inspector, set animation type to Humanoid and create an avatar from this model
3. import your custom model
4. select the custom model. select rig in inspector. animation type-Humanoid, create avatar from this model
5. drag both the standard and custom models into scene.  Have them face same way.
6. Leave both models' animator controllers set to none if they are in same position.  Otherwise use the Unity animator controllers to get them in the same position (i.e. the t-pose)  
7. add PoseAIRigRetarget component to the custom model.  set 'Other Animator' on this component to the reference standard model's animator 
8. Click Play. Click Stop
9. Go to the Unity Console and find the refmodel_to_newmodel mapping output (a string key with a List of Quaternions). copy and paste that as a new entry on a new line into the Remappings dictionary in PoseAIRigRetarget.cs  (we have an example Unity_to_Mixamo entry already).  Change the key a more sensible name (ie 'Unity_to_MyCustomRig').

### Using the remapping
1. Now you should be able to animate your custom character with our framework.  In PoseAIAnimationController, set Remapping to the name of the remapping you just created and Rig Type to the reference standard rig you used (i.e. Unity or Mixamo).  
12.  If the rootbone name is different than the reference rig (i.e. Pelvis instead of Hips), set 'Override RootName' to your custom rig's rootname.
