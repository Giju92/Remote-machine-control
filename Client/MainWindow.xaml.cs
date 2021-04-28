using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Client
{
   public partial class MainWindow : Window
    {
        public static MainWindow screenMain = null;
        
        //List connection
        public ObservableCollection<Connection> ConnectionList { get; set; } =  new ObservableCollection<Connection>();
        public Connection CurrentConnection = null;
        public bool IsEnableCapture = false;

        public MainWindow()
        {
            InitializeComponent();
            screenMain = this;  //to make this class static

            ListBoxConnection.ItemsSource = ConnectionList;

            this.MaxHeight = SystemParameters.MaximizedPrimaryScreenHeight;
            this.Height = SystemParameters.MaximizedPrimaryScreenHeight;
            this.Width = SystemParameters.MaximizedPrimaryScreenWidth;

            //To generate an event each second
            System.Windows.Threading.DispatcherTimer dispatcherTimer = new System.Windows.Threading.DispatcherTimer();
            dispatcherTimer.Tick += new EventHandler(dispatcherTimer_Tick);
            dispatcherTimer.Interval = new TimeSpan(0, 0, 1);
            dispatcherTimer.Start();
        }

        //Button events
        public void EnableDisableCapture(object sender, RoutedEventArgs e)
        {
            if (ConnectionList.Count > 0)
            {
                IsEnableCapture = !IsEnableCapture;

                if (IsEnableCapture)
                {
                    StartStopButton.Content = "STOP";

                }
                else
                    StartStopButton.Content = "START";
            }
        }
        public void NewConnectionWindow(object sender, RoutedEventArgs e)
        {
            ConnectionWindow winCon = new ConnectionWindow();
            winCon.ShowDialog();
        }

        public void AddNewConnection(Connection c)
        {
            ConnectionList.Add(c);
            //SetFocus(c);
        }

        public void SetFocus(Connection c)
        {
            if (!c.Equals(CurrentConnection))
            {
                CurrentConnection = c;

                IPHoSt.Content = c.IPHost;
                DurationTime.Content = c.StringDurationTime;
                LogTable.ItemsSource = c.LogList;

                c.ProcessCollection = new ListCollectionView(c.ProcessList);
                c.ProcessCollection.GroupDescriptions.Add(new PropertyGroupDescription("IsActive"));
                ProcessTable.ItemsSource = c.ProcessCollection;
                
                ((INotifyCollectionChanged)LogTable.Items).CollectionChanged += LogTableChanged;
            }
        }

        //Get and Send commands
        //Import for use GetKeyBoardState
        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool GetKeyboardState(byte[] lpKeyState);
        private void Window_KeyDown(object sender, KeyEventArgs e)
        {
            if (IsEnableCapture)
            {
                List<int> keys = new List<int>();
                Byte[] array = new Byte[256];
                GetKeyboardState(array);
                
                for (int i = 0; i < 255; i++)
                {
                    if ((array[i] & 0x80) != 0)
                    {
                        keys.Add(i);
                    }
                }
                CurrentConnection.WriteCommands(keys);
                e.Handled = true;
            }
        }
        //Event tick
        private void dispatcherTimer_Tick(object sender, EventArgs e)
        {
            if (CurrentConnection != null)
            {
                CurrentConnection.UpdateConnection();
            }
        }

        private void OnClick(object sender, RoutedEventArgs e)
        {
            
        }

        private void LogTableChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            if (LogTable.Items.Count > 0)
            {
                var border = VisualTreeHelper.GetChild(LogTable, 0) as Decorator;
                if (border != null)
                {
                    var scroll = border.Child as ScrollViewer;
                    if (scroll != null) scroll.ScrollToEnd();
                }
            }
        }

        private void CloseConnection(object sender, RoutedEventArgs e)
        {
            CurrentConnection.CloseConnection();
        }
    }
}
