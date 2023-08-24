// Copyright 2021-2023 Pose AI Ltd. All rights reserved

using UnityEngine;
using System.Collections.Generic;
using System.Reflection;
using System.Diagnostics;
using System.Linq;
using System;


namespace PoseAI
{

    public static class PoseAIRigFactory
    {
        public static PoseAIRigBase SelectRig(PoseAI_Rigs rigType) => rigType switch
        {
            PoseAI_Rigs.Unity => new PoseAIRigUnity(),
            PoseAI_Rigs.UE4 => new PoseAIRigUE4(),
            PoseAI_Rigs.Mixamo => new PoseAIRigMixamo(),
            PoseAI_Rigs.MixamoAlt => new PoseAIRigMixamo(),
            PoseAI_Rigs.MetaHuman => new PoseAIRigMetaHuman(),
            _ => new PoseAIRigUnity()
        };
    }

    [System.Serializable]
    public class PoseAIHandshake
    {
        [System.Serializable]
        public class Handshake
        {
            //name displayed in the Pose Camera app upon connection
            public string name = "Unity Demo";
            public string rig;
            public string mode;
            public string mirror;
            public string face;
            public int syncFPS;
            public int cameraFPS;

            // name of app and signature for licensee connections
            public string whoami = "Your App Name";
            public string signature = "Your App Signature";

            // 0 for verbose JSON, 1 for more compact packets.
#if VERBOSE_PACKET
            public int packetFormat = 0;
#else
            public int packetFormat = 1;
#endif
            //this is the version of our AI.  version 3 is current as of June 2023.  
            public int modelVersion = 3;
        }
        public Handshake HANDSHAKE = new Handshake();

        [System.Serializable]
        public class Config
        {
            // sensitivities for the event triggers.  0.0 is lowest sensitivity (more false negatives), 1.0 is highest (more false positivies)
            public float stepSensitivity = 0.75f;
            public float jumpSensitivity = 0.25f;
            public float armSensitivity = 0.5f;
            public float crouchSensitivity = 0.5f;
        }
        public Config CONFIG = new Config();

        //public double echoServerTimestamp = <your timestamp goes here>;

        public static PoseAIHandshake Factory(PoseAI_Modes mode, PoseAI_Rigs rig, bool mirror, bool face, int syncFPS, int cameraFPS)
        {
            PoseAIHandshake handshake = new PoseAIHandshake();
            handshake.HANDSHAKE.rig = rig.ToString();
            handshake.HANDSHAKE.mode = mode==PoseAI_Modes.SeatedAtDesk ? "Desktop" : mode.ToString(); //in process of migrating name
            handshake.HANDSHAKE.mirror = mirror ? "YES" : "NO";
            handshake.HANDSHAKE.face = face ? "YES" : "NO";
            handshake.HANDSHAKE.cameraFPS = Mathf.Max(cameraFPS, PoseAIConfig.MIN_CAMERA_FPS);
            handshake.HANDSHAKE.syncFPS = 0; //syncFPS is being depreciated. setting to zero to ensure all versions of engine don't use 
            return handshake;
        }
    }

    [System.Serializable]
    public abstract class PoseAIRigBase
    {
        // possible fields from incoming JSON stream
        public string userName;
        public string version;
        public string deviceName;
        public string sessionUUID; //this is cleared and stored privately to help track if a new handshake was sent
        private string sessionUUID_;

        public int ModelLatency = 0;
        public double Timestamp;

        // skeleton joint naming and base rotation, such as UE4, Mixamo etc.
        public string Rig;
        // packet format.  0 is verbose, 1 is compressed
        public int PF;

        // face blendshapes if available.  This will only work if PF=1  (compressed).  For Verbose need to change this to a List. May to resolve in future release
#if VERBOSE_PACKET
        public List<float> Face = new(52);
#else
        public string Face = "";
#endif


        public List<float> blendshapes = new List<float>(new float[52]);

