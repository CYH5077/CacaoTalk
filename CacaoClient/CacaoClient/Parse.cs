using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CacaoClient
{
    class CacaoParse
    {
        public int chanel_chat(string data, ref string user_id)
        {
            string[] data_line = data.Split('\n');
            if (data_line[0].Contains("CHAT_MESSAGE"))
            {
                user_id = data_line[1];
                return 0;
            }
            else if (data_line[0].Contains("USER_LIST"))
                return 1;
            else
                return 2;
        }

        public string[] get_chanel_Data(string data)
        {
            string[] line_cut;
            string[] ret_val;

            line_cut = data.Split('\n');

            ret_val = new string[line_cut.Length - 1];
            int data_index = 0;
            for(int i = 0 ; i < line_cut.Length; i++)
            {
                if (line_cut[i] == "")
                    continue;
                ret_val[data_index++] = line_cut[i];
            }
            return ret_val;
        }
    }
}
