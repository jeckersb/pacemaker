��    x      �  �   �      (
  �   )
  
  �
  Q  �  T  :  v  �#    %  N    2  �   o2  �   C3  �   �3  �  }4  y   d6  �   �6  7  g7  �  �8    �=  �"  �@  �   "c  �  �c  �   �f  �   g  �  �g  ]   Qi  m  �i  (  k     Fl  r  Zl  �   �n  N  Uo  �   �p     2q     Pq     ^q  8   sq  :   �q  %   �q    r  ,   s  �   Cs  �   t  )   �t      u  +   =u  0   iu  *   �u  ,   �u  #   �u  *   v      Av     bv  +   �v  '   �v      �v     �v  1   w  %   Gw     mw  &   �w  0   �w  7   �w  H   x  G   dx  c   �x  ?   y  N   Py  F   �y      �y  '   z  Q   /z  '   �z  $   �z  c   �z     2{  )   F{  d   p{  Y   �{  �   /|  �   }  �   �}     :~  j   W~     �~  &   �~  �   �~    �  �   ��  q   >�  0   ��  a   �     C�     Y�  9   Z�  �   ��     ��  Q  ��  M   ��  �   C�     4�  b   P�  �   ��     ��     ��     Ӊ     ى  p   �  M   [�  \   ��  N   �  L   U�  �   ��  $  ��  m   ��     �  !   =�  0   _�  �   ��  g  �  ~   |�  Z   ��    V�  �   h�  
  �  Q  '�  T  y�  v  Ϋ    E�  N   _�  �   ��  �   ��  �   '�  �  ��  y   ��  �   �  7  ��  �  ��    ��  �"  ��  �   a�  �  ��  �   ��  �   ^�  �  