        //receives compact touch updates from app, and adds them as vector 2s to Queue for game logic processing.  Values are relative to phone not user (use orientation to rotate as appropriate)
        public string TouchState = "";
        public string Touches = "";
        public Queue<UITouch> touchQueue = new Queue<UITouch>();
        public List<UITouch> touchStates = UITouch.MakeList(10);
        //if touches are enabled, sends current device orientation. For iOS this is the UIDeviceOrientation enum
        public int Orientation;

        public VisibilityFlags visibility = new VisibilityFlags();

        // extra rotation applied to hip/pelvis.
        public Quaternion baseRotation;

        // whether app is in desktop mode (Excluding legs)
        public bool isDesktop = false;
        //number of joints to skip in desktop mode
        public int lowerBodyNumOfJoints = 8;

        // this is the order the PoseCamera sends the joints.
        private static readonly List<HumanBodyBones> bones = new List<HumanBodyBones>{
            HumanBodyBones.Hips,
            HumanBodyBones.RightUpperLeg,
            HumanBodyBones.RightLowerLeg,
            HumanBodyBones.RightFoot,
            HumanBodyBones.RightToes,
            HumanBodyBones.LeftUpperLeg,
            HumanBodyBones.LeftLowerLeg,
            HumanBodyBones.LeftFoot,
            HumanBodyBones.LeftToes,
            HumanBodyBones.Spine,
            HumanBodyBones.Chest,
            HumanBodyBones.UpperChest,
            HumanBodyBones.Neck,
            HumanBodyBones.Head,
            HumanBodyBones.LeftShoulder,
            HumanBodyBones.LeftUpperArm,
            HumanBodyBones.LeftLowerArm,
            HumanBodyBones.RightShoulder,
            HumanBodyBones.RightUpperArm,
            HumanBodyBones.RightLowerArm,
            HumanBodyBones.LeftHand,
            HumanBodyBones.LastBone, //forearm twist not in unity avatar system, using LastBone as a Null value
            HumanBodyBones.LeftIndexProximal,
            HumanBodyBones.LeftIndexIntermediate,
            HumanBodyBones.LeftIndexDistal,
            HumanBodyBones.LeftMiddleProximal,
            HumanBodyBones.LeftMiddleIntermediate,
            HumanBodyBones.LeftMiddleDistal,
            HumanBodyBones.LeftRingProximal,
            HumanBodyBones.LeftRingIntermediate,
            HumanBodyBones.LeftRingDistal,
            HumanBodyBones.LeftLittleProximal,
            HumanBodyBones.LeftLittleIntermediate,
            HumanBodyBones.LeftLittleDistal,
            HumanBodyBones.LeftThumbProximal,
            HumanBodyBones.LeftThumbIntermediate,
            HumanBodyBones.LeftThumbDistal,
            HumanBodyBones.RightHand,
            HumanBodyBones.LastBone,  //forearm twist not in unity avatar system,  using LastBone as a Null value
            HumanBodyBones.RightIndexProximal,
            HumanBodyBones.RightIndexIntermediate,
            HumanBodyBones.RightIndexDistal,
            HumanBodyBones.RightMiddleProximal,
            HumanBodyBones.RightMiddleIntermediate,
            HumanBodyBones.RightMiddleDistal,
            HumanBodyBones.RightRingProximal,
            HumanBodyBones.RightRingIntermediate,
            HumanBodyBones.RightRingDistal,
            HumanBodyBones.RightLittleProximal,
            HumanBodyBones.RightLittleIntermediate,
            HumanBodyBones.RightLittleDistal,
            HumanBodyBones.RightThumbProximal,
            HumanBodyBones.RightThumbIntermediate,
            HumanBodyBones.RightThumbDistal
        };
        public virtual List<HumanBodyBones> GetBones() {
            return bones;
        }
        public virtual Dictionary<int, string> GetExtraBones() { return new() { }; }


        public static readonly List<int> genericParentIndices = new() {
        0,0,1,2,3,0,5,6,7,0,9,10,11,12,11,14,15,11,17,18,16,16,20,22,23,20,25,26,20,28,29,20,31,32,20,34,35,19,19,37,39,40,37,42,43,37,45,46,37,48,49,37,51,52,
        };

        public virtual List<int> GetParentIndices()
        {
            return genericParentIndices;
        }

