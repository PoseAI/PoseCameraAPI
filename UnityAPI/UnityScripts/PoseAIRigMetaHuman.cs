// Copyright 2023 Pose AI Ltd. All rights reserved


// Important: Importing a MetaHumans avatar into Unity is a violation of Epic Games licensing permissions, and is not a permitted use of this script.  
// We only provide this script for 3rd PARTY (non Epic Games) rigs which are formated to use the same skeleton naming convention and joint orientations as the MetaHuman template


using UnityEngine;
using System.Collections.Generic;

//
namespace PoseAI
{
    [System.Serializable]
    public class PoseAIRigMetaHuman : PoseAIRigBase
    {
    
        // set to 180 degree rotation around Z based on how a test Unreal rig imported
        public PoseAIRigMetaHuman():base(new Quaternion(0.0f, 0.7f, 0.7f, 0.0f)){}
    
        [System.Serializable]
        public class BodyWrapper : BaseBodyWrapper
        {
            [System.Serializable]
            public class RotationsWrapper : BaseRotationsWrapper
            {
                // Rig specific field names go here
                public List<float> pelvis;
                public List<float> thigh_r;
                public List<float> calf_r;
                public List<float> foot_r;
                public List<float> ball_r;
                public List<float> thigh_l;
                public List<float> calf_l;
                public List<float> foot_l;
                public List<float> ball_l;
                public List<float> spine_01;
                public List<float> spine_02;
                public List<float> spine_03;
                public List<float> spine_04;
                public List<float> spine_05;
                public List<float> neck_01;
                public List<float> neck_02;
                public List<float> head;
                public List<float> clavicle_l;
                public List<float> upperarm_l;
                public List<float> lowerarm_l;
                public List<float> clavicle_r;
                public List<float> upperarm_r;
                public List<float> lowerarm_r;
            }
            public RotationsWrapper Rotations = new RotationsWrapper();
        }
        
        [System.Serializable]
        public class LeftHandWrapper : BaseHandWrapper
        {
            [System.Serializable]
            public class RotationsWrapper : BaseRotationsWrapper
            {
                // Rig specific field names go here
                public List<float> hand_l;
                public List<float> lowerarm_twist_01_l;
                public List<float> lowerarm_twist_02_l;
                public List<float> index_metacarpal_l;
                public List<float> index_01_l;
                public List<float> index_02_l;
                public List<float> index_03_l;
                public List<float> middle_carpal_l;
                public List<float> middle_01_l;
                public List<float> middle_02_l;
                public List<float> middle_03_l;
                public List<float> ring_metacarpal_l;
                public List<float> ring_01_l;
                public List<float> ring_02_l;
                public List<float> ring_03_l;
                public List<float> pinky_metacarpal_l;
                public List<float> pinky_01_l;
                public List<float> pinky_02_l;
                public List<float> pinky_03_l;
                public List<float> thumb_01_l;
                public List<float> thumb_02_l;
                public List<float> thumb_03_l;
            }
            public RotationsWrapper Rotations = new RotationsWrapper();

        }
        
        [System.Serializable]
        public class RightHandWrapper : BaseHandWrapper
        {
            [System.Serializable]
            public class RotationsWrapper : BaseRotationsWrapper
            {
                // Rig specific field names go here
                public List<float> hand_r;
                public List<float> lowerarm_twist_01_r;
                public List<float> lowerarm_twist_02_r;
                public List<float> index_metacarpal_r;
                public List<float> index_01_r;
                public List<float> index_02_r;
                public List<float> index_03_r;
                public List<float> middle_carpal_r;
                public List<float> middle_01_r;
                public List<float> middle_02_r;
                public List<float> middle_03_r;
                public List<float> ring_metacarpal_r;
                public List<float> ring_01_r;
                public List<float> ring_02_r;
                public List<float> ring_03_r;
                public List<float> pinky_metacarpal_r;
                public List<float> pinky_01_r;
                public List<float> pinky_02_r;
                public List<float> pinky_03_r;
                public List<float> thumb_01_r;
                public List<float> thumb_02_r;
                public List<float> thumb_03_r;
            }
            public RotationsWrapper Rotations = new RotationsWrapper();
        }
        
        public BodyWrapper Body = new BodyWrapper();
        public LeftHandWrapper LeftHand = new LeftHandWrapper();
        public RightHandWrapper RightHand = new RightHandWrapper();

        protected internal override void BuildListsFromIntrospection()
        {
            Body.Rotations.AddFields(rotationData, rotationNames);
            LeftHand.Rotations.AddFields(rotationData, rotationNames);
            RightHand.Rotations.AddFields(rotationData, rotationNames);
            numOfBodyJoints = Body.Rotations.NumOfJoints();
            numOfLeftHandJoints = LeftHand.Rotations.NumOfJoints();
            numOfRightHandJoints = RightHand.Rotations.NumOfJoints();
        }

