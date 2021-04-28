using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace Client
{
    public class Log
    {
        private enum LogType { NewProcess, ChangeFocus, CloseProcess, ProcessList , SendCommand, CloseSocket}
        public String Event { get; set; }
        public String Time { get; set; }
        private int Type;
        public SolidColorBrush MyColor {
            get
            {
                switch (Type)
                {
                    case (int)LogType.ProcessList:
                        return new SolidColorBrush(Colors.Black);
                    case (int)LogType.NewProcess:
                        return new SolidColorBrush(Colors.Green);
                    case (int)LogType.CloseProcess:
                        return new SolidColorBrush(Colors.Red);
                    case (int)LogType.SendCommand:
                        return new SolidColorBrush(Colors.Yellow);
                    default:
                        return new SolidColorBrush(Colors.Black);
                }

            }
        }

        // Just for local test
        public Log(int t, string s)
        {
            Type = t;
            Event = s;
        }

        public Log()
        {
        }

        public void LogNewProcess(DateTime dt, String name)
        {
            Event = "The process " + name + " is started";
            Time = dt.ToString("HH:mm:ss");
            Type = (int) LogType.NewProcess;
            
        }

        public void LogNewFocus(DateTime dt, String name)
        {
            Event = "The process " + name + " has got the focus";
            Time = dt.ToString("HH:mm:ss");
            Type = (int) LogType.ChangeFocus;

        }

        public void LogCloseProcess(DateTime dt, String name)
        {
            Event = "The process " + name + " has been closed";
            Time = dt.ToString("HH:mm:ss");
            Type = (int)LogType.CloseProcess;

        }

        public void LogProcessList(DateTime dt)
        {
            Event = "The processes list has been received";
            Time = dt.ToString("HH:mm:ss");
            Type = (int)LogType.ProcessList;
        }

        public void LogCloseSocket(DateTime dt)
        {
            Event = "The connection has been closed";
            Time = dt.ToString("HH:mm:ss");
            Type = (int)LogType.CloseSocket;
        }

        public void LogSendCommands(DateTime dt, List<int> keys )
        {
            foreach (int key in keys)
            {
                //Event +=Keyboard()
            }
            Event += " has been send";
            
            Time = dt.ToString("HH:mm:ss");
            Type = (int)LogType.SendCommand;
        }


    }
    /*
    public class LogQueue
    {
        public ObservableCollection<Log> List { get; set; } = new ObservableCollection<Log>();
        private int Size;

        public LogQueue(int i)
        {
            Size = i;
        }

        public void Add(Log l)
        {
            if (List.Count == Size)
            {
                List.RemoveAt(0);
                List.Add(l);
            }
            else
            {
                List.Add(l);
            }
        }

        public ObservableCollection<Log> Get()
        {
            return List;
        }
    }*/
}