        protected internal List<List<float>> rotationData;
        protected internal List<string> rotationNames;
        protected internal int numOfLeftHandJoints;
        protected internal int numOfRightHandJoints;
        protected internal int numOfBodyJoints;

        private Stopwatch stopwatch = Stopwatch.StartNew();
        private bool newHandshake = false;

        // baseRotationIn: extra rotation applied to hip/pelvis.  This will depend on how the rig was setup in modeling package and how it was imported from FBX.  You may need to change the values used in the specific rig subclasses.
        public PoseAIRigBase(Quaternion baseRotationIn)
        {
            baseRotation = baseRotationIn;
            baseRotation.Normalize();
            rotationData = new List<List<float>>();
            rotationNames = new List<string>();
            BuildListsFromIntrospection();
        }


        public bool OverwriteFromJSON(string jsonString)
        {
            // if the port is exposed as a public server, expect to get lots of false connections (i.e. port sniffers or malicious actors).  unity doesn't have a json verifier so will use a try/catch.
            try
            {
                JsonUtility.FromJsonOverwrite(jsonString, this);
            } catch (Exception ex)
            {
                return false;
            }

            if (!string.IsNullOrEmpty(sessionUUID))
            {
                sessionUUID_ = sessionUUID;
                sessionUUID = null;
                newHandshake = true;
            }
            else
            {
                newHandshake = false;
                stopwatch.Restart();
            }
            if (PF == 1)
            {
                GetBody().Scalars.ProcessCompact(ref GetBody().ScaA);
                GetBody().Vectors.ProcessCompact(ref GetBody().VecA);
                GetBody().Events.ProcessCompact(ref GetBody().EveA);
                visibility.ProcessCompact(ref GetBody().VisA);
                GetLeftHand().ProcessCompact();
                GetRightHand().ProcessCompact();
                

            }
            else
            {
                visibility.ProcessVerbose(ref GetBody().Scalars);
            }
            ProcessFace();
            ProcessTouches();
            return true;
        }

        public Quaternion GetBaseRotation()
        {
            return baseRotation;
        }

        public bool HasRigInfo()
        {
            return !string.IsNullOrEmpty(Rig) &&
                 (PF == 1) ? 
                 (GetBody().RotA.Length > 7 ) : 
                 (rotationData.Count > 0 && rotationData[0].Count == 4);
        }

        public bool IsIncomingHandshake()
        {
            return newHandshake && !string.IsNullOrEmpty(version);
        }

        public bool IsStale()
        {
            return !HasRigInfo() || stopwatch.ElapsedMilliseconds >= PoseAIConfig.STALE_TIME_IN_MS;
        }

        public virtual BaseBodyWrapper GetBody()
        {
            return null;
        }

        public PoseAIEvents GetEvents()
        { 
            return GetBody().Events;
        }

        public List<string> GetJointNames()
        {
            return rotationNames;
        }

        public virtual BaseHandWrapper GetLeftHand()
        {
            return null;
        }

        public virtual BaseHandWrapper GetRightHand()
        {
            return null;
        }

        public string GetRootName()
        {
            return rotationNames.Count > 0 ? rotationNames[0] : null;
        }

