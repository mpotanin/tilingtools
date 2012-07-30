using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Threading;
using System.IO;


namespace TilingTools
{
    public partial class Form1 : Form
    {
        private TilingToolsWorker W;

        private string addDoubleQuotes(string str)
        {
            if (str == "") return str;
            return (str[0] == '\"') ? str : "\"" + str + "\"";
        }

        public Form1()
        {
            InitializeComponent();

            comboBox3.SelectedIndex = 0;
            comboBox2.SelectedIndex = 0;
            comboBox4.SelectedIndex = 0;
        }

        /*
        private void button1_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            //ofd.Filter = "(*.tif) Изображение|*.tif|(*.tiff) Изображение|*.tiff|(*.jpg) Изображение|*.jpg|(*.*) Все файлы|*.*";
            ofd.Filter = "Image files(*.tif,*.img,*.jpg,*.png,*.bmp,*.gif,*.ecw)|*.tif;*.img;*.jpg;*.png;*.bmp;*.gif;*.ecw|All files(*.*)|*.*";//|(*.tiff) Изображение|*.tiff|(*.jpg) Изображение|*.jpg|(*.*) Все файлы|*.*";
           // (*.BMP;*.JPG;*.GIF)|*.BMP;*.JPG;*.GIF|All files (*.*)|*.*

            if (ofd.ShowDialog() == DialogResult.OK)
            {
                textBox1.Text = ofd.FileName;
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                textBox2.Text = ofd.FileName;
            }
        }
        

        private void button5_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog ofd = new FolderBrowserDialog();
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                textBox5.Text = ofd.SelectedPath;
            }
        }
        */

        private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
        {

        }

        /*
        private void button16_Click(object sender, EventArgs e)
        {
            if (textBox1.Text == "")
            {
                MessageBox.Show("You must specify input file");
                return;
            }
            string strWarp2Merc = " -image " + textBox1.Text;
            if (textBox2.Text != "") strWarp2Merc += " -proj " + textBox2.Text;
            if (textBox5.Text != "") strWarp2Merc += " -image_out " + textBox5.Text;
            if (comboBox1.SelectedItem != null) strWarp2Merc += " -zoom " + comboBox1.SelectedItem.ToString();
            
            TilingToolsWorker w = new TilingToolsWorker("Warp2Merc", strWarp2Merc);
            w.OnResponseReceived += new ResponseHandler(w_OnResponseReceived);
            w.OnProcessComplete += new EventHandler(w_OnProcessComplete);
            Thread thr = new Thread(new ThreadStart(w.StartProcess));
            //tabControl1.Enabled = false;
            thr.Start();
        }
        */

        //private bool bWarpedDone = false;

        void w_OnProcessStart(object sender, EventArgs e)
        {
            //MessageBox.Show("DONE!");
            //bWarpedDone = true;
            textBox23.Invoke(new MethodInvoker(delegate
            {
                textBox23.Text="";
            }));

            tabControl1.Invoke(new MethodInvoker(delegate
            {
                tabControl1.Enabled = false;
            }));
        }


        void w_OnProcessComplete(object sender, EventArgs e)
        {
            //MessageBox.Show("DONE!");
            //bWarpedDone = true;
            tabControl1.Invoke(new MethodInvoker(delegate
            {
                tabControl1.Enabled = true;
            }));
        }

        void w_OnResponseReceived(object sender, string response)
        {
            textBox23.Invoke(new MethodInvoker(delegate
            {
                textBox23.Text += response;
            }));
        }

        private bool readZoomLevelsFromText (string strText, out int minZoom, out int maxZoom)
        {
            minZoom = 0; maxZoom = 0;
            try
            {

                if (strText.LastIndexOf('-') < 0)
                {
                    maxZoom = int.Parse(strText);
                    return true;
                }
                else
                {
                    string[] strZooms = strText.Split('-');
                    minZoom = (strZooms[0] == "") ? 1 : int.Parse(strZooms[0]);
                    maxZoom = (strZooms[1] == "") ? 0 : int.Parse(strZooms[1]);
                }
            }
            catch (System.FormatException e)
            {
                return false;
            }
            return true;
        }

        private string findVectorFileForImage(string inputImage)
        {
            string baseName = Path.Combine(Path.GetFullPath(inputImage), Path.GetFileNameWithoutExtension(inputImage));
            if (File.Exists(baseName + ".shp")) return baseName + ".shp";
            else if (File.Exists(baseName + ".mif")) return baseName + ".mif";
            else if (File.Exists(baseName + ".tab")) return baseName + ".tab";

            return "";
        }

