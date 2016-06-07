using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Detection_Of_Glass
{
    public partial class BOBO_Result : Form
    {
        public BOBO_Result()
        {
            InitializeComponent();
            pictureBox1.SizeMode = PictureBoxSizeMode.StretchImage;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            Image tmp = Image.FromFile("Contours.jpg");
            pictureBox1.Image = tmp;
        }

        private void BOBO_Result_Load(object sender, EventArgs e)
        {

        }

        private void pictureBox1_Click(object sender, EventArgs e)
        {

        }
    }
}