        public Tuple<List<bool>, List<Quaternion>> GetRotationsCameraSpace()
        {
            List<bool> validity = new List<bool>(rotationNames.Count);
            List<Quaternion> quats = new List<Quaternion>(rotationNames.Count);
            if (PF == 1)
            {
                AppendCompactRotations(ref GetBody().RotA, ref quats);
                validity.AddRange(Enumerable.Repeat(true, quats.Count));
                if (isDesktop && validity.Count >= 1 + lowerBodyNumOfJoints)
                {
                    for(int i = 1; i< lowerBodyNumOfJoints + 1; ++i)
                        validity[i] = false;
                }
                if (GetLeftHand().RotA.Length / 8 != numOfLeftHandJoints)
                {
                    quats.AddRange(Enumerable.Repeat(Quaternion.identity, numOfLeftHandJoints));
                    validity.AddRange(Enumerable.Repeat(false, numOfLeftHandJoints));
                }
                else
                {
                    AppendCompactRotations(ref GetLeftHand().RotA, ref quats);
                    validity.AddRange(Enumerable.Repeat(true, numOfLeftHandJoints));
                }
                if (GetRightHand().RotA.Length / 8 != numOfRightHandJoints)
                {
                    quats.AddRange(Enumerable.Repeat(Quaternion.identity, numOfRightHandJoints));
                    validity.AddRange(Enumerable.Repeat(false, numOfRightHandJoints));
                }
                else
                {
                    AppendCompactRotations(ref GetRightHand().RotA, ref quats);
                    validity.AddRange(Enumerable.Repeat(true, numOfRightHandJoints));
                }
            }
            else
            {
                foreach (List<float> data in rotationData)
                {
                    Quaternion quat = new Quaternion();
                    validity.Add(QuatFromList(data, ref quat));
                    quats.Add(quat);
                }
            }
            return new Tuple<List<bool>, List<Quaternion>>(validity, quats);
        }

        private void AppendCompactRotations(ref string compactString, ref List<Quaternion> quats)
        {            
            if (compactString.Length > 7)
            {
                List<float> flatArray = new List<float>(compactString.Length / 2);
                PoseAI_Decoder.FStringFixed12ToFloat(ref compactString, ref flatArray);
                PoseAI_Decoder.FlatArrayToQuats(ref flatArray, ref quats);
            }
        }

        private void ProcessFace()
        {
#if VERBOSE_PACKET
            if (Face.Count == 52)
            {
                blendshapes = Face;
            }
#else
            if (Face.Length == 104)
            {
                PoseAI_Decoder.FStringFixed12ToFloatInPlace(ref Face, ref blendshapes);
            }
#endif
        }

        private void ProcessTouches()
        {
            if (Touches.Length > 1) {
                // safety valve which clears the queue at a certain size, in case the program is not processing touches
                if (touchQueue.Count > PoseAIConfig.MAX_SIZE_TOUCH_QUEUE)
                    touchQueue.Clear();
                UITouch.DecodeMultiTouchString(ref Touches, ref touchQueue);
                Touches = "";
            }

            if (TouchState.Length > 0)
                UITouch.DecodeMultiTouchStateString(ref TouchState, ref touchStates);
        }

        // convenience routine to get rotation field names and pointers into lists.
        // Only run at creation to avoid introspection overhead, as rig object is reused each frame
        protected internal virtual void BuildListsFromIntrospection() { }

        // converts float list to quaternion and normalizes, adjusting for differences in Unity coordinate system
        // returns true or false if a valid quat is formed (since null is not accepted for quats)
        private static bool QuatFromList(List<float> quatAsList, ref Quaternion quat)
        {
            if (quatAsList == null || quatAsList.Count != 4)
            {
                return false;
            }
            else
            {
                quat.Set(quatAsList[1], -quatAsList[0], quatAsList[3], -quatAsList[2]);
                quat.Normalize();
                return true;
            }
        }
    }


    [System.Serializable]
    public abstract class BaseBodyWrapper
    {
        public VectorsBody Vectors = new VectorsBody();
        public ScalarsBody Scalars = new ScalarsBody();
        public PoseAIEvents Events = new PoseAIEvents();

        // compressed format fields
        public string VisA = "";
        public string ScaA = "";
        public string EveA = "";
        public string VecA = "";
        public string RotA = "";

        public virtual BaseRotationsWrapper GetRotations() { return null; }
    
    }

    [System.Serializable]
    public abstract class BaseRotationsWrapper
    {
        // introspection helper function
        public void AddFields(List<List<float>> rotationData, List<string> rotationNames)
        {
            foreach (var field in this.GetType().GetFields())
            {
                field.SetValue(this, new List<float>());
                rotationData.Add(field.GetValue(this) as List<float>);
                rotationNames.Add(field.Name);
            }
        }

        public int NumOfJoints()
        {
            return this.GetType().GetFields().Length;
        }
    }

