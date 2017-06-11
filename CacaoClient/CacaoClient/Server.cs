using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.Net;
using System.Net.Sockets;

namespace CacaoClient
{
    class CacaoServer
    {
        TcpClient client;
        NetworkStream client_stream;


        public void connect_server(string ip_address)
        {
            client = new TcpClient();
            client.Connect(ip_address, 10000);
            client_stream = client.GetStream();
        }
        public string recv_data()
        {
            byte[] get_byte = new byte[1024];
            string ret_data = "";

            client_stream.Read(get_byte, 0, 1024);
            ret_data = Encoding.ASCII.GetString(get_byte);

            return ret_data;
        }

        public void send_data(string data)
        {
            byte[] data_byte = Encoding.Default.GetBytes(data);
            client_stream.Write(data_byte, 0, data.Length);
        }

        
    }
}
