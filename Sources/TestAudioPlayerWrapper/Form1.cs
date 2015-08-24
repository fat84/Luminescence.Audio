using System;
using System.IO;
using System.Windows.Forms;
using Luminescence.Audio;

namespace TestAudioPlayerWrapper
{
   public partial class mainForm : Form
   {
      private readonly FFmpegAudioPlayer player = new FFmpegAudioPlayer();

      public mainForm()
      {
         InitializeComponent();
         player.PlayerStopped += player_PlayerStopped;
         player.PlayerStarted += player_PlayerStarted;

         string path = @"E:\NAS\music\FLAC\Divers\Shakira - [Live from Paris #20] Je l'aime à mourir.flac";
         // @"C:\Program Files\Microsoft Office\Office15\MEDIA\DefaultHold.wma";
         if (File.Exists(path))
            txtPath.Text = path;
      }

      private void player_PlayerStopped(object sender, PathEventArgs e)
      {
         //if (lblPlaying.InvokeRequired)
         //   Invoke((Action)(() => lblPlaying.Text = null));
         //else
         lblPlaying.Text = null;
      }

      private void player_PlayerStarted(object sender, PathEventArgs e)
      {
         lblPlaying.Text = e.Path;
      }

      private void Form1_FormClosed(object sender, FormClosedEventArgs e)
      {
         player.Dispose();
      }

      private void btnOpen_Click(object sender, EventArgs e)
      {
         if (fdOpen.ShowDialog() == DialogResult.OK)
            txtPath.Text = fdOpen.FileName;
      }

      private void btnPlay_Click(object sender, EventArgs e)
      {
         try
         {
            if (File.Exists(txtPath.Text))
            {
               player.Play(txtPath.Text);
               //lblPlaying.Text = player.PlayingFile;
            }
         }
         catch (Exception ex)
         {
            MessageBox.Show(ex.Message);
         }
      }

      private void btnStop_Click(object sender, EventArgs e)
      {
         //if (player.PlayingFile != null)
         player.Stop();
      }

      private void cbxPaused_CheckedChanged(object sender, EventArgs e)
      {
         if (player.PlayingFile != null)
            player.Paused = cbxPaused.Checked;
      }

      private void cbxMuted_CheckedChanged(object sender, EventArgs e)
      {
         player.Muted = cbxMuted.Checked;
      }

      private void tbrVolume_ValueChanged(object sender, EventArgs e)
      {
         player.Volume = (float)tbrVolume.Value / 10;
         cbxMuted.Checked = false;
      }

      private void btnDispose_Click(object sender, EventArgs e)
      {
         player.Dispose();
      }
   }
}