    [System.Serializable]
    public class VectorsBody
    {
        public List<float> HipScreen = new List<float> { 0.0f, 0.0f };
        public List<float> ChestScreen = new List<float> { 0.0f, 0.0f };
        public List<float> HipLean = new List<float> { 0.0f, 0.0f };
        public List<float> HandIkL = new List<float> { 0.0f, 0.0f, 0.0f };
        public List<float> HandIkR = new List<float> { 0.0f, 0.0f, 0.0f };
        public List<float> Hip = new List<float> { 0.0f, 0.0f, 0.0f };
        public List<float> FootIkL = new List<float> { 0.0f, 0.0f, 0.0f };
        public List<float> FootIkR = new List<float> { 0.0f, 0.0f, 0.0f };


        public void ProcessCompact(ref string compactString)
        {
            if (compactString.Length < 12) return;
            HipLean[0] = PoseAI_Decoder.FixedB64pairToFloat(compactString[0], compactString[1]) * 180.0f;
            HipLean[1] = PoseAI_Decoder.FixedB64pairToFloat(compactString[2], compactString[3]) * 180.0f;
            HipScreen[0] = PoseAI_Decoder.FixedB64pairToFloat(compactString[4], compactString[5]);
            HipScreen[1] = PoseAI_Decoder.FixedB64pairToFloat(compactString[6], compactString[7]);
            ChestScreen[0] = PoseAI_Decoder.FixedB64pairToFloat(compactString[8], compactString[9]);
            ChestScreen[1] = PoseAI_Decoder.FixedB64pairToFloat(compactString[10], compactString[11]);
            if (compactString.Length < 24) return;
            PoseAI_Decoder.FStringFixed12ToFloat(compactString.Substring(12, 6), ref HandIkL, 4.0f, true);
            PoseAI_Decoder.FStringFixed12ToFloat(compactString.Substring(18, 6), ref HandIkR, 4.0f, true);
            if (compactString.Length < 42) return;
            PoseAI_Decoder.FStringFixed12ToFloat(compactString.Substring(24, 6), ref Hip, 4.0f, true);
            PoseAI_Decoder.FStringFixed12ToFloat(compactString.Substring(30, 6), ref FootIkL, 4.0f, true);
            PoseAI_Decoder.FStringFixed12ToFloat(compactString.Substring(36, 6), ref FootIkR, 4.0f, true);
        }

    }

    [System.Serializable]
    public class ScalarsBody
    {
        /** visibility flags, corresponding to figure in the app.  Use the visibility class in the Rig class instead as these do not handle compact packets or track updates **/
        public float VisTorso = 0.0f;
        public float VisArmL = 0.0f;
        public float VisArmR = 0.0f;
        public float VisLegL = 0.0f;
        public float VisLegR = 0.0f;
        public float VisFace = 0.0f;


        /** location of left hand relative to body in broad zones */
        public uint HandZoneL = 5;
        private uint InternalHandZoneL = 5;

        /** location of right hand relative to body in broad zones */
        public uint HandZoneR = 5;
        private uint InternalHandZoneR = 5;

        /** Heading in degrees of torso.  0 is heading to camera */
        public float ChestYaw = 0.0f;

        /** Heading in degrees of flattened left foot to right foot vector relative to camera. 0 is parallel to camera */
        public float StanceYaw = 0.0f;

        /** estimated actual height of the subject in clip coordinates (2.0 = full height of image) */
        public float BodyHeight = 0.0f;

        /** whether subject is crouching */
        public uint IsCrouching = 0;
        private uint InternalIsCrouching = 0;

        /** number of feet that have been stable for a few frames */
        public uint StableFoot = 0;
        private uint InternalStableFoot = 0;


        public bool CrouchHasChangedAndUpdate(){return HasChangedAndUpdate(ref InternalIsCrouching, IsCrouching);}
        public bool StableFootHasChangedAndUpdate() { return HasChangedAndUpdate(ref InternalStableFoot, StableFoot); }
        public bool HandZoneLHasChangedAndUpdate() { return HasChangedAndUpdate(ref InternalHandZoneL, HandZoneL); }
        public bool HandZoneRHasChangedAndUpdate() { return HasChangedAndUpdate(ref InternalHandZoneR, HandZoneR); }