        public static readonly List<HumanBodyBones> metahuman_bones = new List<HumanBodyBones>{
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
            HumanBodyBones.LastBone, //Spine4
            HumanBodyBones.LastBone, //Spine5
            HumanBodyBones.Neck,
            HumanBodyBones.LastBone, //Neck2 not in unity avatar system, using LastBone as a Null value
            HumanBodyBones.Head,
            HumanBodyBones.LeftShoulder,
            HumanBodyBones.LeftUpperArm,
            HumanBodyBones.LeftLowerArm,
            HumanBodyBones.RightShoulder,
            HumanBodyBones.RightUpperArm,
            HumanBodyBones.RightLowerArm,
            HumanBodyBones.LeftHand,
            HumanBodyBones.LastBone, //forearm twist not in unity avatar system, using LastBone as a Null value
            HumanBodyBones.LastBone, //forearm twist not in unity avatar system, using LastBone as a Null value
            HumanBodyBones.LastBone, //metacarpal not in unity avatar system, using LastBone as a Null value
            HumanBodyBones.LeftIndexProximal,
            HumanBodyBones.LeftIndexIntermediate,
            HumanBodyBones.LeftIndexDistal,
            HumanBodyBones.LastBone, //metacarpal not in unity avatar system, using LastBone as a Null value
            HumanBodyBones.LeftMiddleProximal,
            HumanBodyBones.LeftMiddleIntermediate,
            HumanBodyBones.LeftMiddleDistal,
            HumanBodyBones.LastBone, //metacarpal not in unity avatar system, using LastBone as a Null value
            HumanBodyBones.LeftRingProximal,
            HumanBodyBones.LeftRingIntermediate,
            HumanBodyBones.LeftRingDistal,
            HumanBodyBones.LastBone, //metacarpal not in unity avatar system, using LastBone as a Null value
            HumanBodyBones.LeftLittleProximal,
            HumanBodyBones.LeftLittleIntermediate,
            HumanBodyBones.LeftLittleDistal,
            HumanBodyBones.LeftThumbProximal,
            HumanBodyBones.LeftThumbIntermediate,
            HumanBodyBones.LeftThumbDistal,
            HumanBodyBones.RightHand,
            HumanBodyBones.LastBone,  //forearm twist not in unity avatar system,  using LastBone as a Null value
            HumanBodyBones.LastBone, //forearm twist not in unity avatar system, using LastBone as a Null value
            HumanBodyBones.LastBone, //metacarpal not in unity avatar system, using LastBone as a Null value
            HumanBodyBones.RightIndexProximal,
            HumanBodyBones.RightIndexIntermediate,
            HumanBodyBones.RightIndexDistal,
            HumanBodyBones.LastBone, //metacarpal not in unity avatar system, using LastBone as a Null value
            HumanBodyBones.RightMiddleProximal,
            HumanBodyBones.RightMiddleIntermediate,
            HumanBodyBones.RightMiddleDistal,
            HumanBodyBones.LastBone, //metacarpal not in unity avatar system, using LastBone as a Null value
            HumanBodyBones.RightRingProximal,
            HumanBodyBones.RightRingIntermediate,
            HumanBodyBones.RightRingDistal,
            HumanBodyBones.LastBone, //metacarpal not in unity avatar system, using LastBone as a Null value
            HumanBodyBones.RightLittleProximal,
            HumanBodyBones.RightLittleIntermediate,
            HumanBodyBones.RightLittleDistal,
            HumanBodyBones.RightThumbProximal,
            HumanBodyBones.RightThumbIntermediate,
            HumanBodyBones.RightThumbDistal
        };
        public override List<HumanBodyBones> GetBones()
        {
            return metahuman_bones;
        }
         private static readonly List<int> metaHumanParentIndices = new () {
        0,0,1,2,3,0,5,6,7,0,9,10,11,12,13,14,15,13,17,18,13,20,21,19,19,19,23,26,27,28,23,30,31,32,23,34,35,36,23,38,39,40,23,42,43,22,22,22,45,48,49,50,45,52,53,54,45,56,57,58,45,60,61,62,45,64,65
        };

        public override Dictionary<int, string> GetExtraBones() { return new() {
            { 12, "spine_04" } ,
            { 13, "spine_05" } ,
            { 15, "neck_02" },
            { 24, "lowerarm_twist_01_l"},
            { 25, "lowerarm_twist_02_l" },
            { 26, "index_metacarpal_l" },
            { 30, "middle_metacarpal_l" },
            { 34, "ring_metacarpal_l" },
            { 38, "pinky_metacarpal_l" },
            { 46, "lowerarm_twist_01_r" },
            { 47, "lowerarm_twist_02_r" },
            { 48, "index_metacarpal_r" },
            { 52, "middle_metacarpal_r" },
            { 56, "ring_metacarpal_r" },
            { 60, "pinky_metacarpal_r" },

        }; }

        public override List<int> GetParentIndices()
        {
            return metaHumanParentIndices;
        }
        public override BaseBodyWrapper GetBody()
        {
            return Body;
        }

        public override BaseHandWrapper GetLeftHand()
        {
            return LeftHand;
        }

        public override BaseHandWrapper GetRightHand()
        {
            return RightHand;
        }
    }            
}
           