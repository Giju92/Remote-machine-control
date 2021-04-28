using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using Client.Properties;
using Newtonsoft.Json;

namespace Client
{
    public class Connection
    {
        //Connection variable
        public string Name { get;set; }
        public string IPHost { get; set; }
        public string ImgPath
        {
            get
            {
                List<string> l = GetWifiAddresses();

                foreach (string s in l)
                {
                    
                    if (s.Equals(((IPEndPoint) socket.LocalEndPoint).Address.ToString()))
                    {
                        return Path.GetFullPath("Image/wifi.png");
                    }
                }

                return Path.GetFullPath("Image/ethernet.png");
            }
        }

        //Thread
        private Thread Reader;
        
        //Socket
        Socket socket;
        int port = 1500;

        //Control variable
        private bool IsActive = false;
        private volatile bool IsConnected;
        public string StringDurationTime = "00:00:00";
        private Stopwatch Timer = new Stopwatch(); 
        public ObservableCollection<Process> ProcessList = new ObservableCollection<Process>();
        public ListCollectionView ProcessCollection;
        public ObservableCollection<Log> LogList { get; set; } = new ObservableCollection<Log>();
 
        public bool StartConnection(string IP)
        {
            //SetIP
            IPHost = IP;
            //Socket bool 
            bool isUp = false;

            try
            {
                socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
                if (socket != null)
                {
                    IPEndPoint ipEndPoint = new IPEndPoint(IPAddress.Parse(IP), port);
                    isUp = false;
                    //time out 3 Sec
                    CallWithTimeout(ConnectToProxyServers, 10000, socket, ipEndPoint);

                    if (socket != null && socket.Connected)
                    {
                        //Start Thread
                        IsConnected = true;
                        Reader = new Thread(Read);
                        Reader.Start();
                        WriteListRequest();

                        isUp = true;
                    }
                }
            }
            catch (Exception ex)
            {
                isUp = false;
            }
            
            return isUp;
        }

        public void CloseConnection()
        {
            if (IsConnected)
            {
                MessageBoxResult result = MessageBox.Show("If you close, the connection will be lost", "Confirmation", MessageBoxButton.OKCancel, MessageBoxImage.Warning);
                if (result == MessageBoxResult.OK)
                {
                    WriteCloseCommand();
                    IsConnected = false;
                    socket.Shutdown(SocketShutdown.Send);
                    socket.Close();
                    Timer.Stop();
                }
            }
        }

        public void SetFocus(object sender, RoutedEventArgs e)
        {
            MainWindow.screenMain.SetFocus(this);
        }

        private void Read()
        {
            try
            {
                while (true)
                {

                    byte[] Dimension = new byte[7];
                    byte[] bytes = new byte[1];
                    int i = 0;
                    while (i < 7)
                    {
                        socket.Receive(bytes);
                        if (bytes[0] == '\n')
                        {
                            break;
                        }
                        Dimension[i] = bytes[0];
                        i++;
                    }

                    string s = Encoding.UTF8.GetString(Dimension, 0, i);
                    int BufferSize = Int32.Parse(s);

                    byte[] Buffer = new byte[BufferSize];
                    socket.Receive(Buffer);

                    string data = Encoding.ASCII.GetString(Buffer, 0, BufferSize);
                    MyJsonObj o = JsonConvert.DeserializeObject<MyJsonObj>(data);

                    if (o.Message.msg_type.Equals(Message.process_list_str))
                    {

                        foreach (Process p in o.Message.process_list)
                        {
                            p.IsActive = true;
                            AddOnProcessList(ProcessList, p);
                            if (p.Hwnd.Equals(o.Message.hwnd))
                            {
                                //Start count time
                                Timer.Start();
                                p.SetFocus(true);
                            }
                        }
                        //Bind ProcessCollection and set focus


                        ThreadPool.QueueUserWorkItem(act =>
                        {
                            Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                            {
                                MainWindow.screenMain.SetFocus(this);
                            }));
                        });

                        Log l = new Log();
                        l.LogProcessList(DateTime.Now);
                        AddOnLogQueue(LogList, l);
                    }
                    else if (o.Message.msg_type.Equals(Message.new_process))
                    {
                        Process p = o.Message.process_list[0];
                        p.IsActive = true;
                        //ProcessList.Add(p);
                        AddOnProcessList(ProcessList, p);

                        //Add a Log
                        Log l = new Log();
                        l.LogNewProcess(DateTime.Now, o.Message.process_list[0].Name);
                        AddOnLogQueue(LogList, l);

                    }
                    else if (o.Message.msg_type.Equals(Message.process_close))
                    {
                        lock (ProcessList)
                        {
                            foreach (Process p in ProcessList)
                            {
                                if (p.Hwnd == o.Message.hwnd)
                                {
                                    //Add a Log
                                    Log l = new Log();
                                    l.LogCloseProcess(DateTime.Now, p.Name);
                                    AddOnLogQueue(LogList, l);
                                    p.Close();
                                    break;
                                }
                            }
                        }
                    }
                    else if (o.Message.msg_type.Equals(Message.focus_change))
                    {
                        lock (ProcessList)
                        {
                            foreach (Process p in ProcessList)
                            {
                                p.SetFocus(false);
                                if (p.Hwnd == o.Message.hwnd)
                                {
                                    p.SetFocus(true);
                                    //Add a Log
                                    Log l = new Log();
                                    l.LogNewFocus(DateTime.Now, p.Name);
                                    AddOnLogQueue(LogList, l);
                                }
                            }
                        }

                    }
                    else if (o.Message.msg_type.Equals(Message.connection_closed))
                    {
                        IsConnected = false;
                        MessageBoxResult result = MessageBox.Show("The server has closed the connection", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                        break;
                    }
                }
            }
            catch (Exception e)
            {
                if (IsConnected)
                {
                    IsConnected = false;
                    MessageBoxResult result = MessageBox.Show("The connection with the server has encountered some problems", "Error", MessageBoxButton.OK,MessageBoxImage.Error);
                }
            }
        }