        /*
        public bool makeWarpingAndTiling(bool useFolder, bool useContainer, string input, string type, string vectorFile, string outputFolder, int minZoom, int maxZoom)
        {
                string[]    inputImages;
                string[]    inputVectors;
                string[]    warpedImages;

                W = null;
                if (useFolder)
                {
                    DirectoryInfo di = new DirectoryInfo(input);
                    type = (type == "") ? "tif" : type;
                    FileInfo[] rgFiles = di.GetFiles("*." + type);
                    inputImages = new string[rgFiles.Length];
                    inputVectors = new string[rgFiles.Length];
                    for (int i=0; i<inputImages.Length;i++)
                    {
                        inputImages[i] = rgFiles[i].FullName;
                        inputVectors[i] = findVectorFileForImage(inputImages[i]);
                    }
                }
                else
                {
                    inputImages = new string[1];
                    inputVectors = new string[1];
                    inputImages[0] = input;
                    inputVectors[0] = (vectorFile=="") ? findVectorFileForImage(inputImages[0]) : vectorFile;
                }

                string[] exeFile = new string[2 * inputImages.Length];
                string[] args = new string[2 * inputImages.Length];
                warpedImages = new string[inputImages.Length];

                for (int i = 0; i < inputImages.Length; i++)
                {
                    exeFile[2*i] = "Warp2Merc";
                    args[2*i] = "-image " + addDoubleQuotes(inputImages[i]);
                    if (maxZoom > 0) args[2*i] += " -zoom " + maxZoom.ToString();
                    warpedImages[i] = Path.Combine(Path.GetDirectoryName(inputImages[i]), Path.GetFileNameWithoutExtension(inputImages[i]) + "_merc.img");

                    exeFile[2*i+1] = "ImageTiling";
                    args[2*i+1] = "-file " +addDoubleQuotes(warpedImages[i]);
                    if (maxZoom > 0 && minZoom > 0) args[2 * i + 1] += " -zooms " + (maxZoom - minZoom + 1).ToString();
                    else if ((maxZoom == 0) && (minZoom > 0)) args[2 * i + 1] += " -minZoom " + minZoom.ToString();
                   
                    //if (outputFolder != "") args[2*i+1] += " -tiles " + addDoubleQuotes(outputFolder);
                    if (useContainer) args[2*i + 1] += " -container ";

                    if (outputFolder != "")
                    {
                        args[2 * i + 1] += (useContainer) ? (" -tiles " + addDoubleQuotes(Path.Combine(outputFolder, Path.GetFileNameWithoutExtension(warpedImages[i]) + ".tiles")))
                                                    : (" -tiles " + addDoubleQuotes(outputFolder));
                    }

                    if (inputVectors[i] != "") args[2*i+1] += " -border " + addDoubleQuotes(inputVectors[i]);
                }

                TilingToolsWorker w = new TilingToolsWorker(exeFile, args);
                w.OnProcessStart += new EventHandler(w_OnProcessStart);
                w.OnResponseReceived += new ResponseHandler(w_OnResponseReceived);
                w.OnProcessComplete += new EventHandler(w_OnProcessComplete);
                Thread thr = new Thread(new ThreadStart(w.StartProcess));
                thr.Start();
                
                return true;
        }
        */

        public bool makeImageTiling(bool bFolder, bool useContainer, string strInput, bool bBundle, string strType, string strVectorBorder, string strOutputFolder, int minZoom, int maxZoom)
        {
            W = null;
            string strTiling = " -file " + addDoubleQuotes(strInput);
            if (bBundle) strTiling += " -bundle ";
            if (strType != "") strTiling += " -type " + strType;
            if ((minZoom > 0) && (maxZoom > 0)) strTiling += " -zoom " + (maxZoom).ToString();
            else if ((maxZoom == 0) && (minZoom > 0)) strTiling += " -minZoom " + minZoom.ToString();


            if (useContainer) strTiling += " -container ";
            if (strVectorBorder != "") strTiling += " -border " + addDoubleQuotes(strVectorBorder);
            if (strOutputFolder != "")
            {
                strTiling += (useContainer) ? (" -tiles " + addDoubleQuotes(Path.Combine(strOutputFolder,Path.GetFileNameWithoutExtension(strInput)+".tiles"))) 
                                            : (" -tiles " + addDoubleQuotes(strOutputFolder));
            }

            string[] exeFile = { "ImageTiling" };
            string[] args = {strTiling};
            TilingToolsWorker w = new TilingToolsWorker(exeFile, args);
            w.OnProcessStart += new EventHandler(w_OnProcessStart);
            w.OnResponseReceived += new ResponseHandler(w_OnResponseReceived);
            w.OnProcessComplete += new EventHandler(w_OnProcessComplete);
            Thread thr = new Thread(new ThreadStart(w.StartProcess));
            //tabControl1.Enabled = false;
            thr.Start();
            return true;
        }

       
     
