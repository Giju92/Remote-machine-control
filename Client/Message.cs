using System.Collections.Generic;

namespace Client
{
    //Class Message
    public class Message
    {
        //String 
        public static string process_list_req = "PROCESS_LIST_REQ";
        public static string process_list_str = "PROCESS_LIST";
        public static string new_process = "NEW_PROCESS";
        public static string focus_change = "FOCUS_CHANGE";
        public static string process_close = "PROCESS_CLOSE";
        public static string command = "COMMAND";
        public static string connection_closed = "CONNECTION_CLOSED";
        
        public string msg_type { get; set; }
        public int pid;
        public int hwnd;
        public List<Process> process_list = new List<Process>();
        public List<int> commands = new List<int>();
    }

    //Class Json-Object
    public class MyJsonObj
    {
        public Message Message;

        public MyJsonObj(Message mes)
        {
            Message = mes;
        }
    }
}
