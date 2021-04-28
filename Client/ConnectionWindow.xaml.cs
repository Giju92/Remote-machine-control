using System;
using System.IO;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace Client
{
    public partial class ConnectionWindow : Window
    {
        Connection c = new Connection();
        private Thread t;
        private string IP = "192.168.1.102";
        
        public ConnectionWindow()
        {
            InitializeComponent();
            this.Image.Source = new BitmapImage(new Uri(Path.GetFullPath("Image/remoteicon.png")));
            NameBox.Text = "Connection " + (MainWindow.screenMain.ConnectionList.Count + 1);
        }
        public void TryConnect(object sender, RoutedEventArgs e)
        {
            ConnectionButton.IsEnabled = false;
            if (!CheckValidIP())
            {
                ChangeLabel("Not valid IP", new SolidColorBrush(Colors.Red));
                ConnectionButton.IsEnabled = true;
                return;
            }
            else
            {
                ChangeLabel("Try to connect...", new SolidColorBrush(Colors.Black));
                t = new Thread(AsyncConnection);
                t.Start();
            }
        }
        private void AsyncConnection()
        {
            if (c.StartConnection(IP))
            {
                //Add Connection and set CurrentConnection
                ThreadPool.QueueUserWorkItem(o =>
                {
                    Dispatcher.BeginInvoke(new Action(() =>
                    {
                        c.Name = NameBox.Text;
                        MainWindow.screenMain.AddNewConnection(c);
                        this.Close();
                    }));
                });
               
            }
            else
            {
                ThreadPool.QueueUserWorkItem(o =>
                {
                        Dispatcher.BeginInvoke(new Action(() =>
                        {
                            ConnectionButton.IsEnabled = true;
                            ChangeLabel("Impossible to reach the server", new SolidColorBrush(Colors.Red));
                        }));
                });
            }
        }
        private void ChangeLabel(string s, SolidColorBrush c)
        {
            ReportLabel.Foreground = c;
            ReportLabel.Content = s;
        }
        private void NumberValidationTextBox(object sender, TextCompositionEventArgs e)
        {
            TextBox tb = (TextBox)sender;

            if (e.Text.Equals("\r"))
            {
                TraversalRequest request = new TraversalRequest(FocusNavigationDirection.Next);
                request.Wrapped = true;
                ((TextBox)sender).MoveFocus(request);
            }

            string s = tb.Text + e.Text;

            Regex regex = new Regex("^[1-9]$|^[1-9][0-9]$|^1([0-9][0-9])$|^2([0-4][0-9]|5[0-5])$");

            e.Handled = !regex.IsMatch(s);
            
        }
        private void OnGotFocus(object sender, RoutedEventArgs e)
        {
            TextBox tb = (TextBox)sender;
            if(tb.Text.Equals("0"))
                tb.Text = "";
        }
        private void OnLostFocus(object sender, RoutedEventArgs e)
        {
            TextBox tb = (TextBox)sender;
            if (tb.Text.Equals(""))
                tb.Text = "0";
        }
        private void OnLostFocusName(object sender, RoutedEventArgs e)
        {
            TextBox tb = (TextBox)sender;
            if (tb.Text.Equals(""))
                tb.Text = "Connection" + MainWindow.screenMain.ConnectionList.Count + 1;
           
        }
        private void IsAllowedToConnect()
        {
            try
            {
                if (First.Text.Equals("0") | Fourth.Text.Equals("0") | First.Text.Equals("") | Fourth.Text.Equals(""))
                {
                    ConnectionButton.IsEnabled = false;
                }
                else
                {
                    ConnectionButton.IsEnabled = true;
                }

            }
            catch (Exception e)
            {
                //manage the event of null exception
                Console.WriteLine("OK LO SO" + e.ToString());
            }
        }
        private void OnTextChanged(object sender, TextChangedEventArgs e)
        {
            IsAllowedToConnect();
        }
        private bool CheckValidIP()
        {
            int Ip1 = Convert.ToInt16(First.Text);
            int Ip2 = Convert.ToInt16(Second.Text);
            int Ip3 = Convert.ToInt16(Third.Text);
            int Ip4 = Convert.ToInt16(Fourth.Text);
            //IP = Ip1 + "." + Ip2 + "." + Ip3 + "." + Ip4;
            /*
            Regex regex;
            
            //IP class A
            regex = new Regex("(10)(\\.([2][0-5][0-5]|[1][0-9][0-9]|[1-9][0-9]|[0-9])){3}");
            if(regex.IsMatch(IP))
                return true;
            //IP class B
            regex = new Regex("(172)\\.(1[6-9]|2[0-9]|3[0-1])(\\.([2][0-5][0-5]|[1][0-9][0-9]|[1-9][0-9]|[0-9])){2}");
            if(regex.IsMatch(IP))
                return true;
            //IP class C
            regex = new Regex("(192)\\.(168)(\\.([2][0-5][0-5]|[1][0-9][0-9]|[1-9][0-9]|[0-9])){2}");
            if(regex.IsMatch(IP))
                return true;
            */
            return true;
        }
    } 
}



 