        private bool HasChangedAndUpdate(ref uint InternalValue, uint value)
        {
            bool hasTriggered = InternalValue != value;
            InternalValue = value;
            return hasTriggered;
        }

        public void ProcessCompact(ref string compactString){
            if (compactString.Length < 14) return;
            BodyHeight = PoseAI_Decoder.FixedB64pairToFloat(compactString[0], compactString[1]) + 1.0f;
            ChestYaw = PoseAI_Decoder.FixedB64pairToFloat(compactString[2], compactString[3]) * 180.0f;
            StanceYaw = PoseAI_Decoder.FixedB64pairToFloat(compactString[4], compactString[5]) * 180.0f;
            StableFoot = PoseAI_Decoder.UintB64ToUint(compactString[6], compactString[7]);
            HandZoneL = PoseAI_Decoder.UintB64ToUint(compactString[8], compactString[9]);
            HandZoneR = PoseAI_Decoder.UintB64ToUint(compactString[10], compactString[11]);
            IsCrouching = PoseAI_Decoder.UintB64ToUint(compactString[12], compactString[13]);
        }

    }

    public class VisibilityFlags
    {
        public bool isTorso = false;
        public bool isLeftArm = false;
        public bool isRightArm = false;
        public bool isLeftLeg = false;
        public bool isRightLeg = false;
        public bool isFace = false;


        public bool HasChanged() { return hasChanged; }
        public void ProcessVerbose(ref ScalarsBody bodyVerbose)
        {
            hasChanged = false;
            SetAndCheckForChange(bodyVerbose.VisTorso > 0.5f, ref isTorso, ref hasChanged);
            SetAndCheckForChange(bodyVerbose.VisLegL > 0.5f, ref isLeftLeg, ref hasChanged);
            SetAndCheckForChange(bodyVerbose.VisLegR > 0.5f, ref isRightLeg, ref hasChanged);
            SetAndCheckForChange(bodyVerbose.VisArmL > 0.5f, ref isLeftArm, ref hasChanged);
            SetAndCheckForChange(bodyVerbose.VisArmR > 0.5f, ref isRightArm, ref hasChanged);
            SetAndCheckForChange(bodyVerbose.VisFace > 0.5f, ref isFace, ref hasChanged);
        }

        public void ProcessCompact(ref string visString)
        {
            hasChanged = false;
            SetAndCheckForChange(visString[0] != '0', ref isTorso, ref hasChanged);
            SetAndCheckForChange(visString[1] != '0', ref isLeftLeg, ref hasChanged);
            SetAndCheckForChange(visString[2] != '0', ref isRightLeg, ref hasChanged);
            SetAndCheckForChange(visString[3] != '0', ref isLeftArm, ref hasChanged);
            SetAndCheckForChange(visString[4] != '0', ref isRightArm, ref hasChanged);
            if (visString.Length>5)
                SetAndCheckForChange(visString[5] != '0', ref isFace, ref hasChanged);
        }
        private bool hasChanged = false;

        private void SetAndCheckForChange(bool newValue, ref bool field, ref bool changeFlag)
        {
            if (newValue != field)
                changeFlag = true;
            field = newValue;
        }
    }

    [System.Serializable]
    public class EventPairBase
    {
        /** number of events registered by camera */
        public uint Count = 0;
        
        /* once per update use this function to check each relevant event to see if a trigger was detected.  It will update the internal counter */
        public bool CheckTriggerAndUpdate()
        {
            bool hasTriggered = InternalCount != Count;
            InternalCount = Count;
            return hasTriggered;
        }
        private uint InternalCount = 0;

        public virtual void ProcessCompact(string compactString) { }
    }

    [System.Serializable]
    public class EventPair : EventPairBase
    {
        /**magnitude of event where approrpiate */
        public float Magnitude = 0.0f;
        public override void ProcessCompact(string compactString) {
            Count = PoseAI_Decoder.UintB64ToUint(compactString[0], compactString[1], compactString[2]);
            Magnitude = PoseAI_Decoder.FixedB64pairToFloat(compactString[3], compactString[4]);
        }
    }

    

