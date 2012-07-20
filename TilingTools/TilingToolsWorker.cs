#region Using
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.IO;
#endregion

namespace TilingTools
{
    public delegate void ResponseHandler(object sender, string response);
    public class TilingToolsWorker
    {
        private Process Proc = null;

        public event ResponseHandler OnResponseReceived;
        public event EventHandler OnProcessComplete;
        public event EventHandler OnProcessStart;

        #region Конструктор
        /// <summary>
        /// Новый процесс
        /// </summary>
        /// <param name="exefile">Путь к файлу</param>
        /// <param name="args">Аргументы</param>
        public TilingToolsWorker(string[] exefile, string[] args)
        {
            ExeFile = exefile;
            Args = args;
        }
        #endregion

        #region Свойства
        public string[] ExeFile { get; set; }
        public string[] Args { get; set; }
        #endregion
        public void KillProcess()
        {
            if (Proc!=null) Proc.Kill();
        }

        public void StartProcess()
        {
            OnProcessStart(this, EventArgs.Empty);
            for (int i = 0; i < ExeFile.Length; i++)
            {
                Proc = null;
                ProcessStartInfo processInfo;
                //Process process;

                processInfo = new ProcessStartInfo(ExeFile[i], Args[i]);

                //processInfo.StandardOutputEncoding = Encoding.Unicode;

                processInfo.RedirectStandardOutput = true;
                processInfo.UseShellExecute = false;
                processInfo.CreateNoWindow = true;
                //processInfo.UseShellExecute = true;
                //processInfo.CreateNoWindow = false;

                Proc = Process.Start(processInfo);

                StreamReader sr = Proc.StandardOutput;

                while (!sr.EndOfStream)
                {
                    if (OnResponseReceived != null)
                        OnResponseReceived(this, ((char)sr.Read()).ToString());
                }
                OnResponseReceived(this,System.Environment.NewLine);
            }
            if (OnProcessComplete != null)
                OnProcessComplete(this, EventArgs.Empty);
        }
    }
}