        private void button19_Click(object sender, EventArgs e)
        {
            int         minZoom = 0, maxZoom = 0;
            string      input = textBox6.Text;
            bool        useFolder = radioButton2.Checked;
            bool        useBundle = checkBox1.Checked;
            string      type = comboBox3.Text;
            string      vectorFile = textBox3.Text;
            string      outputFolder = textBox4.Text;
            string      strZooms = textBox7.Text;
            bool        useContainer = radioButton3.Checked;  

            if (input == "")
            {
                MessageBox.Show("You must specify input file or folder");
                return;
            }

            if (strZooms != "")
            {
                if (!readZoomLevelsFromText(strZooms, out minZoom, out maxZoom))
                {
                    MessageBox.Show("Bad zoom levels input (e.g.: 1-17 or -13)");
                    return;
                }
            }

          
            makeImageTiling(useFolder,useContainer, input, useBundle, type, vectorFile, outputFolder,minZoom, maxZoom);
        }

        private void button6_Click(object sender, EventArgs e)
        {
            if (radioButton1.Checked)
            {
                OpenFileDialog ofd = new OpenFileDialog();
                ofd.Filter = "Image files(*.tif,*.img,*.jpg,*.png,*.bmp,*.gif,*.ecw)|*.tif;*.img;*.jpg;*.png;*.bmp;*.gif;*.ecw|All files(*.*)|*.*";//|(*.tiff) Изображение|*.tiff|(*.jpg) Изображение|*.jpg|(*.*) Все файлы|*.*";
                if (ofd.ShowDialog() == DialogResult.OK)
                {
                    textBox6.Text = ofd.FileName;
                }
            }
            else
            {
                FolderBrowserDialog ofd = new FolderBrowserDialog();
                if (ofd.ShowDialog() == DialogResult.OK)
                {
                    textBox6.Text = ofd.SelectedPath;
                }
            }
            //System.Windows.Forms.o
            //ofd.Filter = "(*.tif) Изображение|*.tif|(*.tiff) Изображение|*.tiff|(*.jpg) Изображение|*.jpg|(*.*) Все файлы|*.*";
        }

        private void radioButton1_CheckedChanged(object sender, EventArgs e)
        {
            radioButton2.Checked = !radioButton1.Checked;
            label10.Enabled = radioButton2.Checked;
            comboBox3.Enabled = radioButton2.Checked;
            checkBox1.Enabled = radioButton2.Checked;
        }

        private void radioButton2_CheckedChanged(object sender, EventArgs e)
        {
            radioButton1.Checked = !radioButton2.Checked;
            label10.Enabled = radioButton2.Checked;
            comboBox3.Enabled = radioButton2.Checked;
            checkBox1.Enabled = false;
            //checkBox1.Enabled = radioButton2.Checked;

        }

