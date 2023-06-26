// Copyright 2021 Pose AI Ltd. All rights reserved

using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using UnityEngine;

namespace PoseAI
{
    /*
    * A component which handles UDP communication with the Pose Camera app.
    */
    public class PoseAISourceNetwork : PoseAISource
    {
        [Tooltip("Specify listening port. Must be unique per source and valid for aa port (i.e. 1024-65535)")]
        public int Port = 8080;
        
        [Tooltip("Format of rig for streaming. Determines joint names and reference neutral rotation.")]
        public PoseAI_Rigs RigType = PoseAI_Rigs.Unity;
        
        [Tooltip("Sets the camera mode on the paired app.")]
        public PoseAI_Modes Mode = PoseAI_Modes.Room;
        
        [Tooltip("Whether to flip left/right and facing like in a mirror. Set false for follow third-person mode.")]
        public bool MirrorCamera = false;

        [Tooltip("Whether to predict blendshapes for the face.")]
        public bool FaceOn = true;

        [Tooltip("Requested app camera framerate.  iOS will only use 30 or 60 currently.")]
        public int CameraFPS = 60;
        
        [Tooltip("Requested app streaming interpolation framerate, to smooth animations.  Suggest matching target framerate of Unity application.")]
        public int SyncFPS = 0;
        
        
        // each source must have a unique port
        private static List<int> _usedPorts = new List<int>();
        
        // JSON format string which tells app to stop streaming and disconnect
        private static string _disconnectString = "{\"REQUESTS\":[\"DISCONNECT\"]}";

        private string[] _rotationNames = Array.Empty<string>();
        private Thread _receivingThread;
        private ManualResetEvent _stopReceivingThread;
        private IPEndPoint _currentEndPoint;
        private UdpClient _udpSender;
        private PoseAIRigBase _rigObj;


        public override PoseAIRigBase GetRig()
        {
            if (_rigObj == null)
                _rigObj = PoseAIRigFactory.SelectRig(RigType);
            return _rigObj;
        }

        // sends a disconnect message, unreliable over UDP.  If currentEndPoint 
        public void Disconnect(){
            Disconnect(_currentEndPoint);
            _currentEndPoint = null;
        }
        
        // custom client for socket set to nonblocking, non exclusive address.
        private static UdpClient PoseAIUDPClient(int portIn){
            var udpClient = new UdpClient();
            var endPoint = new IPEndPoint(IPAddress.Any, portIn);
            udpClient.ExclusiveAddressUse = false;
            udpClient.Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
            udpClient.Client.Bind(endPoint);
            return udpClient;
        }
        
        private void Start()
        {
            if (_usedPorts.Contains(Port)){
                Debug.LogError("A PoseAI Source is already using port " + Port.ToString());
                return;
            }
            if (Port < 1024 && Port > 65535){
                Debug.LogError("A PoseAI Source is set to use an invalid port number:" + Port.ToString());
                return;
            }
            _usedPorts.Add(Port);
            _receivingThread = new Thread(RunServer)
            {
                Name = nameof(RunServer),
                IsBackground = true
            };
            _stopReceivingThread = new ManualResetEvent(false);
            _receivingThread.Start();
        }

        private void OnDestroy()
        {
            try
            {
                _stopReceivingThread?.Set();
                _receivingThread?.Join();
               
            }
            catch (ObjectDisposedException) { }
            catch (ThreadInterruptedException) { }
        }
      
        private void RunServer()
        {
            using (_stopReceivingThread)
            {
                using (var udpListener = PoseAIUDPClient(Port))
                {
                    _udpSender = PoseAIUDPClient(Port);
                    while (!_stopReceivingThread.WaitOne(0))
                    {
                        var result = udpListener.BeginReceive(null, null);
                        var waitany = WaitHandle.WaitAny(new[] { result.AsyncWaitHandle, _stopReceivingThread });

                        if (waitany == 0)
                        {
                            var endPoint = new IPEndPoint(IPAddress.Any, Port);
                            byte[] packet = null;
                            try
                            {
                                packet = udpListener.EndReceive(result, ref endPoint);
                            }
                            catch (SocketException ex)
                            {
                                Debug.LogWarning("PoseAI: SocketException thrown from udp client. "  + ex.Message);
                                if (_currentEndPoint != null && endPoint.Address.Equals(_currentEndPoint.Address))
                                    _currentEndPoint = null;
                                    continue;
                            }
                            catch (ObjectDisposedException)
                            {
                                Debug.LogWarning("PoseAI: ObjectDisposedException thrown from udp client.  Restarting");
                                if (_currentEndPoint != null && endPoint.Address.Equals(_currentEndPoint.Address))
                                    _currentEndPoint = null;
                                continue;
                                    
                            }
                            if (_currentEndPoint != null && endPoint.Address.Equals(_currentEndPoint.Address))
                            {
                                _currentEndPoint = endPoint;
                                AcceptPacket(packet);
                            }
                            else if (GetRig().IsStale())
                            {
                                Debug.Log("New connection from " + endPoint.ToString());
                                _currentEndPoint = endPoint;
                                AcceptPacket(packet);
                            }
                            else
                            {
                                Debug.Log("New connection attempted by " + endPoint.ToString() + " but already engaged by " + _currentEndPoint.ToString());
                                Disconnect(endPoint);
                            }

                        }
                    }
                        
                    Disconnect();
                    if (_udpSender != null)
                        _udpSender.Close();
                }
            }
            _usedPorts.Remove(Port);
        }
        

        private void AcceptPacket(byte[] packet){
            if (packet != null){
                string packetAsString = Encoding.UTF8.GetString(packet);

                if (!GetRig().OverwriteFromJSON(packetAsString))
                {
                    Debug.LogWarning("Could not read packet from " + _currentEndPoint.ToString());
                    Debug.LogWarning(packetAsString);
                    _currentEndPoint = null;
                }
                if (_rigObj.IsIncomingHandshake())
                    ExchangeHandshake();
            }
        }

        private void ExchangeHandshake()
        {
            PoseAIHandshake handshakeObj = PoseAIHandshake.Factory(Mode, RigType, MirrorCamera, FaceOn, SyncFPS, CameraFPS);
            Debug.Log("Exchanging Handshake");
            string handshake = JsonUtility.ToJson(handshakeObj, true);
            Debug.Log(handshake);
            byte[] data = Encoding.UTF8.GetBytes(handshake);
            _udpSender.Send(data, data.Length, _currentEndPoint);
        }
        
        
        private void Disconnect(IPEndPoint endPoint)
        {
            if (endPoint != null && _udpSender != null){
                byte[] data = Encoding.UTF8.GetBytes(_disconnectString);
                    _udpSender.Send(data, data.Length, endPoint);
            }
        }
        


    }
        
}
