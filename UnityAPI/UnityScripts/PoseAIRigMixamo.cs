// Copyright 2021 Pose AI Ltd. All rights reserved

using UnityEngine;
using System.Collections.Generic;


namespace PoseAI
{
    [System.Serializable]
    public class PoseAIRigMixamo : PoseAIRigBase
    {
        public PoseAIRigMixamo() : base(new Quaternion(0.0f, 0.7071f, 0.7071f, 0.0f)){}
    
        [System.Serializable]
        public class BodyWrapper : BaseBodyWrapper
        {
            [System.Serializable]
            public class RotationsWrapper : BaseRotationsWrapper
            {
                // Rig specific field names go here
                public List<float> Hips;
                public List<float> RightUpLeg;
                public List<float> RightLeg;
                public List<float> RightFoot;
                public List<float> RightToeBase;
                public List<float> LeftUpLeg;
                public List<float> LeftLeg;
                public List<float> LeftFoot;
                public List<float> LeftToeBase;
                public List<float> Spine;
                public List<float> Spine1;
                public List<float> Spine2;
                public List<float> Neck;
                public List<float> Head;
                public List<float> LeftShoulder;
                public List<float> LeftArm;
                public List<float> LeftForeArm;
                public List<float> RightShoulder;
                public List<float> RightArm;
                public List<float> RightForeArm;
                
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
                public List<float> LeftHand;
                public List<float> LeftForeArmTwist;
                public List<float> LeftHandIndex1;
                public List<float> LeftHandIndex2;
                public List<float> LeftHandIndex3;
                public List<float> LeftHandMiddle1;
                public List<float> LeftHandMiddle2;
                public List<float> LeftHandMiddle3;
                public List<float> LeftHandRing1;
                public List<float> LeftHandRing2;
                public List<float> LeftHandRing3;
                public List<float> LeftHandPinky1;
                public List<float> LeftHandPinky2;
                public List<float> LeftHandPinky3;
                public List<float> LeftHandThumb1;
                public List<float> LeftHandThumb2;
                public List<float> LeftHandThumb3;

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
                public List<float> RightHand;
                public List<float> RightForeArmTwist;
                public List<float> RightHandIndex1;
                public List<float> RightHandIndex2;
                public List<float> RightHandIndex3;
                public List<float> RightHandMiddle1;
                public List<float> RightHandMiddle2;
                public List<float> RightHandMiddle3;
                public List<float> RightHandRing1;
                public List<float> RightHandRing2;
                public List<float> RightHandRing3;
                public List<float> RightHandPinky1;
                public List<float> RightHandPinky2;
                public List<float> RightHandPinky3;
                public List<float> RightHandThumb1;
                public List<float> RightHandThumb2;
                public List<float> RightHandThumb3;

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
                { 21, "LeftForeArmTwist" },
                { 38, "RightForeArmTwist" },
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