        private void button3_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "Vector files(*.tab,*.mif,*.shp)|*.tab;*.mif;*.shp";//|(*.tiff) Изображение|*.tiff|(*.jpg) Изображение|*.jpg|(*.*) Все файлы|*.*";
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                textBox3.Text = ofd.FileName;
            }
        }

        private void button4_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog ofd = new FolderBrowserDialog();
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                textBox4.Text = ofd.SelectedPath;
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            int minZoom = 0, maxZoom = 0;
            string inputFolder = textBox11.Text;
            string outputFolder = textBox9.Text;
            string vectorFile = textBox12.Text;

            bool remove = checkBox2.Checked;
            string type = comboBox2.Text;
            string strZooms = textBox8.Text;
        
            if (inputFolder == "")
            {
                MessageBox.Show("You must specify \"from\" folder");
                return;
            }

            if (outputFolder == "")
            {
                MessageBox.Show("You must specify \"to\" folder");
                return;
            }

            if (vectorFile == "")
            {
                MessageBox.Show("You must specify vector border file");
                return;
            }

            if (strZooms != "")
            {
                if (!readZoomLevelsFromText(strZooms, out minZoom, out maxZoom))
                {
                    MessageBox.Show("Bad zoom levels input (e.g.: 1-17, 9-13)");
                    return;
                }
            }
            else
            {
                MessageBox.Show("You must specify min-max zooms");
                return;
            }


            string strCoping = "-from " + addDoubleQuotes(inputFolder) + " -to " +addDoubleQuotes(outputFolder);
            strCoping+= " -zooms " + minZoom.ToString() + "-" + maxZoom.ToString();
            strCoping += " -border " + addDoubleQuotes(vectorFile);
            strCoping += " -type " + type;
            if (remove) strCoping += " -delete";
            string[] exeFile = { "CopyTiles" };
            string[] args = { strCoping };
            TilingToolsWorker w = new TilingToolsWorker(exeFile, args);
            w.OnProcessStart += new EventHandler(w_OnProcessStart);
            w.OnResponseReceived += new ResponseHandler(w_OnResponseReceived);
            w.OnProcessComplete += new EventHandler(w_OnProcessComplete);
            Thread thr = new Thread(new ThreadStart(w.StartProcess));
            //tabControl1.Enabled = false;
            thr.Start();
            return;
        }

        private void label6_Click(object sender, EventArgs e)
        {

        }

        private void button9_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog ofd = new FolderBrowserDialog();
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                textBox11.Text = ofd.SelectedPath;
            }
        }

        private void button7_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog ofd = new FolderBrowserDialog();
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                textBox9.Text = ofd.SelectedPath;
            }
        }

        private void button10_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "Vector files(*.tab,*.mif,*.shp)|*.tab;*.mif;*.shp";//|(*.tiff) Изображение|*.tiff|(*.jpg) Изображение|*.jpg|(*.*) Все файлы|*.*";
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                textBox12.Text = ofd.FileName;
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            int zoom = 0;
            string inputFolder = textBox20.Text;
            string outputFile = textBox19.Text;
            string vectorFile = textBox15.Text;
            string type = comboBox4.Text;
            string strZoom = comboBox5.Text;

            if (inputFolder == "")
            {
                MessageBox.Show("You must specify tiles folder");
                return;
            }

            if (outputFile == "")
            {
                MessageBox.Show("You must specify output file name");
                return;
            }

            if (vectorFile == "")
            {
                MessageBox.Show("You must specify vector border file");
                return;
            }

            if (strZoom != "")
            {
                zoom = int.Parse(strZoom);
            }
            else
            {
                MessageBox.Show("You must specify zoom level");
                return;
            }
            ///*
            string strCombine = "-tiles " + addDoubleQuotes(inputFolder) + " -file " + addDoubleQuotes(outputFile);
            strCombine += " -zoom " + zoom.ToString();
            strCombine += " -border " + addDoubleQuotes(vectorFile);
            strCombine += " -type " + type;
            strCombine += " -kml -tab -map -xml";
            string[] exeFile = { "ImageBuilder" };
            string[] args = { strCombine };
            TilingToolsWorker w = new TilingToolsWorker(exeFile, args);
            w.OnProcessStart += new EventHandler(w_OnProcessStart);
            w.OnResponseReceived += new ResponseHandler(w_OnResponseReceived);
            w.OnProcessComplete += new EventHandler(w_OnProcessComplete);
            Thread thr = new Thread(new ThreadStart(w.StartProcess));
            //tabControl1.Enabled = false;
            thr.Start();
            
            //*/
            return;
        }

        private void button15_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog ofd = new FolderBrowserDialog();
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                textBox20.Text = ofd.SelectedPath;
            }
        }

        private void button12_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "Vector files(*.tab,*.mif,*.shp)|*.tab;*.mif;*.shp";//|(*.tiff) Изображение|*.tiff|(*.jpg) Изображение|*.jpg|(*.*) Все файлы|*.*";
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                textBox15.Text = ofd.FileName;
            }
        }

        private void button14_Click(object sender, EventArgs e)
        {
            SaveFileDialog ofd = new SaveFileDialog();
            ofd.Filter = "Image files(*.jpg,*.png)|*.jpg;*.png|All files(*.*)|*.*";//|(*.tiff) Изображение|*.tiff|(*.jpg) Изображение|*.jpg|(*.*) Все файлы|*.*";
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                textBox19.Text = ofd.FileName;
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            tabControl1.TabPages[1].Enabled = false;
            tabControl1.TabPages[2].Enabled = false;

        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            //MessageBox.Show("done");
            //if (W != null) W.KillProcess(); 

            //Thr.
            //MessageBox.Show("done");
        }

        

      

       
        
    }
}