        private void WriteListRequest()
        {
            if (IsConnected)
            {
                //Request List
                Message m = new Message();
                m.msg_type = Message.process_list_req;

                MyJsonObj Obj = new MyJsonObj(m);

                //convert message in json
                string msg = JsonConvert.SerializeObject(Obj);
                byte[] msg_byte = Encoding.ASCII.GetBytes(msg);

                string dim = msg_byte.Length + "\n";

                byte[] dim_byte = Encoding.ASCII.GetBytes(dim);

                //Single socket
                socket.Send(dim_byte);
                socket.Send(msg_byte);
            }
        }
        private void WriteCloseCommand()
        {
            if (IsConnected)
            {
                //Request List
                Message m = new Message();
                m.msg_type = Message.connection_closed;

                MyJsonObj Obj = new MyJsonObj(m);

                //convert message in json
                string msg = JsonConvert.SerializeObject(Obj);
                byte[] msg_byte = Encoding.ASCII.GetBytes(msg);

                string dim = msg_byte.Length + "\n";

                byte[] dim_byte = Encoding.ASCII.GetBytes(dim);
                
                //Single socket
                socket.Send(dim_byte);
                socket.Send(msg_byte);
            }
        }
        public void WriteCommands(List<int> list)
        {
            if (IsConnected)
            {
                //Request List
                Message m = new Message();
                m.msg_type = Message.command;
                m.commands = list;

                MyJsonObj Obj = new MyJsonObj(m);

                //convert message in json
                string msg = JsonConvert.SerializeObject(Obj);
                byte[] msg_byte = Encoding.ASCII.GetBytes(msg);

                string dim = msg_byte.Length + "\n";

                byte[] dim_byte = Encoding.ASCII.GetBytes(dim);

                socket.Send(dim_byte);
                socket.Send(msg_byte);
            }
        }

        public void UpdateConnection()
        {
            if (IsConnected)
            {
                //Get the time elapsed and convert into string
                TimeSpan ts = Timer.Elapsed;
                MainWindow.screenMain.DurationTime.Content = (StringDurationTime = Timer.Elapsed.ToString(@"hh\:mm\:ss"));

                //Update the amount of time for each process
                foreach (Process p in ProcessList)
                {
                    p.UpdateProcess(ts.Seconds + ts.Minutes * 60 + ts.Hours * 3600);
                }
            }
            
        }

        //Function to get Wifi Addresses
        private static List<string> GetWifiAddresses()
        {
            List<string> l = new List<string>();
            var interfaces = NetworkInterface.GetAllNetworkInterfaces();
            foreach (var ni in interfaces)
            {
                if (ni.NetworkInterfaceType == NetworkInterfaceType.Wireless80211)
                {
                    var adapterProperties = ni.GetIPProperties();
                    foreach (var x in adapterProperties.UnicastAddresses)
                    {
                        if (x.Address.AddressFamily == AddressFamily.InterNetwork)
                        {
                            l.Add(x.Address.ToString());
                        }
                    }
                }
            }
            return l;
        }
        //Functions to add in an observable list
        private static void AddOnProcessList<Process>(ICollection<Process> collection, Process item)
        {
          lock (collection)
            {
                Action<Process> addMethod = collection.Add;
                var op = Application.Current.Dispatcher.BeginInvoke(addMethod, item);

                while (op.Status != DispatcherOperationStatus.Completed)
                {
                    op.Wait(TimeSpan.FromMilliseconds(50));
                    Thread.Sleep(50);
                }
            }
        }
        private static void AddOnLogQueue<Log>(ICollection<Log> collection, Log item)
        {
            lock (collection)
            {
                Action<Log> addMethod = collection.Add;
                var op = Application.Current.Dispatcher.BeginInvoke(addMethod, item);
                while (op.Status != DispatcherOperationStatus.Completed)
                {
                    op.Wait(TimeSpan.FromMilliseconds(50));
                    Thread.Sleep(50);
                }
            }
        }
        //Functions to perform async connection
        private void CallWithTimeout(Action<Socket, IPEndPoint> action, int timeoutMilliseconds, Socket socket, IPEndPoint ipendPoint)
        {
            try
            {
                Action wrappedAction = () =>
                {
                    action(socket, ipendPoint);
                };

                IAsyncResult result = wrappedAction.BeginInvoke(null, null);

                if (result.AsyncWaitHandle.WaitOne(timeoutMilliseconds))
                {
                    wrappedAction.EndInvoke(result);
                }

            }
            catch (Exception ex)
            {

            }
        }
        private void ConnectToProxyServers(Socket testSocket, IPEndPoint ipEndPoint)
        {
            try
            {
                if (testSocket == null || ipEndPoint == null)
                    return;
                testSocket.Connect(ipEndPoint);
            }
            catch (Exception ex)
            {

            }
        }

    }
}
