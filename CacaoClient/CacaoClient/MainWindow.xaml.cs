using System;
using System.Collections.Generic;
using System.Linq;
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

using CacaoClient;
using System.Data;
using System.ComponentModel;
using System.Windows.Threading;

namespace CacaoClient
{
	//chanel_list 추가 Item.
	public class Chanel_Item{
		public string Chanel { get; set; }
		public string User {get; set;}
	}
	
	//chat_list 추가 Item
	public class Chat_Item{
		public ChatBubble Recv { get; set;}
		public ChatBubble Send { get; set;}
	}
	
    /// <summary>
    /// MainWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class MainWindow : Window
    {
        CacaoServer client = new CacaoServer();
        BackgroundWorker login_recver = new BackgroundWorker();
        BackgroundWorker chanel_recver = new BackgroundWorker();
        string my_id;

        public MainWindow()
        {
            InitializeComponent();
			this.MouseLeftButtonDown += new MouseButtonEventHandler(Window_Move);


            login_recver.DoWork += login_recver_worker;
            chanel_recver.DoWork += chanel_recver_worker;

        }


        public void login_recver_worker(object sender, DoWorkEventArgs e)
        {
            string buffer = "";
            string[] chanel_list_array;
            string[] chanel_cut;
            CacaoParse parse = new CacaoParse();

            
            client.send_data("CHANEL_INFO");
            while (true)
            {
                buffer = client.recv_data();
                if (buffer.Contains("JOIN"))
                    break;
                else if (buffer.Contains("LOGIN ERROR")) {
                    MessageBox.Show("로그인 실패");
                    continue;
                } else if (buffer.Contains("USER_ADD_ERROR")) {
                    MessageBox.Show("가입 실패 이미 있는 계정일 수 있습니다.");
                    continue;
                } else if (buffer.Contains("ADD_SUCCESS")) {
                    MessageBox.Show("가입 성공");
                    continue;
                }else if (buffer.Contains("USER_DEL_ERROR")){
                    MessageBox.Show("제거 실패");
                    continue;
                }else if (buffer.Contains("DEL_SUCCESS")) {
                    MessageBox.Show("제거 성공");
                    continue;
                } else if (buffer.Contains("USER_NOW_LOGIN")){
                    MessageBox.Show("해당 계정은 이미 선택하신 채널에 접속중입니다.");
                    continue;
                }

                chanel_list_array = parse.get_chanel_Data(buffer);

                for(int i = 0; i < chanel_list_array.Length; i++)
                {
                    chanel_cut = chanel_list_array[i].Split(':');

                    Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                    {
                        chanel_list.Items.Add(new Chanel_Item{Chanel = chanel_cut[0] + "채널",
                                                              User = chanel_cut[1] + "/" + chanel_cut[2]});
                    }));
                }
            }

            Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
            {
                Login_grid.Visibility = System.Windows.Visibility.Hidden;

                Chat_grid.Visibility = System.Windows.Visibility.Visible;
            }));

            client.send_data("CHANEL_JOIN");
            chanel_recver.RunWorkerAsync();
        }


        public void chanel_recver_worker(object sender, DoWorkEventArgs e)
        {
            string[] data_cut;

            byte[] data_byte = new byte[1024];
            string data = "";
            string user_id = "";

            CacaoParse parse = new CacaoParse();
            int data_type = 0; // 0-채팅, 1-유저리스트, 2-새로운 유저 접속.
            bool my_chat = false; //color

            client.send_data("CHANEL_USER");
            while(true)
            {

                data = "";
                data = client.recv_data();
                data_type = parse.chanel_chat(data, ref user_id);
                if(data_type == 0) //채팅.출력
                {
                    data_cut = data.Split('\n');
                    if (user_id == my_id)
                        my_chat = true;
                    else
                        my_chat = false;
                    Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                    {
                        chat_list.Items.Add(new ChatBubble(user_id, 
                            data_cut[2].Substring(0, int.Parse(data_cut[3])), my_chat));
                    }));
                }
                else if (data_type == 1) //유저 리스트 받아옴.
                {
                    data_cut = data.Split('\n');
                    for (int i = 1; i < data_cut.Length; i++)
                    {
                        if (data_cut[i] == "") continue;
                        Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                        {
                            user_list.Items.Add(data_cut[i]);
                        }));
                    }
                }
                else //새로운 유저 접속 데이터
                {
                    data_cut = data.Split('\n');
                    if (data_cut[2].Contains("CONNECT"))
                    {
                        Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                        {
                            if (data_cut[1] != my_id)
                                user_list.Items.Add(data_cut[1]);
                            chat_list.Items.Add(new ChatBubble("SERVER", data_cut[1] + " 님께서 접속하셨습니다.", true));
                        }));
                    }
                    else
                    {
                        Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                        {
                            find_del_user(data_cut[1]);
                        }));
                    }
                }
				//오토 스크롤
				Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                {
					chat_list.ScrollIntoView(chat_list.Items[chat_list.Items.Count -1]);
                }));
            }
        }
		private void find_del_user(string target)
		{
			 for (int i = 0; i < user_list.Items.Count ;i++)
			{
				if(user_list.Items[i].ToString() == target)
					user_list.Items.RemoveAt(i);
			}
			
		}
		
		private void Window_Move(object sender, MouseButtonEventArgs e)
		{
			this.DragMove();
		}
		
        private void close_btn_Click(object sender, System.Windows.RoutedEventArgs e)
        {
        	// TODO: 여기에 구현된 이벤트 처리기를 추가하십시오.
			this.Close();
        }

        private void id_box_GotFocus(object sender, System.Windows.RoutedEventArgs e)
        {
        	// TODO: 여기에 구현된 이벤트 처리기를 추가하십시오.
			id_box.Text = "";
        }

        private void pw_box_GotFocus(object sender, System.Windows.RoutedEventArgs e)
        {
        	// TODO: 여기에 구현된 이벤트 처리기를 추가하십시오.
			pw_box.Password = "";
        }

        private void Login_btn_Click(object sender, System.Windows.RoutedEventArgs e)
        {
        	// TODO: 여기에 구현된 이벤트 처리기를 추가하십시오.
            string data = "USER_LOGIN\n" +
                            id_box.Text + "\n" +
                            pw_box.Password.ToString() + "\n";

            Chanel_Item item = chanel_list.SelectedItem as Chanel_Item;
            data += item.Chanel.Substring(0, 1);
            my_id = id_box.Text;
            client.send_data(data);
        }

        private void chat_send_btn_Click(object sender, System.Windows.RoutedEventArgs e)
        {
            string data = "CHAT_MESSAGE\n" + my_id + "\n" + chat_box.Text + "\n" + chat_box.Text.Length.ToString();
            client.send_data(data);
			chat_box.Text = "";
        }

        private void chat_box_KeyUp(object sender, System.Windows.Input.KeyEventArgs e)
        {
			if(e.Key == System.Windows.Input.Key.Enter)
			{
				string data = "CHAT_MESSAGE\n" + my_id + "\n" + chat_box.Text + "\n" + chat_box.Text.Length.ToString();
            	client.send_data(data);
				chat_box.Text = "";
			}
        }
		
        private void Window_Initialized(object sender, EventArgs e)
        {
            try
            {
                client.connect_server("192.168.219.105");

                login_recver.RunWorkerAsync();
            }
            catch(Exception error)
            {
                MessageBox.Show(error.Message);
                this.Close();
            }
        }

        private void register_btn_Click(object sender, RoutedEventArgs e)
        {
            string data = "USER_ADD\n" + id_box.Text + "\n" + pw_box.Password + "\n0";

            client.send_data(data);
        }

        private void del_btn_Click(object sender, RoutedEventArgs e)
        {
            string data = "USER_DEL\n" + id_box.Text + "\n0\n0";

            client.send_data(data);
        }


    }
}
