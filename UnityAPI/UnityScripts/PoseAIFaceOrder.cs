// Copyright 2023 Pose AI Ltd. All rights reserved

using System.Collections.Generic;
using UnityEngine;

namespace PoseAI
{
	//specify your rig's blendshape order here, as Unity sets them by index order rather than by name
	public class PoseAIFaceBlendShapeReorder
    {
		static public List<PoseAIFaceBlendShapes> MeshOrder = new List<PoseAIFaceBlendShapes> {
			PoseAIFaceBlendShapes.BrowOuterUpLeft,
			PoseAIFaceBlendShapes.BrowOuterUpRight,
			PoseAIFaceBlendShapes.EyeSquintLeft,
			PoseAIFaceBlendShapes.EyeSquintRight,
			PoseAIFaceBlendShapes.EyeLookInLeft,
			PoseAIFaceBlendShapes.EyeLookOutLeft,
			PoseAIFaceBlendShapes.EyeLookInRight,
			PoseAIFaceBlendShapes.EyeLookOutRight,
			PoseAIFaceBlendShapes.EyeLookUpLeft,
			PoseAIFaceBlendShapes.EyeLookUpRight,
			PoseAIFaceBlendShapes.EyeLookDownLeft,
			PoseAIFaceBlendShapes.EyeLookDownRight,
			PoseAIFaceBlendShapes.CheekPuff,
			PoseAIFaceBlendShapes.CheekSquintLeft,
			PoseAIFaceBlendShapes.CheekSquintRight,
			PoseAIFaceBlendShapes.NoseSneerLeft,
			PoseAIFaceBlendShapes.NoseSneerRight,
			PoseAIFaceBlendShapes.MouthLeft,
			PoseAIFaceBlendShapes.MouthRight,
			PoseAIFaceBlendShapes.MouthPucker,
			PoseAIFaceBlendShapes.MouthFunnel,
			PoseAIFaceBlendShapes.MouthSmileLeft,
			PoseAIFaceBlendShapes.MouthSmileRight,
			PoseAIFaceBlendShapes.MouthFrownLeft,
			PoseAIFaceBlendShapes.MouthFrownRight,
			PoseAIFaceBlendShapes.MouthDimpleLeft,
			PoseAIFaceBlendShapes.MouthDimpleRight,
			PoseAIFaceBlendShapes.MouthPressLeft,
			PoseAIFaceBlendShapes.MouthPressRight,
			PoseAIFaceBlendShapes.MouthShrugLower,
			PoseAIFaceBlendShapes.MouthShrugUpper,
			PoseAIFaceBlendShapes.MouthStretchLeft,
			PoseAIFaceBlendShapes.MouthStretchRight,
			PoseAIFaceBlendShapes.MouthUpperUpLeft,
			PoseAIFaceBlendShapes.MouthUpperUpRight,
			PoseAIFaceBlendShapes.MouthLowerDownLeft,
			PoseAIFaceBlendShapes.MouthLowerDownRight,
			PoseAIFaceBlendShapes.MouthRollUpper,
			PoseAIFaceBlendShapes.MouthRollLower,
			PoseAIFaceBlendShapes.MouthClose,
			PoseAIFaceBlendShapes.JawForward,
			PoseAIFaceBlendShapes.JawOpen,
			PoseAIFaceBlendShapes.JawLeft,
			PoseAIFaceBlendShapes.JawRight,
			PoseAIFaceBlendShapes.BrowInnerUp,
			PoseAIFaceBlendShapes.EyeBlinkRight,  // odd that Unity sample mesh puts right before left
			PoseAIFaceBlendShapes.EyeBlinkLeft,
			PoseAIFaceBlendShapes.BrowDownLeft,
			PoseAIFaceBlendShapes.BrowDownRight,
			PoseAIFaceBlendShapes.EyeWideRight, // odd that Unity sample mesh puts right before left
			PoseAIFaceBlendShapes.EyeWideLeft,
			PoseAIFaceBlendShapes.JawOpen,
			PoseAIFaceBlendShapes.JawForward,
			PoseAIFaceBlendShapes.JawLeft,
			PoseAIFaceBlendShapes.JawRight,
			PoseAIFaceBlendShapes.TongueOut
		};

		static public List<float> ReorderBlendshapes(ref List<float> default_blendshapes)
        {
			List<float> reordered_blendshapes = new List<float>();
			foreach(var shape in MeshOrder)
            {
				reordered_blendshapes.Add(default_blendshapes[(int)shape]);
            }
			return reordered_blendshapes;
        }



		public enum PoseAIFaceBlendShapes : int
		{
			// Left eye blend shapes
			EyeBlinkLeft,
			EyeLookDownLeft,
			EyeLookInLeft,
			EyeLookOutLeft,
			EyeLookUpLeft,
			EyeSquintLeft,
			EyeWideLeft,
			// Right eye blend shapes
			EyeBlinkRight,
			EyeLookDownRight,
			EyeLookInRight,
			EyeLookOutRight,
			EyeLookUpRight,
			EyeSquintRight,
			EyeWideRight,
			// Jaw blend shapes
			JawForward,
			JawLeft,
			JawRight,
			JawOpen,
			// Mouth blend shapes
			MouthClose,
			MouthFunnel,
			MouthPucker,
			MouthLeft,
			MouthRight,

			MouthSmileLeft,
			MouthSmileRight,
			MouthFrownLeft,
			MouthFrownRight,
			MouthDimpleLeft,
			MouthDimpleRight,
			MouthStretchLeft,
			MouthStretchRight,

			MouthRollLower,
			MouthRollUpper,
			MouthShrugLower,
			MouthShrugUpper,
			MouthPressLeft,
			MouthPressRight,
			MouthLowerDownLeft,
			MouthLowerDownRight,
			MouthUpperUpLeft,
			MouthUpperUpRight,
			// Brow blend shapes
			BrowDownLeft,
			BrowDownRight,
			BrowInnerUp,
			BrowOuterUpLeft,
			BrowOuterUpRight,
			// Cheek blend shapes
			CheekPuff,
			CheekSquintLeft,
			CheekSquintRight,
			// Nose blend shapes
			NoseSneerLeft,
			NoseSneerRight,
			TongueOut
		};


	}
}
