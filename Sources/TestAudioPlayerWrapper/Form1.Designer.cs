namespace TestAudioPlayerWrapper
{
   partial class mainForm
   {
      /// <summary>
      /// Variable nécessaire au concepteur.
      /// </summary>
      private System.ComponentModel.IContainer components = null;

      /// <summary>
      /// Nettoyage des ressources utilisées.
      /// </summary>
      /// <param name="disposing">true si les ressources managées doivent être supprimées ; sinon, false.</param>
      protected override void Dispose(bool disposing)
      {
         if (disposing && (components != null))
         {
            components.Dispose();
         }
         base.Dispose(disposing);
      }

      #region Code généré par le Concepteur Windows Form

      /// <summary>
      /// Méthode requise pour la prise en charge du concepteur - ne modifiez pas
      /// le contenu de cette méthode avec l'éditeur de code.
      /// </summary>
      private void InitializeComponent()
      {
         this.txtPath = new System.Windows.Forms.TextBox();
         this.btnOpen = new System.Windows.Forms.Button();
         this.fdOpen = new System.Windows.Forms.OpenFileDialog();
         this.label1 = new System.Windows.Forms.Label();
         this.lblPlaying = new System.Windows.Forms.Label();
         this.btnPlay = new System.Windows.Forms.Button();
         this.btnStop = new System.Windows.Forms.Button();
         this.cbxPaused = new System.Windows.Forms.CheckBox();
         this.cbxMuted = new System.Windows.Forms.CheckBox();
         this.tbrVolume = new System.Windows.Forms.TrackBar();
         this.label2 = new System.Windows.Forms.Label();
         this.btnDispose = new System.Windows.Forms.Button();
         ((System.ComponentModel.ISupportInitialize)(this.tbrVolume)).BeginInit();
         this.SuspendLayout();
         // 
         // txtPath
         // 
         this.txtPath.Location = new System.Drawing.Point(12, 12);
         this.txtPath.Name = "txtPath";
         this.txtPath.Size = new System.Drawing.Size(496, 20);
         this.txtPath.TabIndex = 0;
         // 
         // btnOpen
         // 
         this.btnOpen.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.btnOpen.Location = new System.Drawing.Point(522, 10);
         this.btnOpen.Name = "btnOpen";
         this.btnOpen.Size = new System.Drawing.Size(75, 23);
         this.btnOpen.TabIndex = 1;
         this.btnOpen.Text = "Open";
         this.btnOpen.UseVisualStyleBackColor = true;
         this.btnOpen.Click += new System.EventHandler(this.btnOpen_Click);
         // 
         // label1
         // 
         this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(9, 128);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(60, 13);
         this.label1.TabIndex = 2;
         this.label1.Text = "Playing file:";
         // 
         // lblPlaying
         // 
         this.lblPlaying.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.lblPlaying.AutoSize = true;
         this.lblPlaying.Location = new System.Drawing.Point(75, 128);
         this.lblPlaying.Name = "lblPlaying";
         this.lblPlaying.Size = new System.Drawing.Size(0, 13);
         this.lblPlaying.TabIndex = 3;
         // 
         // btnPlay
         // 
         this.btnPlay.Location = new System.Drawing.Point(12, 38);
         this.btnPlay.Name = "btnPlay";
         this.btnPlay.Size = new System.Drawing.Size(75, 23);
         this.btnPlay.TabIndex = 4;
         this.btnPlay.Text = "Play";
         this.btnPlay.UseVisualStyleBackColor = true;
         this.btnPlay.Click += new System.EventHandler(this.btnPlay_Click);
         // 
         // btnStop
         // 
         this.btnStop.Location = new System.Drawing.Point(161, 38);
         this.btnStop.Name = "btnStop";
         this.btnStop.Size = new System.Drawing.Size(75, 23);
         this.btnStop.TabIndex = 5;
         this.btnStop.Text = "Stop";
         this.btnStop.UseVisualStyleBackColor = true;
         this.btnStop.Click += new System.EventHandler(this.btnStop_Click);
         // 
         // cbxPaused
         // 
         this.cbxPaused.AutoSize = true;
         this.cbxPaused.Location = new System.Drawing.Point(93, 42);
         this.cbxPaused.Name = "cbxPaused";
         this.cbxPaused.Size = new System.Drawing.Size(62, 17);
         this.cbxPaused.TabIndex = 6;
         this.cbxPaused.Text = "Paused";
         this.cbxPaused.UseVisualStyleBackColor = true;
         this.cbxPaused.CheckedChanged += new System.EventHandler(this.cbxPaused_CheckedChanged);
         // 
         // cbxMuted
         // 
         this.cbxMuted.AutoSize = true;
         this.cbxMuted.Location = new System.Drawing.Point(12, 81);
         this.cbxMuted.Name = "cbxMuted";
         this.cbxMuted.Size = new System.Drawing.Size(56, 17);
         this.cbxMuted.TabIndex = 7;
         this.cbxMuted.Text = "Muted";
         this.cbxMuted.UseVisualStyleBackColor = true;
         this.cbxMuted.CheckedChanged += new System.EventHandler(this.cbxMuted_CheckedChanged);
         // 
         // tbrVolume
         // 
         this.tbrVolume.Location = new System.Drawing.Point(137, 72);
         this.tbrVolume.Maximum = 20;
         this.tbrVolume.Name = "tbrVolume";
         this.tbrVolume.Size = new System.Drawing.Size(302, 45);
         this.tbrVolume.TabIndex = 8;
         this.tbrVolume.Value = 10;
         this.tbrVolume.ValueChanged += new System.EventHandler(this.tbrVolume_ValueChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(86, 82);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(45, 13);
         this.label2.TabIndex = 9;
         this.label2.Text = "Volume:";
         // 
         // btnDispose
         // 
         this.btnDispose.Location = new System.Drawing.Point(242, 38);
         this.btnDispose.Name = "btnDispose";
         this.btnDispose.Size = new System.Drawing.Size(75, 23);
         this.btnDispose.TabIndex = 10;
         this.btnDispose.Text = "Dispose!";
         this.btnDispose.UseVisualStyleBackColor = true;
         this.btnDispose.Click += new System.EventHandler(this.btnDispose_Click);
         // 
         // mainForm
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(609, 150);
         this.Controls.Add(this.btnDispose);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.tbrVolume);
         this.Controls.Add(this.cbxMuted);
         this.Controls.Add(this.cbxPaused);
         this.Controls.Add(this.btnStop);
         this.Controls.Add(this.btnPlay);
         this.Controls.Add(this.lblPlaying);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.btnOpen);
         this.Controls.Add(this.txtPath);
         this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
         this.Name = "mainForm";
         this.Text = "Audio Player";
         this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.Form1_FormClosed);
         ((System.ComponentModel.ISupportInitialize)(this.tbrVolume)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.TextBox txtPath;
      private System.Windows.Forms.Button btnOpen;
      private System.Windows.Forms.OpenFileDialog fdOpen;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label lblPlaying;
      private System.Windows.Forms.Button btnPlay;
      private System.Windows.Forms.Button btnStop;
      private System.Windows.Forms.CheckBox cbxPaused;
      private System.Windows.Forms.CheckBox cbxMuted;
      private System.Windows.Forms.TrackBar tbrVolume;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Button btnDispose;
   }
}