�  9   ��  n  ��    9�     G�  S  Z�  }   ��  D  ,�  [   q�     ��  	   ��     ��  %   ��  ?    �  $   `�  �   ��  0   J�  �   {�  �   �     ��     ��     ��  $   ��     �     <�     [�     {�     ��     ��     ��     ��     �     �  !   5�     W�     t�  #   ��      ��  )   ��  4   ��  8   0�  S   i�  (   ��  9   ��  3    �     T�     r�  9   ��     ��     ��  Z         [  )   h  T   �  N   �  �   6 ]   � n       � Z   �    � '   � �   ! �   � O  u U   � 7    -   S (   � �  � 1   � �   �    F	 D  Y	 M   �
 �   �
    � s   � �   W     >   !    `    g j   z M   � H   3 6   | '   � �   � �   y @   B    �    �    � d   � @  = ~   ~ Z   �    A   W   &      6                 Y   n   P          I              4           G   N          (   	   O   w   h   S       \   1   )   $   `          j   9       /      K       e   R   U   t       ;       s          M   
       v         ,   D   B   3   >                  L   a      X                      o      :       .                  5   E   7       q       p   ^   Z   k      i   2   !   ?       H   '       -   u       +      #   l           r   8      c   m       =   x   %   f   g   <      [   C       J   *       F          _   0          ]                      V   b   "   @          d   T   Q                
	  <userinput>grep pcmk /etc/hosts</userinput>
[root@pcmk-1 ~]# grep pcmk /etc/hosts
192.168.122.101 pcmk-1.clusterlabs.org pcmk-1
192.168.122.102 pcmk-2.clusterlabs.org pcmk-2
	 
	  <userinput>ping -c 3 192.168.122.102</userinput>
[root@pcmk-1 ~]# ping -c 3 192.168.122.102
PING 192.168.122.102 (192.168.122.102) 56(84) bytes of data.
64 bytes from 192.168.122.102: icmp_seq=1 ttl=64 time=0.343 ms
64 bytes from 192.168.122.102: icmp_seq=2 ttl=64 time=0.402 ms
64 bytes from 192.168.122.102: icmp_seq=3 ttl=64 time=0.558 ms

--- 192.168.122.102 ping statistics ---
<emphasis>3 packets transmitted, 3 received, 0% packet loss</emphasis>, time 2000ms
rtt min/avg/max/mdev = 0.343/0.434/0.558/0.092 ms
	 
	  <userinput>ping -c 3 pcmk-2</userinput>
[root@pcmk-1 ~]# ping -c 3 pcmk-2
PING pcmk-2.clusterlabs.org (192.168.122.101) 56(84) bytes of data.
64 bytes from pcmk-1.clusterlabs.org (192.168.122.101): icmp_seq=1 ttl=64 time=0.164 ms
64 bytes from pcmk-1.clusterlabs.org (192.168.122.101): icmp_seq=2 ttl=64 time=0.475 ms
64 bytes from pcmk-1.clusterlabs.org (192.168.122.101): icmp_seq=3 ttl=64 time=0.186 ms

--- pcmk-2.clusterlabs.org ping statistics ---
<emphasis>3 packets transmitted, 3 received, 0% packet loss</emphasis>, time 2001ms
rtt min/avg/max/mdev = 0.164/0.275/0.475/0.141 ms
	 
==========================================================================================
 Package                Arch     Version                             Repository      Size
==========================================================================================
Installing:
 corosync               x86_64   1.2.1-1.fc13                        fedora         136 k
 pacemaker              x86_64   1.1.1-1.fc13                        fedora         543 k
Installing for dependencies:
 OpenIPMI-libs          x86_64   2.0.16-8.fc13                       fedora         474 k
 PyXML                  x86_64   0.8.4-17.fc13                       fedora         906 k
 cluster-glue           x86_64   1.0.2-1.fc13                        fedora         230 k
 cluster-glue-libs      x86_64   1.0.2-1.fc13                        fedora         116 k
 corosynclib            x86_64   1.2.1-1.fc13                        fedora         145 k
 heartbeat              x86_64   3.0.0-0.7.0daab7da36a8.hg.fc13      updates        172 k
 heartbeat-libs         x86_64   3.0.0-0.7.0daab7da36a8.hg.fc13      updates        265 k
 libesmtp               x86_64   1.0.4-12.fc12                       fedora          54 k
 libibverbs             x86_64   1.1.3-4.fc13                        fedora          42 k
 libmlx4                x86_64   1.0.1-5.fc13                        fedora          27 k
 libnet                 x86_64   1.1.4-3.fc12                        fedora          49 k
 librdmacm              x86_64   1.0.10-2.fc13                       fedora          22 k
 lm_sensors-libs        x86_64   3.1.2-2.fc13                        fedora          37 k
 net-snmp               x86_64   1:5.5-12.fc13                       fedora         295 k
 net-snmp-libs          x86_64   1:5.5-12.fc13                       fedora         1.5 M
 openhpi-libs           x86_64   2.14.1-3.fc13                       fedora         135 k
 pacemaker-libs         x86_64   1.1.1-1.fc13                        fedora         264 k
 perl-TimeDate          noarch   1:1.20-1.fc13                       fedora          42 k
 resource-agents        x86_64   3.0.10-1.fc13                       fedora         357 k

Transaction Summary
=========================================================================================
Install      21 Package(s)
Upgrade       0 Package(s)

Total download size: 5.7 M
Installed size: 20 M
Downloading Packages:
Setting up and reading Presto delta metadata
updates-testing/prestodelta                                           | 164 kB     00:00     
fedora/prestodelta                                                    |  150 B     00:00     
Processing delta metadata
Package(s) data still to download: 5.7 M
(1/21): OpenIPMI-libs-2.0.16-8.fc13.x86_64.rpm                        | 474 kB     00:00     
(2/21): PyXML-0.8.4-17.fc13.x86_64.rpm                                | 906 kB     00:01     
(3/21): cluster-glue-1.0.2-1.fc13.x86_64.rpm                          | 230 kB     00:00     
(4/21): cluster-glue-libs-1.0.2-1.fc13.x86_64.rpm                     | 116 kB     00:00     
(5/21): corosync-1.2.1-1.fc13.x86_64.rpm                              | 136 kB     00:00     
(6/21): corosynclib-1.2.1-1.fc13.x86_64.rpm                           | 145 kB     00:00     
(7/21): heartbeat-3.0.0-0.7.0daab7da36a8.hg.fc13.x86_64.rpm           | 172 kB     00:00     
(8/21): heartbeat-libs-3.0.0-0.7.0daab7da36a8.hg.fc13.x86_64.rpm      | 265 kB     00:00     
(9/21): libesmtp-1.0.4-12.fc12.x86_64.rpm                             |  54 kB     00:00     
(10/21): libibverbs-1.1.3-4.fc13.x86_64.rpm                           |  42 kB     00:00     
(11/21): libmlx4-1.0.1-5.fc13.x86_64.rpm                              |  27 kB     00:00     
(12/21): libnet-1.1.4-3.fc12.x86_64.rpm                               |  49 kB     00:00     
(13/21): librdmacm-1.0.10-2.fc13.x86_64.rpm                           |  22 kB     00:00     
(14/21): lm_sensors-libs-3.1.2-2.fc13.x86_64.rpm                      |  37 kB     00:00     
(15/21): net-snmp-5.5-12.fc13.x86_64.rpm                              | 295 kB     00:00     
(16/21): net-snmp-libs-5.5-12.fc13.x86_64.rpm                         | 1.5 MB     00:01     
(17/21): openhpi-libs-2.14.1-3.fc13.x86_64.rpm                        | 135 kB     00:00     
(18/21): pacemaker-1.1.1-1.fc13.x86_64.rpm                            | 543 kB     00:00     
(19/21): pacemaker-libs-1.1.1-1.fc13.x86_64.rpm                       | 264 kB     00:00     
(20/21): perl-TimeDate-1.20-1.fc13.noarch.rpm                         |  42 kB     00:00     
(21/21): resource-agents-3.0.10-1.fc13.x86_64.rpm                     | 357 kB     00:00     
----------------------------------------------------------------------------------------
Total                                                        539 kB/s | 5.7 MB     00:10     
warning: rpmts_HdrFromFdno: Header V3 RSA/SHA256 Signature, key ID e8e40fde: NOKEY
fedora/gpgkey                                                         | 3.2 kB     00:00 ... 
Importing GPG key 0xE8E40FDE "Fedora (13) &lt;fedora@fedoraproject.org%gt;" from /etc/pki/rpm-gpg/RPM-GPG-KEY-fedora-x86_64
       
May  4 19:30:54 pcmk-1 setroubleshoot: SELinux is preventing /usr/sbin/corosync "getattr" access on /. For complete SELinux messages. run sealert -l 6e0d4384-638e-4d55-9aaf-7dac011f29c1
May  4 19:30:54 pcmk-1 setroubleshoot: SELinux is preventing /usr/sbin/corosync "getattr" access on /. For complete SELinux messages. run sealert -l 6e0d4384-638e-4d55-9aaf-7dac011f29c1
	 
Running rpm_check_debug
Running Transaction Test
Transaction Test Succeeded
Running Transaction
  Installing     : lm_sensors-libs-3.1.2-2.fc13.x86_64                            1/21 
  Installing     : 1:net-snmp-libs-5.5-12.fc13.x86_64                             2/21 
  Installing     : 1:net-snmp-5.5-12.fc13.x86_64                                  3/21 
  Installing     : openhpi-libs-2.14.1-3.fc13.x86_64                              4/21 
  Installing     : libibverbs-1.1.3-4.fc13.x86_64                                 5/21 
  Installing     : libmlx4-1.0.1-5.fc13.x86_64                                    6/21 
  Installing     : librdmacm-1.0.10-2.fc13.x86_64                                 7/21 
  Installing     : corosync-1.2.1-1.fc13.x86_64                                   8/21 
  Installing     : corosynclib-1.2.1-1.fc13.x86_64                                9/21 
  Installing     : libesmtp-1.0.4-12.fc12.x86_64                                 10/21 
  Installing     : OpenIPMI-libs-2.0.16-8.fc13.x86_64                            11/21 
  Installing     : PyXML-0.8.4-17.fc13.x86_64                                    12/21 
  Installing     : libnet-1.1.4-3.fc12.x86_64                                    13/21 
  Installing     : 1:perl-TimeDate-1.20-1.fc13.noarch                            14/21 
  Installing     : cluster-glue-1.0.2-1.fc13.x86_64                              15/21 
  Installing     : cluster-glue-libs-1.0.2-1.fc13.x86_64                         16/21 
  Installing     : resource-agents-3.0.10-1.fc13.x86_64                          17/21 
  Installing     : heartbeat-libs-3.0.0-0.7.0daab7da36a8.hg.fc13.x86_64          18/21 
  Installing     : heartbeat-3.0.0-0.7.0daab7da36a8.hg.fc13.x86_64               19/21 
  Installing     : pacemaker-1.1.1-1.fc13.x86_64                                 20/21 
  Installing     : pacemaker-libs-1.1.1-1.fc13.x86_64                            21/21 

Installed:
  corosync.x86_64 0:1.2.1-1.fc13                    pacemaker.x86_64 0:1.1.1-1.fc13                   

Dependency Installed:
  OpenIPMI-libs.x86_64 0:2.0.16-8.fc13                          
  PyXML.x86_64 0:0.8.4-17.fc13                                  
  cluster-glue.x86_64 0:1.0.2-1.fc13                            
  cluster-glue-libs.x86_64 0:1.0.2-1.fc13                       
  corosynclib.x86_64 0:1.2.1-1.fc13                             
  heartbeat.x86_64 0:3.0.0-0.7.0daab7da36a8.hg.fc13             
  heartbeat-libs.x86_64 0:3.0.0-0.7.0daab7da36a8.hg.fc13        
  libesmtp.x86_64 0:1.0.4-12.fc12                               
  libibverbs.x86_64 0:1.1.3-4.fc13                              
  libmlx4.x86_64 0:1.0.1-5.fc13                                 
  libnet.x86_64 0:1.1.4-3.fc12                                  
  librdmacm.x86_64 0:1.0.10-2.fc13                              
  lm_sensors-libs.x86_64 0:3.1.2-2.fc13                         
  net-snmp.x86_64 1:5.5-12.fc13                                 
  net-snmp-libs.x86_64 1:5.5-12.fc13                            
  openhpi-libs.x86_64 0:2.14.1-3.fc13                           
  pacemaker-libs.x86_64 0:1.1.1-1.fc13                          
  perl-TimeDate.noarch 1:1.20-1.fc13                            
  resource-agents.x86_64 0:3.0.10-1.fc13                        

Complete!
[root@pcmk-1 ~]# 
       
[beekhof@pcmk-1 ~]$ su -
Password:
[<emphasis>root</emphasis>@pcmk-1 ~]#
     
[root@pcmk-1 ~]# <userinput>cat &lt;&lt;-END &gt;&gt;/etc/corosync/service.d/pcmk</userinput>
service {
        # Load the Pacemaker Cluster Resource Manager
        name: pacemaker
        ver:  0
}
END
       
[root@pcmk-1 ~]# <userinput>cat /etc/sysconfig/network</userinput>
NETWORKING=yes
<emphasis>HOSTNAME=pcmk-1.clusterlabs.org</emphasis>
GATEWAY=192.168.122.1
       
[root@pcmk-1 ~]# <userinput>cat /etc/sysconfig/network</userinput>
NETWORKING=yes
<emphasis>HOSTNAME=pcmk-1</emphasis>
GATEWAY=192.168.122.1
       
[root@pcmk-1 ~]# <userinput>cp /etc/corosync/corosync.conf.example  /etc/corosync/corosync.conf</userinput>
[root@pcmk-1 ~]# <userinput>sed -i.bak "s/.*mcastaddr:.*/mcastaddr:\ $ais_mcast/g" /etc/corosync/corosync.conf</userinput>
[root@pcmk-1 ~]# <userinput>sed -i.bak "s/.*mcastport:.*/mcastport:\ $ais_port/g" /etc/corosync/corosync.conf</userinput>
[root@pcmk-1 ~]# <userinput>sed -i.bak "s/.*bindnetaddr:.*/bindnetaddr:\ $ais_addr/g" /etc/corosync/corosync.conf</userinput>
       
[root@pcmk-1 ~]# <userinput>env | grep ais_</userinput>
ais_mcast=226.94.1.1
ais_port=4000
ais_addr=192.168.122.0
       
[root@pcmk-1 ~]# <userinput>export ais_port=4000</userinput>
[root@pcmk-1 ~]# <userinput>export ais_mcast=226.94.1.1</userinput>
       
[root@pcmk-1 ~]# <userinput>for f in /etc/corosync/corosync.conf /etc/corosync/service.d/pcmk /etc/hosts; do scp $f pcmk-2:$f ; done</userinput>
corosync.conf                            100% 1528     1.5KB/s   00:00
hosts                                    100%  281     0.3KB/s   00:00
[root@pcmk-1 ~]#
       
[root@pcmk-1 ~]# <userinput>ip addr</userinput>
1: lo: &lt;LOOPBACK,UP,LOWER_UP&gt; mtu 16436 qdisc noqueue state UNKNOWN 
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
    inet6 ::1/128 scope host 
       valid_lft forever preferred_lft forever
2: eth0: &lt;BROADCAST,MULTICAST,UP,LOWER_UP&gt; mtu 1500 qdisc pfifo_fast state UNKNOWN qlen 1000
    link/ether 00:0c:29:6f:e1:58 brd ff:ff:ff:ff:ff:ff
    inet 192.168.9.41/24 brd 192.168.9.255 scope global eth0
    inet6 ::20c:29ff:fe6f:e158/64 scope global dynamic 
       valid_lft 2591667sec preferred_lft 604467sec
    inet6 2002:57ae:43fc:0:20c:29ff:fe6f:e158/64 scope global dynamic 
       valid_lft 2591990sec preferred_lft 604790sec
    inet6 fe80::20c:29ff:fe6f:e158/64 scope link 
       valid_lft forever preferred_lft forever
[root@pcmk-1 ~]# ping -c 1 www.google.com
PING www.l.google.com (74.125.39.99) 56(84) bytes of data.
64 bytes from fx-in-f99.1e100.net (74.125.39.99): icmp_seq=1 ttl=56 time=16.7 ms

--- www.l.google.com ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 20ms
rtt min/avg/max/mdev = 16.713/16.713/16.713/0.000 ms
[root@pcmk-1 ~]# <userinput>/sbin/chkconfig network on</userinput>
[root@pcmk-1 ~]# 
     
[root@pcmk-1 ~]# <userinput>scp -r .ssh pcmk-2:</userinput>
The authenticity of host 'pcmk-2 (192.168.122.102)' can't be established.
RSA key fingerprint is b1:2b:55:93:f1:d9:52:2b:0f:f2:8a:4e:ae:c6:7c:9a.
Are you sure you want to continue connecting (yes/no)? yes
Warning: Permanently added 'pcmk-2,192.168.122.102' (RSA) to the list of known hosts.
<emphasis>root@pcmk-2's password:</emphasis> 
id_dsa.pub                           100%  616     0.6KB/s   00:00    
id_dsa                               100%  672     0.7KB/s   00:00    
known_hosts                          100%  400     0.4KB/s   00:00    
authorized_keys                      100%  616     0.6KB/s   00:00    
[root@pcmk-1 ~]# <userinput>ssh pcmk-2 -- uname -n</userinput>
pcmk-2
[root@pcmk-1 ~]#
	 
[root@pcmk-1 ~]# <userinput>sed -i.bak "s/enabled=0/enabled=1/g" /etc/yum.repos.d/fedora.repo</userinput>
[root@pcmk-1 ~]# <userinput>sed -i.bak "s/enabled=0/enabled=1/g" /etc/yum.repos.d/fedora-updates.repo</userinput>
[root@pcmk-1 ~]# <userinput>yum install -y pacemaker corosync</userinput>
Loaded plugins: presto, refresh-packagekit
fedora/metalink                   	                           |  22 kB     00:00     
fedora-debuginfo/metalink         	                           |  16 kB     00:00     
fedora-debuginfo                  	                           | 3.2 kB     00:00     
fedora-debuginfo/primary_db       	                           | 1.4 MB     00:04     
fedora-source/metalink            	                           |  22 kB     00:00     
fedora-source                     	                           | 3.2 kB     00:00     
fedora-source/primary_db          	                           | 3.0 MB     00:05     
updates/metalink                  	                           |  26 kB     00:00     
updates                           	                           | 2.6 kB     00:00     
updates/primary_db                	                           | 1.1 kB     00:00     
updates-debuginfo/metalink        	                           |  18 kB     00:00     
updates-debuginfo                 	                           | 2.6 kB     00:00     
updates-debuginfo/primary_db      	                           | 1.1 kB     00:00     
updates-source/metalink           	                           |  25 kB     00:00     
updates-source                    	                           | 2.6 kB     00:00     
updates-source/primary_db         	                           | 1.1 kB     00:00     
Setting up Install Process
Resolving Dependencies
--&gt; Running transaction check
---&gt; Package corosync.x86_64 0:1.2.1-1.fc13 set to be updated
--&gt; Processing Dependency: corosynclib = 1.2.1-1.fc13 for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libquorum.so.4(COROSYNC_QUORUM_1.0)(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libvotequorum.so.4(COROSYNC_VOTEQUORUM_1.0)(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libcpg.so.4(COROSYNC_CPG_1.0)(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libconfdb.so.4(COROSYNC_CONFDB_1.0)(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libcfg.so.4(COROSYNC_CFG_0.82)(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libpload.so.4(COROSYNC_PLOAD_1.0)(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: liblogsys.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libconfdb.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libcoroipcc.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libcpg.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libquorum.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libcoroipcs.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libvotequorum.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libcfg.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libtotem_pg.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libpload.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
---&gt; Package pacemaker.x86_64 0:1.1.1-1.fc13 set to be updated
--&gt; Processing Dependency: heartbeat &gt;= 3.0.0 for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: net-snmp &gt;= 5.4 for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: resource-agents for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: cluster-glue for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libnetsnmp.so.20()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libcrmcluster.so.1()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libpengine.so.3()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libnetsnmpagent.so.20()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libesmtp.so.5()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libstonithd.so.1()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libhbclient.so.1()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libpils.so.2()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libpe_status.so.2()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libnetsnmpmibs.so.20()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libnetsnmphelpers.so.20()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libcib.so.1()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libccmclient.so.1()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libstonith.so.1()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: liblrm.so.2()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libtransitioner.so.1()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libpe_rules.so.2()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libcrmcommon.so.2()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libplumb.so.2()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Running transaction check
---&gt; Package cluster-glue.x86_64 0:1.0.2-1.fc13 set to be updated
--&gt; Processing Dependency: perl-TimeDate for package: cluster-glue-1.0.2-1.fc13.x86_64
--&gt; Processing Dependency: libOpenIPMIutils.so.0()(64bit) for package: cluster-glue-1.0.2-1.fc13.x86_64
--&gt; Processing Dependency: libOpenIPMIposix.so.0()(64bit) for package: cluster-glue-1.0.2-1.fc13.x86_64
--&gt; Processing Dependency: libopenhpi.so.2()(64bit) for package: cluster-glue-1.0.2-1.fc13.x86_64
--&gt; Processing Dependency: libOpenIPMI.so.0()(64bit) for package: cluster-glue-1.0.2-1.fc13.x86_64
---&gt; Package cluster-glue-libs.x86_64 0:1.0.2-1.fc13 set to be updated
---&gt; Package corosynclib.x86_64 0:1.2.1-1.fc13 set to be updated
--&gt; Processing Dependency: librdmacm.so.1(RDMACM_1.0)(64bit) for package: corosynclib-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libibverbs.so.1(IBVERBS_1.0)(64bit) for package: corosynclib-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libibverbs.so.1(IBVERBS_1.1)(64bit) for package: corosynclib-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libibverbs.so.1()(64bit) for package: corosynclib-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: librdmacm.so.1()(64bit) for package: corosynclib-1.2.1-1.fc13.x86_64
---&gt; Package heartbeat.x86_64 0:3.0.0-0.7.0daab7da36a8.hg.fc13 set to be updated
--&gt; Processing Dependency: PyXML for package: heartbeat-3.0.0-0.7.0daab7da36a8.hg.fc13.x86_64
---&gt; Package heartbeat-libs.x86_64 0:3.0.0-0.7.0daab7da36a8.hg.fc13 set to be updated
---&gt; Package libesmtp.x86_64 0:1.0.4-12.fc12 set to be updated
---&gt; Package net-snmp.x86_64 1:5.5-12.fc13 set to be updated
--&gt; Processing Dependency: libsensors.so.4()(64bit) for package: 1:net-snmp-5.5-12.fc13.x86_64
---&gt; Package net-snmp-libs.x86_64 1:5.5-12.fc13 set to be updated
---&gt; Package pacemaker-libs.x86_64 0:1.1.1-1.fc13 set to be updated
---&gt; Package resource-agents.x86_64 0:3.0.10-1.fc13 set to be updated
--&gt; Processing Dependency: libnet.so.1()(64bit) for package: resource-agents-3.0.10-1.fc13.x86_64
--&gt; Running transaction check
---&gt; Package OpenIPMI-libs.x86_64 0:2.0.16-8.fc13 set to be updated
---&gt; Package PyXML.x86_64 0:0.8.4-17.fc13 set to be updated
---&gt; Package libibverbs.x86_64 0:1.1.3-4.fc13 set to be updated
--&gt; Processing Dependency: libibverbs-driver for package: libibverbs-1.1.3-4.fc13.x86_64
---&gt; Package libnet.x86_64 0:1.1.4-3.fc12 set to be updated
---&gt; Package librdmacm.x86_64 0:1.0.10-2.fc13 set to be updated
---&gt; Package lm_sensors-libs.x86_64 0:3.1.2-2.fc13 set to be updated
---&gt; Package openhpi-libs.x86_64 0:2.14.1-3.fc13 set to be updated
---&gt; Package perl-TimeDate.noarch 1:1.20-1.fc13 set to be updated
--&gt; Running transaction check
---&gt; Package libmlx4.x86_64 0:1.0.1-5.fc13 set to be updated
--&gt; Finished Dependency Resolution

Dependencies Resolved

       
[root@pcmk-1 ~]# <userinput>source /etc/sysconfig/network</userinput>
[root@pcmk-1 ~]# <userinput>hostname $HOSTNAME</userinput>
       
[root@pcmk-1 ~]# <userinput>ssh-keygen -t dsa -f ~/.ssh/id_dsa -N ""</userinput>
<emphasis>Generating public/private dsa key pair.</emphasis>
<emphasis>Your identification has been saved in /root/.ssh/id_dsa.</emphasis>
<emphasis>Your public key has been saved in /root/.ssh/id_dsa.pub.</emphasis>
The key fingerprint is:
91:09:5c:82:5a:6a:50:08:4e:b2:0c:62:de:cc:74:44 root@pcmk-1.clusterlabs.org

The key's randomart image is:
+--[ DSA 1024]----+
|==.ooEo..        |
|X O + .o o       |
| * A    +        |
|  +      .       |
| .      S        |
|                 |
|                 |
|                 |
|                 |
+-----------------+
[root@pcmk-1 ~]# <userinput>cp .ssh/id_dsa.pub .ssh/authorized_keys</userinput>
[root@pcmk-1 ~]#
	 
[root@pcmk-1 ~]# <userinput>uname -n</userinput>
pcmk-1
[root@pcmk-1 ~]# <userinput>dnsdomainname </userinput>
clusterlabs.org
       
[root@pcmk-1 ~]# <userinput>uname -n</userinput>
pcmk-1<emphasis>.clusterlabs.org</emphasis>
[root@pcmk-1 ~]# <userinput>dnsdomainname </userinput>
clusterlabs.org
       
[root@pcmk-1 ~]# sed -i.bak "s/SELINUX=enforcing/SELINUX=permissive/g" /etc/selinux/config
[root@pcmk-1 ~]# /sbin/chkconfig --del iptables
[root@pcmk-1 ~]# service iptables stop
iptables: Flushing firewall rules:                         [  OK  ]
iptables: Setting chains to policy ACCEPT: filter          [  OK  ]
iptables: Unloading modules:                               [  OK  ]
       All we need to do now is strip off the domain name portion, which is stored elsewhere anyway. Assign your machine a host name. <footnote> <para> <ulink url="http://docs.fedoraproject.org/install-guide/f13/en-US/html/sn-networkconfig-fedora.html"> http://docs.fedoraproject.org/install-guide/f&amp;DISTRO_VERSION;/en-US/html/sn-networkconfig-fedora.html </ulink> </para> </footnote> I happen to control the clusterlabs.org domain name, so I will use that here. Be sure that the values you chose do not conflict with any existing clusters you might have. For advice on choosing a multi-cast address, see <ulink url="http://www.29west.com/docs/THPM/multicast-address-assignment.html"> http://www.29west.com/docs/THPM/multicast-address-assignment.html </ulink> Before You Continue Burn the disk image to a DVD <footnote> <para> <ulink url="http://docs.fedoraproject.org/readme-burning-isos/en-US.html"> http://docs.fedoraproject.org/readme-burning-isos/en-US.html </ulink> </para> </footnote> and boot from it. Or use the image to boot a virtual machine as I have done here. After clicking through the welcome screen, select your language and keyboard layout <footnote> <para> <ulink url="http://docs.fedoraproject.org/install-guide/f13/en-US/html/s1-langselection-x86.html"> http://docs.fedoraproject.org/install-guide/f&amp;DISTRO_VERSION;/en-US/html/s1-langselection-x86.html </ulink> </para> </footnote> By default, Fedora will give all the space to the <literal>/</literal> (aka. root) partition. Wel'll take some back so we can use DRBD. Choose a port number and multi-cast <footnote> <para> <ulink url="http://en.wikipedia.org/wiki/Multicast">http://en.wikipedia.org/wiki/Multicast</ulink> </para> </footnote> address. <footnote> <para> <ulink url="http://en.wikipedia.org/wiki/Multicast_address">http://en.wikipedia.org/wiki/Multicast_address</ulink> </para> </footnote> Click through the next screens until you reach the login window. Click on the user you created and supply the password you indicated earlier. Cluster Software Installation Configure SSH Configuring Corosync Confirm that you can communicate with the two new nodes: Create a new key and allow anyone with that key to log in: Creating and Activating a new SSH Key Detailed instructions for installing Fedora are available at <ulink url="http://docs.fedoraproject.org/install-guide/f13/">http://docs.fedoraproject.org/install-guide/f&amp;DISTRO_VERSION;/</ulink> in a number of languages. The abbreviated version is as follows... Display and verify the configuration options Do not accept the default network settings. Cluster machines should <emphasis>never</emphasis> obtain an ip address via DHCP. Here I will use the <emphasis>internal</emphasis> addresses for the clusterlab.org network. During installation, we filled in the machine’s fully qualifier domain name (FQDN) which can be rather long when it appears in cluster logs and status output. See for yourself how the machine identifies itself: Fedora Installation - Activate Networking Fedora Installation - Bootloader Fedora Installation - Bring up the Terminal Fedora Installation - Create Non-privileged User Fedora Installation - Customize Networking Fedora Installation - Customize Partitioning Fedora Installation - Date and Time Fedora Installation - Default Partitioning Fedora Installation - First Boot Fedora Installation - Hostname Fedora Installation - Installation Complete Fedora Installation - Installation Type Fedora Installation - Installing Fedora Installation - Software Fedora Installation - Specify Network Preferences Fedora Installation - Storage Devices Fedora Installation - Welcome Fedora Installation: Choose a hostname Fedora Installation: Choose an installation type Fedora Installation: Click here to configure networking Fedora Installation: Click the big green button to activate your changes Fedora Installation: Create a partition to use (later) for website data Fedora Installation: Creating a non-privileged user, take note of the password, you'll need it soon Fedora Installation: Down to business, fire up the command line Fedora Installation: Enable NTP to keep the times on all your nodes consistent Fedora Installation: Go grab something to drink, this may take a while Fedora Installation: Good choice Fedora Installation: Software selection Fedora Installation: Specify network settings for your machine, never choose DHCP Fedora Installation: Stage 1, completed Fedora Installation: Storage Devices Fedora Installation: Unless you have a strong reason not to, accept the default bootloader location Finalize Networking Finally, tell Corosync to start Pacemaker For the purposes of this document, the additional node is called pcmk-2 with address 192.168.122.42. For this document, I have chosen port 4000 and used 226.94.1.1 as the multi-cast address. Go to the terminal window you just opened and switch to the super user (aka. "root") account with the <command>su</command> command. You will need to supply the password you entered earlier during the installation process. However we’re not finished. The machine wont normally see the shortened host name until about it reboots, but we can force it to update. If you plan on following the DRBD or GFS2 portions of this guide, you should reserve at least 1Gb of space on each machine from which to create a shared volume. Install the Cluster Software Install the key on the other nodes and test that you can now run commands remotely, without being prompted Installation Installing the SSH Key on Another Host It is highly recommended to enable NTP on your cluster nodes. Doing so ensures all nodes agree on the current time and makes reading log files significantly easier. Next choose which software should be installed. Change the selection to <literal>Web Server</literal> since we plan on using Apache. Don't enable updates yet, we'll do that (and install any extra software we need) later. After you click next, Fedora will begin installing. Next we automatically determine the hosts address. By not using the full address, we make the configuration suitable to be copied to other nodes. Note that the username (the text before the @ symbol) now indicates we’re running as the super user “root”. Now check the machine is using the correct names Now confirm the change was successful. The revised file contents should look something like this. Now repeat on pcmk-2. Now select where you want Fedora installed. <footnote> <para> <ulink url="http://docs.fedoraproject.org/install-guide/f13/en-US/html/s1-diskpartsetup-x86.html">http://docs.fedoraproject.org/install-guide/f13/en-US/html/s1-diskpartsetup-x86.html</ulink> </para> </footnote> As I don’t care about any existing data, I will accept the default and allow Fedora to use the complete drive. However I want to reserve some space for DRBD, so I'll check the <literal>Review and modify partitioning layout</literal> box. Now we need to copy the changes so far to the other node: Now we need to make sure we can communicate with the machines by their name. If you have a DNS server, add additional entries for the three machines. Otherwise, you’ll need to add the machines to /etc/hosts . Below are the entries for my cluster nodes: OS Installation Once the node reboots, follow the on screen instructions <footnote> <para> <ulink url="http://docs.fedoraproject.org/install-guide/f13/en-US/html/ch-firstboot.html"> http://docs.fedoraproject.org/install-guide/f&amp;DISTRO_VERSION;/en-US/html/ch-firstboot.html </ulink> </para> </footnote> to create a system user and configure the time. Once you’re happy with the chosen values, update the Corosync configuration Point your browser to <ulink url="http://fedoraproject.org/en/get-fedora-all">http://fedoraproject.org/en/get-fedora-all</ulink>, locate the <emphasis>Install Media</emphasis> section and download the install DVD that matches your hardware. Propagate the Configuration Repeat the Installation steps so that you have 2 Fedora nodes with the cluster software installed. SSH is a convenient and secure way to copy files and perform commands remotely. For the purposes of this guide, we will create a key without a password (using the -N “” option) so that we can perform remote actions without being prompted. Security Shortcuts Set up /etc/hosts entries Setup Short Node Names Since version 12, Fedora comes with recent versions of everything you need, so simply fire up the shell and run: TODO: Create an Appendix that deals with (at least) re-enabling the firewall. That was the last screenshot, from here on in we’re going to be working from the terminal. The final configuration should look something like the sample in the appendix. The finalized partition layout should look something like the diagram below. The output from the second command is fine, but we really don’t need the domain name included in the basic host details. To address this, we need to update /etc/sysconfig/network. This is what it should look like before we start. To simplify this guide and focus on the aspects directly connected to clustering, we will now disable the machine’s firewall and SELinux installation. Both of these actions create significant security issues and should not be performed on machines that will be exposed to the outside world. Unprotected SSH keys, those without a password, are not recommended for servers exposed to the outside world. Verify Connectivity by Hostname Verify Connectivity by IP address We can now verify the setup by again using ping: You will need to reboot for the SELinux changes to take effect. Otherwise you will see something like this when you start corosync: You will then be prompted to indicate the machine’s physical location and to supply a root password. <footnote> <para> <ulink url="http://docs.fedoraproject.org/install-guide/f13/en-US/html/sn-account_configuration.html"> http://docs.fedoraproject.org/install-guide/f&amp;DISTRO_VERSION;/en-US/html/sn-account_configuration.html </ulink> </para> </footnote> [root@pcmk-1 ~]# <userinput>export ais_addr=`ip addr | grep "inet " | tail -n 1 | awk '{print $4}' | sed s/255/0/`</userinput> [root@pcmk-1 ~]# <userinput>sed -i.bak 's/\.[a-z].*//g' /etc/sysconfig/network</userinput> Project-Id-Version: 0
POT-Creation-Date: 2010-12-15T23:32:37
PO-Revision-Date: 2010-12-16 00:16+0800
Last-Translator: Charlie Chen <laneovcc@gmail.com>
Language-Team: None
Language: 
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit
 
	  <userinput>grep pcmk /etc/hosts</userinput>
[root@pcmk-1 ~]# grep pcmk /etc/hosts
192.168.122.101 pcmk-1.clusterlabs.org pcmk-1
192.168.122.102 pcmk-2.clusterlabs.org pcmk-2
	 
	  <userinput>ping -c 3 192.168.122.102</userinput>
[root@pcmk-1 ~]# ping -c 3 192.168.122.102
PING 192.168.122.102 (192.168.122.102) 56(84) bytes of data.
64 bytes from 192.168.122.102: icmp_seq=1 ttl=64 time=0.343 ms
64 bytes from 192.168.122.102: icmp_seq=2 ttl=64 time=0.402 ms
64 bytes from 192.168.122.102: icmp_seq=3 ttl=64 time=0.558 ms

--- 192.168.122.102 ping statistics ---
<emphasis>3 packets transmitted, 3 received, 0% packet loss</emphasis>, time 2000ms
rtt min/avg/max/mdev = 0.343/0.434/0.558/0.092 ms
	 
	  <userinput>ping -c 3 pcmk-2</userinput>
[root@pcmk-1 ~]# ping -c 3 pcmk-2
PING pcmk-2.clusterlabs.org (192.168.122.101) 56(84) bytes of data.
64 bytes from pcmk-1.clusterlabs.org (192.168.122.101): icmp_seq=1 ttl=64 time=0.164 ms
64 bytes from pcmk-1.clusterlabs.org (192.168.122.101): icmp_seq=2 ttl=64 time=0.475 ms
64 bytes from pcmk-1.clusterlabs.org (192.168.122.101): icmp_seq=3 ttl=64 time=0.186 ms

--- pcmk-2.clusterlabs.org ping statistics ---
<emphasis>3 packets transmitted, 3 received, 0% packet loss</emphasis>, time 2001ms
rtt min/avg/max/mdev = 0.164/0.275/0.475/0.141 ms
	 
==========================================================================================
 Package                Arch     Version                             Repository      Size
==========================================================================================
Installing:
 corosync               x86_64   1.2.1-1.fc13                        fedora         136 k
 pacemaker              x86_64   1.1.1-1.fc13                        fedora         543 k
Installing for dependencies:
 OpenIPMI-libs          x86_64   2.0.16-8.fc13                       fedora         474 k
 PyXML                  x86_64   0.8.4-17.fc13                       fedora         906 k
 cluster-glue           x86_64   1.0.2-1.fc13                        fedora         230 k
 cluster-glue-libs      x86_64   1.0.2-1.fc13                        fedora         116 k
 corosynclib            x86_64   1.2.1-1.fc13                        fedora         145 k
 heartbeat              x86_64   3.0.0-0.7.0daab7da36a8.hg.fc13      updates        172 k
 heartbeat-libs         x86_64   3.0.0-0.7.0daab7da36a8.hg.fc13      updates        265 k
 libesmtp               x86_64   1.0.4-12.fc12                       fedora          54 k
 libibverbs             x86_64   1.1.3-4.fc13                        fedora          42 k
 libmlx4                x86_64   1.0.1-5.fc13                        fedora          27 k
 libnet                 x86_64   1.1.4-3.fc12                        fedora          49 k
 librdmacm              x86_64   1.0.10-2.fc13                       fedora          22 k
 lm_sensors-libs        x86_64   3.1.2-2.fc13                        fedora          37 k
 net-snmp               x86_64   1:5.5-12.fc13                       fedora         295 k
 net-snmp-libs          x86_64   1:5.5-12.fc13                       fedora         1.5 M
 openhpi-libs           x86_64   2.14.1-3.fc13                       fedora         135 k
 pacemaker-libs         x86_64   1.1.1-1.fc13                        fedora         264 k
 perl-TimeDate          noarch   1:1.20-1.fc13                       fedora          42 k
 resource-agents        x86_64   3.0.10-1.fc13                       fedora         357 k

Transaction Summary
=========================================================================================
Install      21 Package(s)
Upgrade       0 Package(s)

Total download size: 5.7 M
Installed size: 20 M
Downloading Packages:
Setting up and reading Presto delta metadata
updates-testing/prestodelta                                           | 164 kB     00:00     
fedora/prestodelta                                                    |  150 B     00:00     
Processing delta metadata
Package(s) data still to download: 5.7 M
(1/21): OpenIPMI-libs-2.0.16-8.fc13.x86_64.rpm                        | 474 kB     00:00     
(2/21): PyXML-0.8.4-17.fc13.x86_64.rpm                                | 906 kB     00:01     
(3/21): cluster-glue-1.0.2-1.fc13.x86_64.rpm                          | 230 kB     00:00     
(4/21): cluster-glue-libs-1.0.2-1.fc13.x86_64.rpm                     | 116 kB     00:00     
(5/21): corosync-1.2.1-1.fc13.x86_64.rpm                              | 136 kB     00:00     
(6/21): corosynclib-1.2.1-1.fc13.x86_64.rpm                           | 145 kB     00:00     
(7/21): heartbeat-3.0.0-0.7.0daab7da36a8.hg.fc13.x86_64.rpm           | 172 kB     00:00     
(8/21): heartbeat-libs-3.0.0-0.7.0daab7da36a8.hg.fc13.x86_64.rpm      | 265 kB     00:00     
(9/21): libesmtp-1.0.4-12.fc12.x86_64.rpm                             |  54 kB     00:00     
(10/21): libibverbs-1.1.3-4.fc13.x86_64.rpm                           |  42 kB     00:00     
(11/21): libmlx4-1.0.1-5.fc13.x86_64.rpm                              |  27 kB     00:00     
(12/21): libnet-1.1.4-3.fc12.x86_64.rpm                               |  49 kB     00:00     
(13/21): librdmacm-1.0.10-2.fc13.x86_64.rpm                           |  22 kB     00:00     
(14/21): lm_sensors-libs-3.1.2-2.fc13.x86_64.rpm                      |  37 kB     00:00     
(15/21): net-snmp-5.5-12.fc13.x86_64.rpm                              | 295 kB     00:00     
(16/21): net-snmp-libs-5.5-12.fc13.x86_64.rpm                         | 1.5 MB     00:01     
(17/21): openhpi-libs-2.14.1-3.fc13.x86_64.rpm                        | 135 kB     00:00     
(18/21): pacemaker-1.1.1-1.fc13.x86_64.rpm                            | 543 kB     00:00     
(19/21): pacemaker-libs-1.1.1-1.fc13.x86_64.rpm                       | 264 kB     00:00     
(20/21): perl-TimeDate-1.20-1.fc13.noarch.rpm                         |  42 kB     00:00     
(21/21): resource-agents-3.0.10-1.fc13.x86_64.rpm                     | 357 kB     00:00     
----------------------------------------------------------------------------------------
Total                                                        539 kB/s | 5.7 MB     00:10     
warning: rpmts_HdrFromFdno: Header V3 RSA/SHA256 Signature, key ID e8e40fde: NOKEY
fedora/gpgkey                                                         | 3.2 kB     00:00 ... 
Importing GPG key 0xE8E40FDE "Fedora (13) &lt;fedora@fedoraproject.org%gt;" from /etc/pki/rpm-gpg/RPM-GPG-KEY-fedora-x86_64
       
May  4 19:30:54 pcmk-1 setroubleshoot: SELinux is preventing /usr/sbin/corosync "getattr" access on /. For complete SELinux messages. run sealert -l 6e0d4384-638e-4d55-9aaf-7dac011f29c1
May  4 19:30:54 pcmk-1 setroubleshoot: SELinux is preventing /usr/sbin/corosync "getattr" access on /. For complete SELinux messages. run sealert -l 6e0d4384-638e-4d55-9aaf-7dac011f29c1
	 
Running rpm_check_debug
Running Transaction Test
Transaction Test Succeeded
Running Transaction
  Installing     : lm_sensors-libs-3.1.2-2.fc13.x86_64                            1/21 
  Installing     : 1:net-snmp-libs-5.5-12.fc13.x86_64                             2/21 
  Installing     : 1:net-snmp-5.5-12.fc13.x86_64                                  3/21 
  Installing     : openhpi-libs-2.14.1-3.fc13.x86_64                              4/21 
  Installing     : libibverbs-1.1.3-4.fc13.x86_64                                 5/21 
  Installing     : libmlx4-1.0.1-5.fc13.x86_64                                    6/21 
  Installing     : librdmacm-1.0.10-2.fc13.x86_64                                 7/21 
  Installing     : corosync-1.2.1-1.fc13.x86_64                                   8/21 
  Installing     : corosynclib-1.2.1-1.fc13.x86_64                                9/21 
  Installing     : libesmtp-1.0.4-12.fc12.x86_64                                 10/21 
  Installing     : OpenIPMI-libs-2.0.16-8.fc13.x86_64                            11/21 
  Installing     : PyXML-0.8.4-17.fc13.x86_64                                    12/21 
  Installing     : libnet-1.1.4-3.fc12.x86_64                                    13/21 
  Installing     : 1:perl-TimeDate-1.20-1.fc13.noarch                            14/21 
  Installing     : cluster-glue-1.0.2-1.fc13.x86_64                              15/21 
  Installing     : cluster-glue-libs-1.0.2-1.fc13.x86_64                         16/21 
  Installing     : resource-agents-3.0.10-1.fc13.x86_64                          17/21 
  Installing     : heartbeat-libs-3.0.0-0.7.0daab7da36a8.hg.fc13.x86_64          18/21 
  Installing     : heartbeat-3.0.0-0.7.0daab7da36a8.hg.fc13.x86_64               19/21 
  Installing     : pacemaker-1.1.1-1.fc13.x86_64                                 20/21 
  Installing     : pacemaker-libs-1.1.1-1.fc13.x86_64                            21/21 

Installed:
  corosync.x86_64 0:1.2.1-1.fc13                    pacemaker.x86_64 0:1.1.1-1.fc13                   

Dependency Installed:
  OpenIPMI-libs.x86_64 0:2.0.16-8.fc13                          
  PyXML.x86_64 0:0.8.4-17.fc13                                  
  cluster-glue.x86_64 0:1.0.2-1.fc13                            
  cluster-glue-libs.x86_64 0:1.0.2-1.fc13                       
  corosynclib.x86_64 0:1.2.1-1.fc13                             
  heartbeat.x86_64 0:3.0.0-0.7.0daab7da36a8.hg.fc13             
  heartbeat-libs.x86_64 0:3.0.0-0.7.0daab7da36a8.hg.fc13        
  libesmtp.x86_64 0:1.0.4-12.fc12                               
  libibverbs.x86_64 0:1.1.3-4.fc13                              
  libmlx4.x86_64 0:1.0.1-5.fc13                                 
  libnet.x86_64 0:1.1.4-3.fc12                                  
  librdmacm.x86_64 0:1.0.10-2.fc13                              
  lm_sensors-libs.x86_64 0:3.1.2-2.fc13                         
  net-snmp.x86_64 1:5.5-12.fc13                                 
  net-snmp-libs.x86_64 1:5.5-12.fc13                            
  openhpi-libs.x86_64 0:2.14.1-3.fc13                           
  pacemaker-libs.x86_64 0:1.1.1-1.fc13                          
  perl-TimeDate.noarch 1:1.20-1.fc13                            
  resource-agents.x86_64 0:3.0.10-1.fc13                        

Complete!
[root@pcmk-1 ~]# 
       
[beekhof@pcmk-1 ~]$ su -
Password:
[<emphasis>root</emphasis>@pcmk-1 ~]#
     
[root@pcmk-1 ~]# <userinput>cat &lt;&lt;-END &gt;&gt;/etc/corosync/service.d/pcmk</userinput>
service {
        # Load the Pacemaker Cluster Resource Manager
        name: pacemaker
        ver:  0
}
END
       
[root@pcmk-1 ~]# <userinput>cat /etc/sysconfig/network</userinput>
NETWORKING=yes
<emphasis>HOSTNAME=pcmk-1.clusterlabs.org</emphasis>
GATEWAY=192.168.122.1
       
[root@pcmk-1 ~]# <userinput>cat /etc/sysconfig/network</userinput>
NETWORKING=yes
<emphasis>HOSTNAME=pcmk-1</emphasis>
GATEWAY=192.168.122.1
       
[root@pcmk-1 ~]# <userinput>cp /etc/corosync/corosync.conf.example  /etc/corosync/corosync.conf</userinput>
[root@pcmk-1 ~]# <userinput>sed -i.bak "s/.*mcastaddr:.*/mcastaddr:\ $ais_mcast/g" /etc/corosync/corosync.conf</userinput>
[root@pcmk-1 ~]# <userinput>sed -i.bak "s/.*mcastport:.*/mcastport:\ $ais_port/g" /etc/corosync/corosync.conf</userinput>
[root@pcmk-1 ~]# <userinput>sed -i.bak "s/.*bindnetaddr:.*/bindnetaddr:\ $ais_addr/g" /etc/corosync/corosync.conf</userinput>
       
[root@pcmk-1 ~]# <userinput>env | grep ais_</userinput>
ais_mcast=226.94.1.1
ais_port=4000
ais_addr=192.168.122.0
       
[root@pcmk-1 ~]# <userinput>export ais_port=4000</userinput>
[root@pcmk-1 ~]# <userinput>export ais_mcast=226.94.1.1</userinput>
       
[root@pcmk-1 ~]# <userinput>for f in /etc/corosync/corosync.conf /etc/corosync/service.d/pcmk /etc/hosts; do scp $f pcmk-2:$f ; done</userinput>
corosync.conf                            100% 1528     1.5KB/s   00:00
hosts                                    100%  281     0.3KB/s   00:00
[root@pcmk-1 ~]#
       
[root@pcmk-1 ~]# <userinput>ip addr</userinput>
1: lo: &lt;LOOPBACK,UP,LOWER_UP&gt; mtu 16436 qdisc noqueue state UNKNOWN 
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
    inet6 ::1/128 scope host 
       valid_lft forever preferred_lft forever
2: eth0: &lt;BROADCAST,MULTICAST,UP,LOWER_UP&gt; mtu 1500 qdisc pfifo_fast state UNKNOWN qlen 1000
    link/ether 00:0c:29:6f:e1:58 brd ff:ff:ff:ff:ff:ff
    inet 192.168.9.41/24 brd 192.168.9.255 scope global eth0
    inet6 ::20c:29ff:fe6f:e158/64 scope global dynamic 
       valid_lft 2591667sec preferred_lft 604467sec
    inet6 2002:57ae:43fc:0:20c:29ff:fe6f:e158/64 scope global dynamic 
       valid_lft 2591990sec preferred_lft 604790sec
    inet6 fe80::20c:29ff:fe6f:e158/64 scope link 
       valid_lft forever preferred_lft forever
[root@pcmk-1 ~]# ping -c 1 www.google.com
PING www.l.google.com (74.125.39.99) 56(84) bytes of data.
64 bytes from fx-in-f99.1e100.net (74.125.39.99): icmp_seq=1 ttl=56 time=16.7 ms

--- www.l.google.com ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 20ms
rtt min/avg/max/mdev = 16.713/16.713/16.713/0.000 ms
[root@pcmk-1 ~]# <userinput>/sbin/chkconfig network on</userinput>
[root@pcmk-1 ~]# 
     
[root@pcmk-1 ~]# <userinput>scp -r .ssh pcmk-2:</userinput>
The authenticity of host 'pcmk-2 (192.168.122.102)' can't be established.
RSA key fingerprint is b1:2b:55:93:f1:d9:52:2b:0f:f2:8a:4e:ae:c6:7c:9a.
Are you sure you want to continue connecting (yes/no)? yes
Warning: Permanently added 'pcmk-2,192.168.122.102' (RSA) to the list of known hosts.
<emphasis>root@pcmk-2's password:</emphasis> 
id_dsa.pub                           100%  616     0.6KB/s   00:00    
id_dsa                               100%  672     0.7KB/s   00:00    
known_hosts                          100%  400     0.4KB/s   00:00    
authorized_keys                      100%  616     0.6KB/s   00:00    
[root@pcmk-1 ~]# <userinput>ssh pcmk-2 -- uname -n</userinput>
pcmk-2
[root@pcmk-1 ~]#
	 
[root@pcmk-1 ~]# <userinput>sed -i.bak "s/enabled=0/enabled=1/g" /etc/yum.repos.d/fedora.repo</userinput>
[root@pcmk-1 ~]# <userinput>sed -i.bak "s/enabled=0/enabled=1/g" /etc/yum.repos.d/fedora-updates.repo</userinput>
[root@pcmk-1 ~]# <userinput>yum install -y pacemaker corosync</userinput>
Loaded plugins: presto, refresh-packagekit
fedora/metalink                   	                           |  22 kB     00:00     
fedora-debuginfo/metalink         	                           |  16 kB     00:00     
fedora-debuginfo                  	                           | 3.2 kB     00:00     
fedora-debuginfo/primary_db       	                           | 1.4 MB     00:04     
fedora-source/metalink            	                           |  22 kB     00:00     
fedora-source                     	                           | 3.2 kB     00:00     
fedora-source/primary_db          	                           | 3.0 MB     00:05     
updates/metalink                  	                           |  26 kB     00:00     
updates                           	                           | 2.6 kB     00:00     
updates/primary_db                	                           | 1.1 kB     00:00     
updates-debuginfo/metalink        	                           |  18 kB     00:00     
updates-debuginfo                 	                           | 2.6 kB     00:00     
updates-debuginfo/primary_db      	                           | 1.1 kB     00:00     
updates-source/metalink           	                           |  25 kB     00:00     
updates-source                    	                           | 2.6 kB     00:00     
updates-source/primary_db         	                           | 1.1 kB     00:00     
Setting up Install Process
Resolving Dependencies
--&gt; Running transaction check
---&gt; Package corosync.x86_64 0:1.2.1-1.fc13 set to be updated
--&gt; Processing Dependency: corosynclib = 1.2.1-1.fc13 for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libquorum.so.4(COROSYNC_QUORUM_1.0)(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libvotequorum.so.4(COROSYNC_VOTEQUORUM_1.0)(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libcpg.so.4(COROSYNC_CPG_1.0)(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libconfdb.so.4(COROSYNC_CONFDB_1.0)(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libcfg.so.4(COROSYNC_CFG_0.82)(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libpload.so.4(COROSYNC_PLOAD_1.0)(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: liblogsys.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libconfdb.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libcoroipcc.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libcpg.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libquorum.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libcoroipcs.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libvotequorum.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libcfg.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libtotem_pg.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libpload.so.4()(64bit) for package: corosync-1.2.1-1.fc13.x86_64
---&gt; Package pacemaker.x86_64 0:1.1.1-1.fc13 set to be updated
--&gt; Processing Dependency: heartbeat &gt;= 3.0.0 for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: net-snmp &gt;= 5.4 for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: resource-agents for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: cluster-glue for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libnetsnmp.so.20()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libcrmcluster.so.1()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libpengine.so.3()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libnetsnmpagent.so.20()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libesmtp.so.5()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libstonithd.so.1()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libhbclient.so.1()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libpils.so.2()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libpe_status.so.2()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libnetsnmpmibs.so.20()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libnetsnmphelpers.so.20()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libcib.so.1()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libccmclient.so.1()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libstonith.so.1()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: liblrm.so.2()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libtransitioner.so.1()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libpe_rules.so.2()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libcrmcommon.so.2()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Processing Dependency: libplumb.so.2()(64bit) for package: pacemaker-1.1.1-1.fc13.x86_64
--&gt; Running transaction check
---&gt; Package cluster-glue.x86_64 0:1.0.2-1.fc13 set to be updated
--&gt; Processing Dependency: perl-TimeDate for package: cluster-glue-1.0.2-1.fc13.x86_64
--&gt; Processing Dependency: libOpenIPMIutils.so.0()(64bit) for package: cluster-glue-1.0.2-1.fc13.x86_64
--&gt; Processing Dependency: libOpenIPMIposix.so.0()(64bit) for package: cluster-glue-1.0.2-1.fc13.x86_64
--&gt; Processing Dependency: libopenhpi.so.2()(64bit) for package: cluster-glue-1.0.2-1.fc13.x86_64
--&gt; Processing Dependency: libOpenIPMI.so.0()(64bit) for package: cluster-glue-1.0.2-1.fc13.x86_64
---&gt; Package cluster-glue-libs.x86_64 0:1.0.2-1.fc13 set to be updated
---&gt; Package corosynclib.x86_64 0:1.2.1-1.fc13 set to be updated
--&gt; Processing Dependency: librdmacm.so.1(RDMACM_1.0)(64bit) for package: corosynclib-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libibverbs.so.1(IBVERBS_1.0)(64bit) for package: corosynclib-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libibverbs.so.1(IBVERBS_1.1)(64bit) for package: corosynclib-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: libibverbs.so.1()(64bit) for package: corosynclib-1.2.1-1.fc13.x86_64
--&gt; Processing Dependency: librdmacm.so.1()(64bit) for package: corosynclib-1.2.1-1.fc13.x86_64
---&gt; Package heartbeat.x86_64 0:3.0.0-0.7.0daab7da36a8.hg.fc13 set to be updated
--&gt; Processing Dependency: PyXML for package: heartbeat-3.0.0-0.7.0daab7da36a8.hg.fc13.x86_64
---&gt; Package heartbeat-libs.x86_64 0:3.0.0-0.7.0daab7da36a8.hg.fc13 set to be updated
---&gt; Package libesmtp.x86_64 0:1.0.4-12.fc12 set to be updated
---&gt; Package net-snmp.x86_64 1:5.5-12.fc13 set to be updated
--&gt; Processing Dependency: libsensors.so.4()(64bit) for package: 1:net-snmp-5.5-12.fc13.x86_64
---&gt; Package net-snmp-libs.x86_64 1:5.5-12.fc13 set to be updated
---&gt; Package pacemaker-libs.x86_64 0:1.1.1-1.fc13 set to be updated
---&gt; Package resource-agents.x86_64 0:3.0.10-1.fc13 set to be updated
--&gt; Processing Dependency: libnet.so.1()(64bit) for package: resource-agents-3.0.10-1.fc13.x86_64
--&gt; Running transaction check
---&gt; Package OpenIPMI-libs.x86_64 0:2.0.16-8.fc13 set to be updated
---&gt; Package PyXML.x86_64 0:0.8.4-17.fc13 set to be updated
---&gt; Package libibverbs.x86_64 0:1.1.3-4.fc13 set to be updated
--&gt; Processing Dependency: libibverbs-driver for package: libibverbs-1.1.3-4.fc13.x86_64
---&gt; Package libnet.x86_64 0:1.1.4-3.fc12 set to be updated
---&gt; Package librdmacm.x86_64 0:1.0.10-2.fc13 set to be updated
---&gt; Package lm_sensors-libs.x86_64 0:3.1.2-2.fc13 set to be updated
---&gt; Package openhpi-libs.x86_64 0:2.14.1-3.fc13 set to be updated
---&gt; Package perl-TimeDate.noarch 1:1.20-1.fc13 set to be updated
--&gt; Running transaction check
---&gt; Package libmlx4.x86_64 0:1.0.1-5.fc13 set to be updated
--&gt; Finished Dependency Resolution

Dependencies Resolved

       
[root@pcmk-1 ~]# <userinput>source /etc/sysconfig/network</userinput>
[root@pcmk-1 ~]# <userinput>hostname $HOSTNAME</userinput>
       
[root@pcmk-1 ~]# <userinput>ssh-keygen -t dsa -f ~/.ssh/id_dsa -N ""</userinput>
<emphasis>Generating public/private dsa key pair.</emphasis>
<emphasis>Your identification has been saved in /root/.ssh/id_dsa.</emphasis>
<emphasis>Your public key has been saved in /root/.ssh/id_dsa.pub.</emphasis>
The key fingerprint is:
91:09:5c:82:5a:6a:50:08:4e:b2:0c:62:de:cc:74:44 root@pcmk-1.clusterlabs.org

The key's randomart image is:
+--[ DSA 1024]----+
|==.ooEo..        |
|X O + .o o       |
| * A    +        |
|  +      .       |
| .      S        |
|                 |
|                 |
|                 |
|                 |
+-----------------+
[root@pcmk-1 ~]# <userinput>cp .ssh/id_dsa.pub .ssh/authorized_keys</userinput>
[root@pcmk-1 ~]#
	 
[root@pcmk-1 ~]# <userinput>uname -n</userinput>
pcmk-1
[root@pcmk-1 ~]# <userinput>dnsdomainname </userinput>
clusterlabs.org
       
[root@pcmk-1 ~]# <userinput>uname -n</userinput>
pcmk-1<emphasis>.clusterlabs.org</emphasis>
[root@pcmk-1 ~]# <userinput>dnsdomainname </userinput>
clusterlabs.org
       
[root@pcmk-1 ~]# sed -i.bak "s/SELINUX=enforcing/SELINUX=permissive/g" /etc/selinux/config
[root@pcmk-1 ~]# /sbin/chkconfig --del iptables
[root@pcmk-1 ~]# service iptables stop
iptables: Flushing firewall rules:                         [  OK  ]
iptables: Setting chains to policy ACCEPT: filter          [  OK  ]
iptables: Unloading modules:                               [  OK  ]
       我们要做的只是要把域名后面的部分去掉。 给你的机器取个名字。<footnote> <para> <ulink url="http://docs.fedoraproject.org/install-guide/f13/en-US/html/sn-networkconfig-fedora.html"> http://docs.fedoraproject.org/install-guide/f&amp;DISTRO_VERSION;/en-US/html/sn-networkconfig-fedora.html </ulink> </para> </footnote> 我可以使用clusterlabs.org这个域名，所以在这里我用这个域名。 请注意你选择的端口和地址不能跟已存在的集群冲突，关于组播地址的选择，可以参考 <ulink url="http://www.29west.com/docs/THPM/multicast-address-assignment.html"> http://www.29west.com/docs/THPM/multicast-address-assignment.html </ulink> 写在开始之前 烧录一个DVD光盘 <footnote> <para> <ulink url="http://docs.fedoraproject.org/readme-burning-isos/en-US.html"> http://docs.fedoraproject.org/readme-burning-isos/en-US.html </ulink> </para> </footnote> 并从它启动。或者就像我一样启动一个虚拟机。 在点击欢迎界面的NETX后 ，我们要开始选择语言和键盘类型 <footnote> <para> <ulink url="http://docs.fedoraproject.org/install-guide/f13/en-US/html/s1-langselection-x86.html"> http://docs.fedoraproject.org/install-guide/f&amp;DISTRO_VERSION;/en-US/html/s1-langselection-x86.html </ulink> </para> </footnote> 默认的话，Fedora会将所有的空间都分配给<literal>/</literal> (aka. 根)分区。我们要保留一点给DRBD。 选择一个组播<footnote> <para> <ulink url="http://en.wikipedia.org/wiki/Multicast">http://en.wikipedia.org/wiki/Multicast</ulink> </para> </footnote>端口和地址。<footnote> <para> <ulink url="http://en.wikipedia.org/wiki/Multicast_address">http://en.wikipedia.org/wiki/Multicast_address</ulink> </para> </footnote> 点击next会进入登入界面，点击你创建的用户并输入之前设定的密码。 集群软件安装 配置SSH 配置 Corosync 确认这两个新节点能够通讯: 创建一个密钥并允许所有有这个密钥的用户登入 创建并激活一个新的SSH密钥 详细的安装手册在<ulink url="http://docs.fedoraproject.org/install-guide/f13/">http://docs.fedoraproject.org/install-guide/f&amp;DISTRO_VERSION;/</ulink>。下文是一个简短的版本... 显示并检查配置的环境变量是否正确 不要使用默认的网络设置，集群<emphasis>永远</emphasis>不会靠DHCP来管理IP，这里我使用clusterslab的<emphasis>内部</emphasis>IP。 在安装过程中，我们发现FQDN域名太长了，不利于在日志或状态界面中查看，我们用以下操作来简化机器名: 安装Fedora - 激活网络 安装Fedora - Bootloader 安装Fedora - 打开终端 安装Fedora - 创建非特权用户 安装Fedora  -自定义网络 安装Fedora - 自定义分区 安装Fedora  - 日期和时间 安装Fedora - 默认分区 安装Fedora - 第一次启动 安装Fedora -机器名 安装Fedora - 安装完成 安装Fedora - 安装类型 安装Fedora - 安装中 安装Fedora - 软件 安装Fedora - 指定网络参数 安装Fedora  - 存储设备 安装Fedora  - 欢迎  安装Fedora: 选择一个机器名 安装Fedora: 选择安装类型 安装Fedora: 点击这里来配置网络 安装Fedora:点击绿色按钮来应用你的更改 安装Fedora: 创建一个网站存放数据用的分区 安装Fedora: 创建非特权用户，请注意密码，一会你要用到它的。 安装Fedora:开始干活，打开终端 安装Fedora : 启用NTP来保证所有节点时间同步 安装Fedora: 去搞点东西喝喝 这要一会儿 安装Fedora: 好的选择！ 安装Fedora: 软件选择 安装Fedora: 设定你的网络，永远不要选择DHCP 安装Fedora: Stage 1, 完成 安装Fedora: 存储设备 安装Fedora: 除非有非常强力的理由，不然选择默认的bootloader安装位置 设定网络 最后，告诉corosync要启动pacemaker 在这篇文档中, 另外一个节点叫 pcmk-2 并且IP地址为 192.168.122.42。 在这个文档中，我选择端口4000并且用226.94.1.1作为组播地址: 打开一个终端,然后使用<command>su</command>命令切换到超级用户(root)。输入之前安装时候设定的密码: 然而到这里还没结束，机器还没接受新的配置文件，我们强制它生效。 如果你想试验本文档中关于DRBD或者GFS2的部分，你要为每个节点保留至少1Gb的空间。 安装集群软件 在其他节点安装这个密钥并测试你是否可以执行命令而不用输入密码 安装 在另一个机器上面安装SSH密钥 强烈建议开启NTP时间同步，这样可以使集群更好的同步配置文件以及使日志文件有更好的可读性。 然后我们选择应该安装什么软件。因为我们想用Apache，所以选择<literal>Web Server</literal>。现在不要开启Update源，我们一会操作它。点击下一步，开始安装Fedora。 然后我们用下面的命令自动获得机器的地址。为了让配置文件能够在机器上面的各个机器通用，我们不使用完整的IP地址而使用网络地址。（译者注:corosync配置文件中的监听地址一项可以填写网络地址，corosync会自动匹配应该监听在哪个地址而不是0.0.0.0） 注意用户名 (@符号左边的字符串) 显示我们现在使用的是root用户. 现在我们看看是否按达到我们预期的效果: 现在cat一下看看更改是否成功了。 现在在pcmk-2上面重复以上操作. 如然后你选择想在哪安装Fedora <footnote> <para> <ulink url="http://docs.fedoraproject.org/install-guide/f13/en-US/html/s1-diskpartsetup-x86.html">http://docs.fedoraproject.org/install-guide/f13/en-US/html/s1-diskpartsetup-x86.html</ulink> </para> </footnote>。 如果你像我一样不在意已存在的数据，就选择默认让Fedora来使用完整的驱动器。然而我想为DRBD保留一些空间，所以我勾选了Review and modify partitioning layout。 然后我们把配置文件拷贝到其他节点: 现在我们需要确认我们能通过机器名访问这两台机器，如果你有一个DNS服务器，为这两台节点做域名解析。 安装操作系统 一旦系统重启完毕你可以看到以下界面 <footnote> <para> <ulink url="http://docs.fedoraproject.org/install-guide/f13/en-US/html/ch-firstboot.html"> http://docs.fedoraproject.org/install-guide/f&amp;DISTRO_VERSION;/en-US/html/ch-firstboot.html </ulink> </para> </footnote> ，然后配置用户和设定时间。 确认以上输出没有错误以后，我们用以下命令来配置corosync 在你的浏览器中打开 <ulink url="http://fedoraproject.org/en/get-fedora-all">http://fedoraproject.org/en/get-fedora-all</ulink>,找到<emphasis>Install Media</emphasis>部分并下载适合你硬件的安装DVD文件。  传送配置文件 在另一台Fedora 12机器上面重复以上操作步骤，这样你就有2台安装了集群软件的节点了。 SSH 是一个方便又安全来的用来远程传输文件或运行命令 的工具. 在这个文档中, 我们创建ssh key(用 -N “” 选项)来免去登入要输入密码的麻烦。 安全提示 否则，我们修改/etc/hosts文件来达到相同的效果: 安装 简化节点名称 从Fedora 12开始，你需要的东西都已经准备好了，只需在终端命令行运行以下命令: TODO: Create an Appendix that deals with (at least) re-enabling the firewall. 这是最后一个截屏了，剩下的我们都用命令行来操作。 最后配置文件应该看起来像下面的样子。 完整的分区应该像下面一样。 第二个命令的输出是正常的，但是我们真的不需要这么详细的输出，我们更改/etc/sysconfig/network文件来达到简化的目的。 为了简化本文档并更好的关注集群方面的问题，我们现在在先禁用防火墙和SELinux。这些操作都会导致重大的安全问题,并不推荐对公网上的集群这样做。 不推荐在公网的机器上采用未用密码保护的ssh-key 通过机器名检查连接 通过IP地址来检查连接 现在让我们ping一下: 你需要重启来保证SELinux正确关闭。不然你启动corosync的时候将看到以下提示: 然后你会被提示选择机器所在地并设定root密码。<footnote> <para> <ulink url="http://docs.fedoraproject.org/install-guide/f13/en-US/html/sn-account_configuration.html"> http://docs.fedoraproject.org/install-guide/f&amp;DISTRO_VERSION;/en-US/html/sn-account_configuration.html </ulink> </para> </footnote> [root@pcmk-1 ~]# <userinput>export ais_addr=`ip addr | grep "inet " | tail -n 1 | awk '{print $4}' | sed s/255/0/`</userinput> [root@pcmk-1 ~]# <userinput>sed -i.bak 's/\.[a-z].*//g' /etc/sysconfig/network</userinput> 