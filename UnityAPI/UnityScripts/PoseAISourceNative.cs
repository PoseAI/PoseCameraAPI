// Copyright 2022 Pose AI Ltd. All rights reserved

using System;
using UnityEngine;

namespace PoseAI
{
    /*
     * A character component which can be used to supply a source to the 
     * PoseAICharacterController and PoseAIAnimator.  This source uses a direct call to
     * ProcessNativePacket to provide the PoseAI Engine information as an formated packet.
     * This is useful when the Unity game and the engine are on the same device, and could
     * be connected with simple messaging (similar to the Unity iOS demo app)
     */
    public class PoseAISourceNative : PoseAISource
    {
        [Tooltip("Format of the rig for streaming.  Determines joint names and reference neutral rotations")]
        public PoseAI_Rigs RigType = PoseAI_Rigs.Unity;

        private PoseAIRigBase _rigObj;

        public override PoseAIRigBase GetRig()
        {
            if (_rigObj == null)
                _rigObj = PoseAIRigFactory.SelectRig(RigType);
            return _rigObj;
        }

        public void ProcessNativePacket(string packetAsJsonString)
        {
            if (!GetRig().OverwriteFromJSON(packetAsJsonString))
            {
                Debug.Log("Could not read native packet: " + packetAsJsonString);
            }
        }
    }
    
}
