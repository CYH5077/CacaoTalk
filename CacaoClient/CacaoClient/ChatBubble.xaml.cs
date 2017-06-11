using System;
using System.Collections.Generic;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace CacaoClient
{
	/// <summary>
	/// ChatBubble.xaml에 대한 상호 작용 논리
	/// </summary>
	public partial class ChatBubble : UserControl
	{
		public ChatBubble()
		{
			this.InitializeComponent();
		}
		
		public ChatBubble(string id, string text, bool me)
		{
			this.InitializeComponent();
		
			double now_width = text_box.Width;
			double now_height = text_box.Height;
			double label_width = user_id.Width;
			
			text_box.Text = text;
			user_id.Content = id;
			double size_val = text_box.Width - now_width;
			double size_label = user_id.Width - label_width;
			
			this.Height += size_val / 2;
			this.Width += size_val /2;
			
			if(me)
			{
				text_rec.Fill = new SolidColorBrush(System.Windows.Media.Colors.Yellow); 
				text_try.Fill = new SolidColorBrush(System.Windows.Media.Colors.Yellow); 
			}
		}
	}
}