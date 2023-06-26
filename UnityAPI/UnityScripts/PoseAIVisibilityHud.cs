// Copyright 2021-2023 Pose AI Ltd. All rights reserved

using System;
using UnityEngine;

namespace PoseAI
{

    
    public class PoseAIVisibilityHud : MonoBehaviour
    {
        [Tooltip("Specify a direct source created elsewhere.")]
        public PoseAISource Source;

        public GameObject LeftArm;
        public GameObject RightArm;
        public GameObject Torso;
        public GameObject LeftLeg;
        public GameObject RightLeg;
        public GameObject Face;
        private void Update()
        {
            if (Source)
            {
                var visibility = Source.GetRig().visibility;
                if (LeftArm)
                    LeftArm.SetActive(!visibility.isLeftArm);
                if (RightArm)
                    RightArm.SetActive(!visibility.isRightArm);
                if (Torso)
                    Torso.SetActive(!visibility.isTorso);
                if (LeftLeg)
                    LeftLeg.SetActive(!visibility.isLeftLeg);
                if (RightLeg)
                    RightLeg.SetActive(!visibility.isRightLeg);
                if (Face)
                    Face.SetActive(!visibility.isFace);
            }
        }
        
    }
    
}
