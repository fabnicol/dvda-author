AOB headers for MLP input
-------------------------

This is an experimental fork of ffmpeg aimed at importing MLP file layout into dvda-author
when input is MLP. This is required to compute PTS and DTS stamps in 64/43 B sector headers
of MLP AOBS.
Algorithm is as follows:

At each sector of rank (1-based) N of 2048 B, compute :   

  * sum S of header size in bytes before N in B (64 + 43 + 43 + ... + 43, N-1 terms)    
  * total byte size = 2048  * (N-1) -S   
  * run parallel layout using libffmpeg.a, gives rank of packet and position of packet   
  * this is given by get_mlp_layout() which returns an MLP_LAYOUT table of at most MAX_AOB_SECTORS  
    i.e 1024 * 512 lines. Each line has fields pkt_pos, nb_samples and rank.   
  * pkt_pos is position in MLP file (raw), nb_samples is number of PCM samples in output (raw),  
    rank is sector rank in AOB (all fields 0-based).   
  * take rank such that pkt_pos of MLP packet just after start of sector (total byte size)   
  * nb_samples * c * br = Nbyteswritten    
  * Nbyteswritten / (c * sr * br) = duration    
     Or : duration = nb_samples /sr   
     PTS = ceiling(duration * 90000)   
     DTS = floor(duration * 90000)   
   Add :   
     PTS = PTS + PTS0  
     DTS = DTS + DTS0  
     PTS0=105  
     DTS0=24  
  
Write PTS at offset 23 (0-based) on 5 bytes
Write DTS at offset 28 (0-based) on 5 bytes
First sectors with have PTS starting with 0x31 and DTS with 0x11.

**REST OF AOBs is just MLP bytes except for last sector padding (PCM style, TODO)**

