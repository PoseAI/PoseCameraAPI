// Copyright 2021 Pose AI Ltd. All rights reserved


using UnityEngine;
using System.Collections;


namespace PoseAI
{
    public static class PoseAIConfig
    {
        public const int STALE_TIME_IN_MS = 10000;
        public const int MIN_CAMERA_FPS = 30;
        public const int MAX_SIZE_TOUCH_QUEUE = 20;

    }

    public enum PoseAI_Modes { Room, SeatedAtDesk, Portrait, RoomBodyOnly, PortraitBodyOnly };
    public enum PoseAI_Rigs { Unity, UE4, Mixamo, MixamoAlt, MetaHuman };
    public enum PoseAI_Gestures {None,
        Swipe1, Swipe2, Swipe3, Swipe4, Swipe6, Swipe7, Swipe8, Swipe9,
        FlapLateral, Flap, WaxOn, WaxOff, OverheadClapSmall, OverheadClap, ReverseOverheadClap};
    public enum TouchState { Begun, Touching, Ended, Cancelled};
    
    public enum PoseAI_CameraQuality {VGA, _360p, _720p, _1080p, QHD, _4K}

    public static class PoseAI_Methods
    {
        public static Resolution Resolution(PoseAI_CameraQuality quality)
        {
            switch (quality)
            {
                case PoseAI_CameraQuality.VGA: return new Resolution { width= 480, height = 640 };
                case PoseAI_CameraQuality._360p: return new Resolution { width = 360, height = 640 };
                case PoseAI_CameraQuality._720p: return new Resolution { width = 1280, height = 720 };
                case PoseAI_CameraQuality._1080p: return new Resolution { width = 1920, height = 1080 };
                case PoseAI_CameraQuality.QHD: return new Resolution { width = 2560, height = 1440 };
                case PoseAI_CameraQuality._4K: return new Resolution { width = 3840, height = 2160 };
                default: return new Resolution { width = 480, height = 640 };
            }
        }
        public static PoseAI_Gestures Gesture(this uint s1)
        {
            switch (s1)
            {
                case 1: return PoseAI_Gestures.Swipe1;
                case 2: return PoseAI_Gestures.Swipe2;
                case 3: return PoseAI_Gestures.Swipe3;
                case 4: return PoseAI_Gestures.Swipe4;
                case 6: return PoseAI_Gestures.Swipe6;
                case 7: return PoseAI_Gestures.Swipe7;
                case 8: return PoseAI_Gestures.Swipe8;
                case 9: return PoseAI_Gestures.Swipe9;
                case 10: return PoseAI_Gestures.FlapLateral;
                case 11: return PoseAI_Gestures.WaxOn;
                case 12: return PoseAI_Gestures.WaxOff;
                case 51: return PoseAI_Gestures.ReverseOverheadClap;
                case 50: return PoseAI_Gestures.OverheadClap;
                case 53: return PoseAI_Gestures.Flap;
                case 52: return PoseAI_Gestures.OverheadClapSmall;   
                default:
                    return PoseAI_Gestures.None;
            }
        }

    
        public static bool IsSeatedAtDesk(PoseAI_Modes mode) => mode switch
        {
            PoseAI_Modes.SeatedAtDesk => true,
            _ => false
        };
    }
      
}