    [System.Serializable]
    public class GesturePair : EventPairBase
    {
        /**index code for most recent gesture */
        public uint Current = 0;
        public override void ProcessCompact(string compactString) {
            Count = PoseAI_Decoder.UintB64ToUint(compactString[0], compactString[1], compactString[2]);
            Current = PoseAI_Decoder.UintB64ToUint(compactString[3], compactString[4]);
        }
        public PoseAI_Gestures GetGesture()
        {
            return PoseAI_Methods.Gesture(Current);
        }
    }

    /* structure to receive event notifications */
    [System.Serializable]
    public class PoseAIEvents
    {
        /** number of footsteps registered by camera, body height magnitude */
        public EventPair Footstep = new EventPair();

        /** number of left foot sidesteps registered by camera. sign indicates direction */
        public EventPair SidestepL = new EventPair();

        /** number of right foot sidesteps registered by camera. sign indicates direction */
        public EventPair SidestepR = new EventPair();

        /** number of jumps registered by camera */
        public EventPair Jump = new EventPair();

        /** number of footsplits registered by camera, body width magnitude */
        public EventPair FeetSplit = new EventPair();

        /** number of arm pumps registered by camera, body height magnitude */
        public EventPair ArmPump = new EventPair();

        /** number of arm flexes registered by camera, body width magnitude */
        public EventPair ArmFlex = new EventPair();

        /** number of left or dual arm gestures registered by camera and most recent gesture */
        public GesturePair ArmGestureL = new GesturePair();

        /** number of right arm gestures registered by camera and most recent gesture */
        public GesturePair ArmGestureR = new GesturePair();

        public PoseAIEvents()
        {
            compactOrder = new List<EventPairBase> { Footstep, SidestepL, SidestepR, Jump, FeetSplit, ArmPump, ArmFlex, ArmGestureL, ArmGestureR };
        }

        public void ClearEventTriggers()
        {
            foreach(var e in compactOrder)
                e.CheckTriggerAndUpdate();
        }

        public void ProcessCompact(ref string compactString)
        {
            List<EventPairBase> compactOrder = new List<EventPairBase> { Footstep, SidestepL, SidestepR, Jump, FeetSplit, ArmPump, ArmFlex, ArmGestureL, ArmGestureR };
            if (compactString.Length < compactOrder.Count * 5)
            {
                // string is invalid or missing (may be excluded from SDK for speed).
                return;
            }
            for (int i = 0; i < compactOrder.Count; ++i)
            {
                if (compactString.Length < 5 * i  + 5)
                    break;
                compactOrder[i].ProcessCompact(compactString.Substring(i * 5, 5));
            }
        }
        private List<EventPairBase> compactOrder;
    }

    [System.Serializable]
    public abstract class BaseHandWrapper
    {
        public VectorsHand Vectors = new VectorsHand();
        public ScalarsHand Scalars = new ScalarsHand();

        // compressed format field
        public string RotA = "";

        public string Point = "";


        // fist = 0.0, fully extended fingers = 1.0, can vary depending on number of straight fingers.
        public float Open = 0.5f;

        public void ProcessCompact()
        {
            if (Point.Length >= 4)
            {
                Vectors.PointScreen[0] = PoseAI_Decoder.FixedB64pairToFloat(Point[0], Point[1]);
                Vectors.PointScreen[1] = PoseAI_Decoder.FixedB64pairToFloat(Point[2], Point[3]);
            }
            if (Point.Length >= 8)
            {
                Vectors.ThumbScreen[0] = PoseAI_Decoder.FixedB64pairToFloat(Point[4], Point[5]);
                Vectors.ThumbScreen[1] = PoseAI_Decoder.FixedB64pairToFloat(Point[6], Point[7]);
            }
        }


    }

    [System.Serializable]
    public class ScalarsHand { }

    
    [System.Serializable]
    public class VectorsHand
    {
        public List<float> PointScreen = new List<float> { 0.0f, 0.0f };
        public List<float> ThumbScreen = new List<float> { 0.0f, 0.0f };
    }



}