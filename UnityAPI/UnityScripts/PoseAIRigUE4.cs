// Copyright 2021 Pose AI Ltd. All rights reserved

using UnityEngine;
using System.Collections.Generic;


namespace PoseAI
{
    [System.Serializable]
    public class PoseAIRigUE4 : PoseAIRigBase
    {
    
        // set to 180 degree rotation around Z based on how a test Unreal rig imported
        public PoseAIRigUE4():base(new Quaternion(0.0f, 0.0f, 1.0f, 0.0f)){}
    
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
                public List<float> neck_01;
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
                public List<float> index_01_l;
                public List<float> index_02_l;
                public List<float> index_03_l;
                public List<float> middle_01_l;
                public List<float> middle_02_l;
                public List<float> middle_03_l;
                public List<float> ring_01_l;
                public List<float> ring_02_l;
                public List<float> ring_03_l;
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
                public List<float> index_01_r;
                public List<float> index_02_r;
                public List<float> index_03_r;
                public List<float> middle_01_r;
                public List<float> middle_02_r;
                public List<float> middle_03_r;
                public List<float> ring_01_r;
                public List<float> ring_02_r;
                public List<float> ring_03_r;
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
        
        protected internal override void BuildListsFromIntrospection(){
            Body.Rotations.AddFields(rotationData, rotationNames);
            LeftHand.Rotations.AddFields(rotationData, rotationNames);
            RightHand.Rotations.AddFields(rotationData, rotationNames);
            numOfBodyJoints = Body.Rotations.NumOfJoints();
            numOfLeftHandJoints = LeftHand.Rotations.NumOfJoints();
            numOfRightHandJoints = RightHand.Rotations.NumOfJoints();
        }
        public override Dictionary<int, string> GetExtraBones()
        {
            return new()
            {
                { 21, "lowerarm_twist_01_l" },
                { 38, "lowerarm_twist_01_r" },
            };
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


