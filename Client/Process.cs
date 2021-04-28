using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Media;

namespace Client
{

    //Class Process 
    public class Process : INotifyPropertyChanged
    {
        public string Name { get; set; }
        public int Id { get; set; }
        public int Hwnd { get; set; }
        public Stopwatch Timer { get; set; } = new Stopwatch();
        public string FocusTime { get; set; } = "00:00:00";
        private float totalFocusTime = 0;
        public bool IsActive { get; set; }


        public bool HasFocus { get; set; } = false;
        

        //icona
        public string Percentage { get; set; } = "0 %";
        // Declare the PropertyChanged event
        public event PropertyChangedEventHandler PropertyChanged;
        
        public Process(string s,bool b)
        {
            Name = s;
            IsActive = b;
        }

        public void SetFocus(bool state)
        {
            HasFocus = state;
            OnPropertyChanged("HasFocus");
            if (state)
                Timer.Start();
            else
                Timer.Stop();
        }

        public void UpdateProcess(float totalDurationInSecond)
        {
            if (totalDurationInSecond > 0)
            {
                if (HasFocus == true)
                {
                    totalFocusTime = Timer.Elapsed.Seconds + Timer.Elapsed.Minutes * 60 + Timer.Elapsed.Hours * 3600;
                    FocusTime = Timer.Elapsed.ToString(@"hh\:mm\:ss");
                    OnPropertyChanged("FocusTime");
                }
                if (totalFocusTime > 0)
                {
                    Decimal partialPercentage = (Decimal)totalFocusTime / (Decimal)totalDurationInSecond * 100;
                    Percentage = Math.Round(partialPercentage, 1, MidpointRounding.AwayFromZero) + " %";
                    OnPropertyChanged("Percentage");
                }
            }
        }

        // Create the OnPropertyChanged method to raise the event
        protected void OnPropertyChanged(string name)
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (handler != null)
            {
                handler(this, new PropertyChangedEventArgs(name));
            }
        }
        
        public void Close()
        {
            IsActive = false;
            OnPropertyChanged("IsActive");
            Timer.Stop();
        }
    }
}