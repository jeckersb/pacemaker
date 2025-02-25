��    G      T  a   �        �     �	  �  h   n  ]  �  �  5  �  �  �  �  e  �  �      k  �/  
  U3  �  t=  �   A  E   �A  ]	  �A  �   SK  h  $L  Y  �W  �  �Z  �   �]     Z^     n^  �   �^  (   j_    �_     �`     �`     �`  %   �`  R   a  �   ja  �  :b     �c  #   �c  �   d  �   e  2   �e  �   �e  �   Vf  h   �f  �   Jg  �   	h  ;   �h     �h     �h  '    i      (i     Ii  u   Vi      �i  +   �i  2   j  7   Lj     �j  �   �j  <   k  r   [k  �   �k  =  �l  v   n  v   zn  �  �n  m   �p  M   \q  �   �q  =   ^r  T   �r  B   �r     4s     Ns  b  bs  �   �t  �	  �u  h   "  ]  �  �  �  �  w�  �  n�  e  N�  �  ��  k  ��  
  	�  �  (�  �   ϯ  E   c�  ]	  ��  �   �  h  غ  Y  A�  �  ��  �   _�     �      �     3�     4�  �   T�     7�     O�     f�  $   ��  '   ��  �   ��  N  ��     ��  %   ��  �   �  m   �  3   ��  z   ��  U   2�  X   ��  �   ��  ~   }�  @   ��     =�     V�  (   c�     ��     ��  M   ��      �  "   �  2   9�  7   l�     ��  �   ��  .   ;�  Z   j�  �   ��  �   ��  j   ��  i    �  �  j�  R   �  E   m�  }   ��  3   1�  7   e�  B   ��     ��     ��                 5   1   ;   &   /         *   <      B       2                        D   8   7      #   >                         0          F      6       "       C      (   G             +   ?   E              	           ,         $          !             3   4       
   A         -   .           :   9            =   )                        '   @   %    
[root@pcmk-1 ~]# <userinput>configure clone WebIP ClusterIP  \</userinput>
<userinput>        meta globally-unique=”true” clone-max=”2” clone-node-max=”2”</userinput>
     
[root@pcmk-1 ~]# <userinput>crm </userinput>
crm(live)# <userinput>cib new active</userinput>
INFO: active shadow CIB created
crm(active)# <userinput>configure clone WebIP ClusterIP  \</userinput>
        meta globally-unique=”true” clone-max=”2” clone-node-max=”2”
crm(active)# <userinput>configure show</userinput>
node pcmk-1
node pcmk-2
primitive WebData ocf:linbit:drbd \
        params drbd_resource="wwwdata" \
        op monitor interval="60s"
primitive WebFS ocf:heartbeat:Filesystem \
        params device="/dev/drbd/by-res/wwwdata" directory="/var/www/html" fstype=”gfs2”
primitive WebSite ocf:heartbeat:apache \
        params configfile="/etc/httpd/conf/httpd.conf" \
        op monitor interval="1min"
primitive ClusterIP ocf:heartbeat:IPaddr2 \
        params ip=”192.168.122.101” cidr_netmask=”32” clusterip_hash=”sourceip” \
        op monitor interval="30s"
primitive dlm ocf:pacemaker:controld \
        op monitor interval="120s"
primitive gfs-control ocf:pacemaker:controld \
   params daemon=”gfs_controld.pcmk” args=”-g 0” \
        op monitor interval="120s"
ms WebDataClone WebData \
        meta master-max="1" master-node-max="1" clone-max="2" clone-node-max="1" notify="true"
<emphasis>clone WebIP ClusterIP \</emphasis>
<emphasis> meta globally-unique=”true” clone-max=”2” clone-node-max=”2”</emphasis>
clone dlm-clone dlm \
        meta interleave="true"
clone gfs-clone gfs-control \
        meta interleave="true"
colocation WebFS-with-gfs-control inf: WebFS gfs-clone
colocation WebSite-with-WebFS inf: WebSite WebFS
colocation fs_on_drbd inf: WebFS WebDataClone:Master
colocation gfs-with-dlm inf: gfs-clone dlm-clone
<emphasis>colocation website-with-ip inf: WebSite WebIP</emphasis>
order WebFS-after-WebData inf: WebDataClone:promote WebFS:start
order WebSite-after-WebFS inf: WebFS WebSite
<emphasis>order apache-after-ip inf: WebIP WebSite</emphasis>
order start-WebFS-after-gfs-control inf: gfs-clone WebFS
order start-gfs-after-dlm inf: dlm-clone gfs-clone
property $id="cib-bootstrap-options" \
        dc-version="1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7" \
        cluster-infrastructure="openais" \
        expected-quorum-votes=”2” \
        stonith-enabled="false" \
        no-quorum-policy="ignore"
rsc_defaults $id="rsc-options" \
        resource-stickiness=”100”
     
[root@pcmk-1 ~]# <userinput>crm</userinput>
[root@pcmk-1 ~]# <userinput>cib new active</userinput>
     
[root@pcmk-1 ~]# <userinput>crm</userinput>
crm(live)# <userinput>cib new GFS2</userinput>
INFO: GFS2 shadow CIB created
crm(GFS2)# <userinput>configure delete WebFS</userinput>
crm(GFS2)# <userinput>configure primitive WebFS ocf:heartbeat:Filesystem params device="/dev/drbd/by-res/wwwdata" directory="/var/www/html" fstype=”gfs2”</userinput>
 
[root@pcmk-1 ~]# <userinput>crm</userinput>
crm(live)# <userinput>cib new gfs-glue --force</userinput>
INFO: gfs-glue shadow CIB created
crm(gfs-glue)# <userinput>configure primitive gfs-control ocf:pacemaker:controld params daemon=gfs_controld.pcmk args="-g 0" op monitor interval=120s</userinput>
crm(gfs-glue)# <userinput>configure clone gfs-clone gfs-control meta interleave=true</userinput>
 
[root@pcmk-1 ~]# <userinput>crm</userinput>
crm(live)# <userinput>cib new stack-glue</userinput>
INFO: stack-glue shadow CIB created
crm(stack-glue)# <userinput>configure primitive dlm ocf:pacemaker:controld op monitor interval=120s</userinput>
crm(stack-glue)# <userinput>configure clone dlm-clone dlm meta interleave=true</userinput>
crm(stack-glue)# <userinput>configure show xml</userinput>
crm(stack-glue)# <userinput>configure show</userinput>
node pcmk-1
node pcmk-2
primitive WebData ocf:linbit:drbd \
        params drbd_resource="wwwdata" \
        op monitor interval="60s"
primitive WebFS ocf:heartbeat:Filesystem \
        params device="/dev/drbd/by-res/wwwdata" directory="/var/www/html" fstype="ext4"
primitive WebSite ocf:heartbeat:apache \
        params configfile="/etc/httpd/conf/httpd.conf" \
        op monitor interval="1min"
primitive ClusterIP ocf:heartbeat:IPaddr2 \
        params ip="192.168.122.101" cidr_netmask="32" \
        op monitor interval="30s"
<emphasis>primitive dlm ocf:pacemaker:controld \</emphasis>
<emphasis> op monitor interval="120s"</emphasis>
ms WebDataClone WebData \
        meta master-max="1" master-node-max="1" clone-max="2" clone-node-max="1" notify="true"
<emphasis>clone dlm-clone dlm \</emphasis>
<emphasis> meta interleave="true"</emphasis>
location prefer-pcmk-1 WebSite 50: pcmk-1
colocation WebSite-with-WebFS inf: WebSite WebFS
colocation fs_on_drbd inf: WebFS WebDataClone:Master
colocation website-with-ip inf: WebSite ClusterIP
order WebFS-after-WebData inf: WebDataClone:promote WebFS:start
order WebSite-after-WebFS inf: WebFS WebSite
order apache-after-ip inf: ClusterIP WebSite
property $id="cib-bootstrap-options" \
        dc-version="1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7" \
        cluster-infrastructure="openais" \
        expected-quorum-votes=”2” \
        stonith-enabled="false" \
        no-quorum-policy="ignore"
rsc_defaults $id="rsc-options" \
        resource-stickiness=”100”
 
[root@pcmk-1 ~]# <userinput>crm_resource --resource WebFS --set-parameter target-role --meta --parameter-value Stopped</userinput>
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Thu Sep  3 15:18:06 2009
Stack: openais
Current DC: pcmk-1 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
6 Resources configured.
============

Online: [ pcmk-1 pcmk-2 ]

Master/Slave Set: WebDataClone
        Masters: [ pcmk-1 ]
        Slaves: [ pcmk-2 ]
ClusterIP        (ocf::heartbeat:IPaddr):        Started pcmk-1
Clone Set: dlm-clone
        Started: [ pcmk-2 pcmk-1 ]
Clone Set: gfs-clone
        Started: [ pcmk-2 pcmk-1 ]
 
[root@pcmk-1 ~]# <userinput>mount /dev/drbd1 /mnt/</userinput>
[root@pcmk-1 ~]# <userinput>cat &lt;&lt;-END &gt;/mnt/index.html</userinput>
&lt;html&gt;
&lt;body&gt;My Test Site - GFS2&lt;/body&gt;
&lt;/html&gt;
END
[root@pcmk-1 ~]# <userinput>umount /dev/drbd1</userinput>
[root@pcmk-1 ~]# <userinput>drbdadm verify wwwdata</userinput>
[root@pcmk-1 ~]#
 
[root@pcmk-1 ~]# <userinput>yum install -y gfs2-utils gfs-pcmk</userinput>
Setting up Install Process
Resolving Dependencies
--&gt; Running transaction check
---&gt; Package gfs-pcmk.x86_64 0:3.0.5-2.fc12 set to be updated
--&gt; Processing Dependency: libSaCkpt.so.3(OPENAIS_CKPT_B.01.01)(64bit) for package: gfs-pcmk-3.0.5-2.fc12.x86_64
--&gt; Processing Dependency: dlm-pcmk for package: gfs-pcmk-3.0.5-2.fc12.x86_64
--&gt; Processing Dependency: libccs.so.3()(64bit) for package: gfs-pcmk-3.0.5-2.fc12.x86_64
--&gt; Processing Dependency: libdlmcontrol.so.3()(64bit) for package: gfs-pcmk-3.0.5-2.fc12.x86_64
--&gt; Processing Dependency: liblogthread.so.3()(64bit) for package: gfs-pcmk-3.0.5-2.fc12.x86_64
--&gt; Processing Dependency: libSaCkpt.so.3()(64bit) for package: gfs-pcmk-3.0.5-2.fc12.x86_64
---&gt; Package gfs2-utils.x86_64 0:3.0.5-2.fc12 set to be updated
--&gt; Running transaction check
---&gt; Package clusterlib.x86_64 0:3.0.5-2.fc12 set to be updated
---&gt; Package dlm-pcmk.x86_64 0:3.0.5-2.fc12 set to be updated
---&gt; Package openaislib.x86_64 0:1.1.0-1.fc12 set to be updated
--&gt; Finished Dependency Resolution

Dependencies Resolved

===========================================================================================
 Package                Arch               Version                   Repository        Size
===========================================================================================
Installing:
 gfs-pcmk               x86_64             3.0.5-2.fc12              custom           101 k
 gfs2-utils             x86_64             3.0.5-2.fc12              custom           208 k
Installing for dependencies:
 clusterlib             x86_64             3.0.5-2.fc12              custom            65 k
 dlm-pcmk               x86_64             3.0.5-2.fc12              custom            93 k
 openaislib             x86_64             1.1.0-1.fc12              fedora            76 k

Transaction Summary
===========================================================================================
Install       5 Package(s)
Upgrade       0 Package(s)

Total download size: 541 k
Downloading Packages:
(1/5): clusterlib-3.0.5-2.fc12.x86_64.rpm                                |  65 kB     00:00
(2/5): dlm-pcmk-3.0.5-2.fc12.x86_64.rpm                                  |  93 kB     00:00
(3/5): gfs-pcmk-3.0.5-2.fc12.x86_64.rpm                                  | 101 kB     00:00
(4/5): gfs2-utils-3.0.5-2.fc12.x86_64.rpm                                | 208 kB     00:00
(5/5): openaislib-1.1.0-1.fc12.x86_64.rpm                                |  76 kB     00:00
-------------------------------------------------------------------------------------------
Total                                                           992 kB/s | 541 kB     00:00
Running rpm_check_debug
Running Transaction Test
Finished Transaction Test
Transaction Test Succeeded
Running Transaction
  Installing     : clusterlib-3.0.5-2.fc12.x86_64                                       1/5 
  Installing     : openaislib-1.1.0-1.fc12.x86_64                                       2/5 
  Installing     : dlm-pcmk-3.0.5-2.fc12.x86_64                                         3/5 
  Installing     : gfs-pcmk-3.0.5-2.fc12.x86_64                                         4/5 
  Installing     : gfs2-utils-3.0.5-2.fc12.x86_64                                       5/5 

Installed:
  gfs-pcmk.x86_64 0:3.0.5-2.fc12                    gfs2-utils.x86_64 0:3.0.5-2.fc12

Dependency Installed:
  clusterlib.x86_64 0:3.0.5-2.fc12   dlm-pcmk.x86_64 0:3.0.5-2.fc12 
  openaislib.x86_64 0:1.1.0-1.fc12  

Complete!
[root@pcmk-1 x86_64]#
 
crm(GFS2)# <userinput>cib commit GFS2</userinput>
INFO: commited 'GFS2' shadow CIB to the cluster
crm(GFS2)# <userinput>quit</userinput>
bye
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Thu Sep  3 20:49:54 2009
Stack: openais
Current DC: pcmk-2 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
6 Resources configured.
============

Online: [ pcmk-1 pcmk-2 ]

WebSite (ocf::heartbeat:apache):        Started pcmk-2
Master/Slave Set: WebDataClone
        Masters: [ pcmk-1 ]
        Slaves: [ pcmk-2 ]
ClusterIP        (ocf::heartbeat:IPaddr):        Started pcmk-2
Clone Set: dlm-clone
        Started: [ pcmk-2 pcmk-1 ]
Clone Set: gfs-clone
        Started: [ pcmk-2 pcmk-1 ]
<emphasis>WebFS (ocf::heartbeat:Filesystem): Started pcmk-1</emphasis>
 
crm(GFS2)# <userinput>configure colocation WebSite-with-WebFS inf: WebSite WebFS</userinput>
crm(GFS2)# <userinput>configure colocation fs_on_drbd inf: WebFS WebDataClone:Master</userinput>
crm(GFS2)# <userinput>configure order WebFS-after-WebData inf: WebDataClone:promote WebFS:start</userinput>
crm(GFS2)# <userinput>configure order WebSite-after-WebFS inf: WebFS WebSite</userinput>
crm(GFS2)# <userinput>configure colocation WebFS-with-gfs-control INFINITY: WebFS gfs-clone</userinput>
crm(GFS2)# <userinput>configure order start-WebFS-after-gfs-control mandatory: gfs-clone WebFS</userinput>
crm(GFS2)# <userinput>configure show</userinput>
node pcmk-1
node pcmk-2
primitive WebData ocf:linbit:drbd \
        params drbd_resource="wwwdata" \
        op monitor interval="60s"
<emphasis>primitive WebFS ocf:heartbeat:Filesystem \</emphasis>
<emphasis> params device="/dev/drbd/by-res/wwwdata" directory="/var/www/html" fstype=”gfs2”</emphasis>
primitive WebSite ocf:heartbeat:apache \
        params configfile="/etc/httpd/conf/httpd.conf" \
        op monitor interval="1min"
primitive ClusterIP ocf:heartbeat:IPaddr2 \
        params ip="192.168.122.101" cidr_netmask="32" \
        op monitor interval="30s"
primitive dlm ocf:pacemaker:controld \
        op monitor interval="120s"
primitive gfs-control ocf:pacemaker:controld \
   params daemon=”gfs_controld.pcmk” args=”-g 0” \
        op monitor interval="120s"
ms WebDataClone WebData \
        meta master-max="1" master-node-max="1" clone-max="2" clone-node-max="1" notify="true"
clone dlm-clone dlm \
        meta interleave="true"
clone gfs-clone gfs-control \
        meta interleave="true"
colocation WebFS-with-gfs-control inf: WebFS gfs-clone
colocation WebSite-with-WebFS inf: WebSite WebFS
colocation fs_on_drbd inf: WebFS WebDataClone:Master
colocation gfs-with-dlm inf: gfs-clone dlm-clone
colocation website-with-ip inf: WebSite ClusterIP
order WebFS-after-WebData inf: WebDataClone:promote WebFS:start
order WebSite-after-WebFS inf: WebFS WebSite
order apache-after-ip inf: ClusterIP WebSite
order start-WebFS-after-gfs-control inf: gfs-clone WebFS
order start-gfs-after-dlm inf: dlm-clone gfs-clone
property $id="cib-bootstrap-options" \
        dc-version="1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7" \
        cluster-infrastructure="openais" \
        expected-quorum-votes=”2” \
        stonith-enabled="false" \
        no-quorum-policy="ignore"
rsc_defaults $id="rsc-options" \
        resource-stickiness=”100”
 
crm(active)# <userinput>cib commit active</userinput>
INFO: commited 'active' shadow CIB to the cluster
crm(active)# <userinput>quit</userinput>
bye
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Thu Sep  3 21:37:27 2009
Stack: openais
Current DC: pcmk-2 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
6 Resources configured.
============

Online: [ pcmk-1 pcmk-2 ]

Master/Slave Set: WebDataClone
        Masters: [ pcmk-1 pcmk-2 ]
Clone Set: dlm-clone
        Started: [ pcmk-2 pcmk-1 ]
Clone Set: gfs-clone
        Started: [ pcmk-2 pcmk-1 ]
<emphasis>Clone Set: WebIP</emphasis>
<emphasis> Started: [ pcmk-1 pcmk-2 ]</emphasis>
<emphasis>Clone Set: WebFSClone</emphasis>
<emphasis> Started: [ pcmk-1 pcmk-2 ]</emphasis>
<emphasis>Clone Set: WebSiteClone</emphasis>
<emphasis> Started: [ pcmk-1 pcmk-2 ]</emphasis>
     
crm(active)# <userinput>configure clone WebFSClone WebFS</userinput>
crm(active)# <userinput>configure clone WebSiteClone WebSite</userinput>
     
crm(active)# <userinput>configure edit WebDataClone</userinput>
     
crm(active)# <userinput>configure show</userinput>
node pcmk-1
node pcmk-2
primitive WebData ocf:linbit:drbd \
        params drbd_resource="wwwdata" \
        op monitor interval="60s"
primitive WebFS ocf:heartbeat:Filesystem \
        params device="/dev/drbd/by-res/wwwdata" directory="/var/www/html" fstype=”gfs2”
primitive WebSite ocf:heartbeat:apache \
        params configfile="/etc/httpd/conf/httpd.conf" \
        op monitor interval="1min"
primitive ClusterIP ocf:heartbeat:IPaddr2 \
        params ip=”192.168.122.101” cidr_netmask=”32” clusterip_hash=”sourceip” \
        op monitor interval="30s"
primitive dlm ocf:pacemaker:controld \
        op monitor interval="120s"
primitive gfs-control ocf:pacemaker:controld \
   params daemon=”gfs_controld.pcmk” args=”-g 0” \
        op monitor interval="120s"
ms WebDataClone WebData \
        meta master-max="2" master-node-max="1" clone-max="2" clone-node-max="1" notify="true"
<emphasis>clone WebFSClone WebFS</emphasis>
clone WebIP ClusterIP  \
        meta globally-unique=”true” clone-max=”2” clone-node-max=”2”
<emphasis>clone WebSiteClone WebSite</emphasis>
clone dlm-clone dlm \
        meta interleave="true"
clone gfs-clone gfs-control \
        meta interleave="true"
<emphasis>colocation WebFS-with-gfs-control inf: WebFSClone gfs-clone</emphasis>
<emphasis>colocation WebSite-with-WebFS inf: WebSiteClone WebFSClone</emphasis>
<emphasis>colocation fs_on_drbd inf: WebFSClone WebDataClone:Master</emphasis>
colocation gfs-with-dlm inf: gfs-clone dlm-clone
<emphasis>colocation website-with-ip inf: WebSiteClone WebIP</emphasis>
<emphasis>order WebFS-after-WebData inf: WebDataClone:promote WebFSClone:start</emphasis>
<emphasis>order WebSite-after-WebFS inf: WebFSClone WebSiteClone</emphasis>
<emphasis>order apache-after-ip inf: WebIP WebSiteClone</emphasis>
order start-WebFS-after-gfs-control inf: gfs-clone WebFSClone
order start-gfs-after-dlm inf: dlm-clone gfs-clone
property $id="cib-bootstrap-options" \
        dc-version="1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7" \
        cluster-infrastructure="openais" \
        expected-quorum-votes=”2” \
        stonith-enabled="false" \
        no-quorum-policy="ignore"
rsc_defaults $id="rsc-options" \
        resource-stickiness=”100”
     
crm(gfs-glue)# <userinput>configure colocation gfs-with-dlm INFINITY: gfs-clone dlm-clone</userinput>
crm(gfs-glue)# <userinput>configure order start-gfs-after-dlm mandatory: dlm-clone gfs-clone</userinput>
 
crm(gfs-glue)# <userinput>configure show</userinput>
node pcmk-1
node pcmk-2
primitive WebData ocf:linbit:drbd \
        params drbd_resource="wwwdata" \
        op monitor interval="60s"
primitive WebFS ocf:heartbeat:Filesystem \
        params device="/dev/drbd/by-res/wwwdata" directory="/var/www/html" fstype="ext4"
primitive WebSite ocf:heartbeat:apache \
        params configfile="/etc/httpd/conf/httpd.conf" \
        op monitor interval="1min"
primitive ClusterIP ocf:heartbeat:IPaddr2 \
        params ip="192.168.122.101" cidr_netmask="32" \
        op monitor interval="30s"
primitive dlm ocf:pacemaker:controld \
        op monitor interval="120s"
<emphasis>primitive gfs-control ocf:pacemaker:controld \</emphasis>
<emphasis> params daemon=”gfs_controld.pcmk” args=”-g 0” \</emphasis>
<emphasis> op monitor interval="120s"</emphasis>
ms WebDataClone WebData \
        meta master-max="1" master-node-max="1" clone-max="2" clone-node-max="1" notify="true"
clone dlm-clone dlm \
        meta interleave="true"
<emphasis>clone gfs-clone gfs-control \</emphasis>
<emphasis> meta interleave="true"</emphasis>
location prefer-pcmk-1 WebSite 50: pcmk-1
colocation WebSite-with-WebFS inf: WebSite WebFS
colocation fs_on_drbd inf: WebFS WebDataClone:Master
<emphasis>colocation gfs-with-dlm inf: gfs-clone dlm-clone</emphasis>
colocation website-with-ip inf: WebSite ClusterIP
order WebFS-after-WebData inf: WebDataClone:promote WebFS:start
order WebSite-after-WebFS inf: WebFS WebSite
order apache-after-ip inf: ClusterIP WebSite
<emphasis>order start-gfs-after-dlm inf: dlm-clone gfs-clone</emphasis>
property $id="cib-bootstrap-options" \
        dc-version="1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7" \
        cluster-infrastructure="openais" \
        expected-quorum-votes=”2” \
        stonith-enabled="false" \
        no-quorum-policy="ignore"
rsc_defaults $id="rsc-options" \
        resource-stickiness=”100”
crm(gfs-glue)# <userinput>cib commit gfs-glue</userinput>
INFO: commited 'gfs-glue' shadow CIB to the cluster
crm(gfs-glue)# <userinput>quit</userinput>
bye
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Thu Sep  3 20:49:54 2009
Stack: openais
Current DC: pcmk-2 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
6 Resources configured.
============

Online: [ pcmk-1 pcmk-2 ]

WebSite (ocf::heartbeat:apache):        Started pcmk-2
Master/Slave Set: WebDataClone
        Masters: [ pcmk-1 ]
        Slaves: [ pcmk-2 ]
ClusterIP        (ocf::heartbeat:IPaddr):        Started pcmk-2
Clone Set: dlm-clone
        Started: [ pcmk-2 pcmk-1 ]
<emphasis>Clone Set: gfs-clone</emphasis>
<emphasis> Started: [ pcmk-2 pcmk-1 ]</emphasis>
WebFS   (ocf::heartbeat:Filesystem):    Started pcmk-1
 
crm(stack-glue)# <userinput>cib commit stack-glue</userinput>
INFO: commited 'stack-glue' shadow CIB to the cluster
crm(stack-glue)# <userinput>quit</userinput>
bye
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Thu Sep  3 20:49:54 2009
Stack: openais
Current DC: pcmk-2 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
5 Resources configured.
============

Online: [ pcmk-1 pcmk-2 ]

WebSite (ocf::heartbeat:apache):        Started pcmk-2
Master/Slave Set: WebDataClone
        Masters: [ pcmk-1 ]
        Slaves: [ pcmk-2 ]
ClusterIP        (ocf::heartbeat:IPaddr):        Started pcmk-2
<emphasis>Clone Set: dlm-clone</emphasis>
<emphasis> Started: [ pcmk-2 pcmk-1 ]</emphasis>
WebFS   (ocf::heartbeat:Filesystem):    Started pcmk-2
 
mkfs.gfs2 -p lock_dlm -j 2 -t pcmk:web /dev/drbd1
[root@pcmk-1 ~]# <userinput>mkfs.gfs2 -t pcmk:web -p lock_dlm -j 2 /dev/vdb </userinput>
This will destroy any data on /dev/vdb.
It appears to contain: data

Are you sure you want to proceed? [y/n] y

Device:                    /dev/vdb
Blocksize:                 4096
Device Size                1.00 GB (131072 blocks)
Filesystem Size:           1.00 GB (131070 blocks)
Journals:                  2
Resource Groups:           2
Locking Protocol:          "lock_dlm"
Lock Table:                "pcmk:web"
UUID:                      6B776F46-177B-BAF8-2C2B-292C0E078613

[root@pcmk-1 ~]#
 
primitive ClusterIP ocf:heartbeat:IPaddr2 \ 
        params ip="192.168.122.101" cidr_netmask="32" clusterip_hash="sourceip" \
        op monitor interval="30s"
     Add the DLM service Add the GFS2 service Almost everything is in place. Recent versions of DRBD are capable of operating in Primary/Primary mode and the filesystem we’re using is cluster aware. All we need to do now is reconfigure the cluster to take advantage of this. And add the following to the params line Before we do anything to the existing partition, we need to make sure it is unmounted. We do this by tell the cluster to stop the WebFS resource. This will ensure that other resources (in our case, Apache) using WebFS are not only stopped, but stopped in the correct order. Change master-max to 2 Conversion to Active/Active Create a GFS2 Filesystem Create and Populate an GFS2 Partition Do this on each node in the cluster and be sure to restart them before continuing. First we must use the -p option to specify that we want to use the the Kernel’s DLM. Next we use -j to indicate that it should reserve enough space for two journals (one per node accessing the filesystem). GFS2 needs two services to be running, the first is the user-space interface to the kernel’s distributed lock manager (DLM). The DLM is used to co-ordinate which node(s) can access a given file (and when) and integrates with Pacemaker to obtain node membership <footnote> <para> The list of nodes the cluster considers to be available </para> </footnote> information and fencing capabilities. Here is the full transcript Install a Cluster Filesystem - GFS2 Lastly, we use -t to specify the lock table name. The format for this field is clustername:fsname. For the fsname, we just need to pick something unique and descriptive and since we haven’t specified a clustername yet, we will use the default (pcmk). Next we need to convert the filesystem and Apache resources into clones. Again, the shell will automatically update any relevant constraints. Note that both Apache and WebFS have been stopped. Notice how any constraints that referenced ClusterIP have been updated to use WebIP instead. This is an additional benefit of using the crm shell. Now ensure Pacemaker only starts the gfs-control service on nodes that also have a copy of the dlm service (created above) already running Now that the cluster stack and integration pieces are running smoothly, we can create an GFS2 partition. Now that we’ve recreated the resource, we also need to recreate all the constraints that used it. This is because the shell will automatically remove any constraints that referenced WebFS. Now we must tell the ClusterIP how to decide which requests are processed by which hosts. To do this we must specify the clusterip_hash parameter. Once the DLM is active, we can add the GFS2 control daemon. Open the ClusterIP resource Preparation Reconfigure Pacemaker for Active/Active Reconfigure the Cluster for GFS2 Requirements Review the configuration before uploading it to the cluster, quitting the shell and watching the cluster’s response Setup Pacemaker-GFS2 Integration So that the complete definition looks like: TODO: Explain the meaning of the interleave option TODO: Put one node into standby to demonstrate failover Testing Recovery The DLM control daemon needs to run on all active cluster nodes, so we will use the shells interactive mode to create a cloned resource. The first thing to do is install gfs2-utils on each machine. The last step is to tell the cluster that it is now allowed to promote both instances to be Primary (aka. Master). The only hitch is that we need to use a cluster-aware filesystem (and the one we used earlier with DRBD, ext4, is not one of those). Both OCFS2 and GFS2 are supported, however here we will use GFS2 which comes with &DISTRO; &amp;DISTRO_VERSION; . The primary requirement for an Active/Active cluster is that the data required for your services are available, simultaneously, on both machines. Pacemaker makes no requirement on how this is achieved, you could use a SAN if you had one available, however since DRBD supports multiple Primaries, we can also use that. The second service is GFS2’s own control daemon which also integrates with Pacemaker to obtain node membership data. Then (re)populate the new filesystem with data (web pages). For now we’ll create another variation on our home page. There’s no point making the services active on both locations if we can’t reach them, so lets first clone the IP address. Cloned IPaddr2 resources use an iptables rule to ensure that each request only processed by one of the two clone instances. The additional meta options tell the cluster how many instances of the clone we want (one “request bucket” for each node) and that if all other nodes fail, then the remaining node should hold all of them. Otherwise the requests would be simply discarded. This will erase all previous content stored on the DRBD device. Ensure you have a copy of any important data. This will involve a number of changes, so we’ll again use interactive mode. To specify an alternate name for the cluster, locate the service section containing “name: pacemaker” in corosync.conf and insert the following line anywhere inside the block: Use the crm shell to create the gfs-control cluster resource: We need to specify a number of additional parameters when creating a GFS2 partition. [root@pcmk-1 ~]# <userinput>configure edit  ClusterIP</userinput> clusterip_hash="sourceip" clustername: myname Project-Id-Version: 0
POT-Creation-Date: 2010-12-15T23:32:36
PO-Revision-Date: 2010-12-16 00:37+0800
Last-Translator: Charlie Chen <laneovcc@gmail.com>
Language-Team: None
Language: 
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit
X-Poedit-Language: Chinese
X-Poedit-Country: CHINA
X-Poedit-SourceCharset: utf-8
 
[root@pcmk-1 ~]# <userinput>configure clone WebIP ClusterIP  \</userinput>
<userinput>        meta globally-unique=”true” clone-max=”2” clone-node-max=”2”</userinput>
     
[root@pcmk-1 ~]# <userinput>crm </userinput>
crm(live)# <userinput>cib new active</userinput>
INFO: active shadow CIB created
crm(active)# <userinput>configure clone WebIP ClusterIP  \</userinput>
        meta globally-unique=”true” clone-max=”2” clone-node-max=”2”
crm(active)# <userinput>configure show</userinput>
node pcmk-1
node pcmk-2
primitive WebData ocf:linbit:drbd \
        params drbd_resource="wwwdata" \
        op monitor interval="60s"
primitive WebFS ocf:heartbeat:Filesystem \
        params device="/dev/drbd/by-res/wwwdata" directory="/var/www/html" fstype=”gfs2”
primitive WebSite ocf:heartbeat:apache \
        params configfile="/etc/httpd/conf/httpd.conf" \
        op monitor interval="1min"
primitive ClusterIP ocf:heartbeat:IPaddr2 \
        params ip=”192.168.122.101” cidr_netmask=”32” clusterip_hash=”sourceip” \
        op monitor interval="30s"
primitive dlm ocf:pacemaker:controld \
        op monitor interval="120s"
primitive gfs-control ocf:pacemaker:controld \
   params daemon=”gfs_controld.pcmk” args=”-g 0” \
        op monitor interval="120s"
ms WebDataClone WebData \
        meta master-max="1" master-node-max="1" clone-max="2" clone-node-max="1" notify="true"
<emphasis>clone WebIP ClusterIP \</emphasis>
<emphasis> meta globally-unique=”true” clone-max=”2” clone-node-max=”2”</emphasis>
clone dlm-clone dlm \
        meta interleave="true"
clone gfs-clone gfs-control \
        meta interleave="true"
colocation WebFS-with-gfs-control inf: WebFS gfs-clone
colocation WebSite-with-WebFS inf: WebSite WebFS
colocation fs_on_drbd inf: WebFS WebDataClone:Master
colocation gfs-with-dlm inf: gfs-clone dlm-clone
<emphasis>colocation website-with-ip inf: WebSite WebIP</emphasis>
order WebFS-after-WebData inf: WebDataClone:promote WebFS:start
order WebSite-after-WebFS inf: WebFS WebSite
<emphasis>order apache-after-ip inf: WebIP WebSite</emphasis>
order start-WebFS-after-gfs-control inf: gfs-clone WebFS
order start-gfs-after-dlm inf: dlm-clone gfs-clone
property $id="cib-bootstrap-options" \
        dc-version="1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7" \
        cluster-infrastructure="openais" \
        expected-quorum-votes=”2” \
        stonith-enabled="false" \
        no-quorum-policy="ignore"
rsc_defaults $id="rsc-options" \
        resource-stickiness=”100”
     
[root@pcmk-1 ~]# <userinput>crm</userinput>
[root@pcmk-1 ~]# <userinput>cib new active</userinput>
     
[root@pcmk-1 ~]# <userinput>crm</userinput>
crm(live)# <userinput>cib new GFS2</userinput>
INFO: GFS2 shadow CIB created
crm(GFS2)# <userinput>configure delete WebFS</userinput>
crm(GFS2)# <userinput>configure primitive WebFS ocf:heartbeat:Filesystem params device="/dev/drbd/by-res/wwwdata" directory="/var/www/html" fstype=”gfs2”</userinput>
 
[root@pcmk-1 ~]# <userinput>crm</userinput>
crm(live)# <userinput>cib new gfs-glue --force</userinput>
INFO: gfs-glue shadow CIB created
crm(gfs-glue)# <userinput>configure primitive gfs-control ocf:pacemaker:controld params daemon=gfs_controld.pcmk args="-g 0" op monitor interval=120s</userinput>
crm(gfs-glue)# <userinput>configure clone gfs-clone gfs-control meta interleave=true</userinput>
 
[root@pcmk-1 ~]# <userinput>crm</userinput>
crm(live)# <userinput>cib new stack-glue</userinput>
INFO: stack-glue shadow CIB created
crm(stack-glue)# <userinput>configure primitive dlm ocf:pacemaker:controld op monitor interval=120s</userinput>
crm(stack-glue)# <userinput>configure clone dlm-clone dlm meta interleave=true</userinput>
crm(stack-glue)# <userinput>configure show xml</userinput>
crm(stack-glue)# <userinput>configure show</userinput>
node pcmk-1
node pcmk-2
primitive WebData ocf:linbit:drbd \
        params drbd_resource="wwwdata" \
        op monitor interval="60s"
primitive WebFS ocf:heartbeat:Filesystem \
        params device="/dev/drbd/by-res/wwwdata" directory="/var/www/html" fstype="ext4"
primitive WebSite ocf:heartbeat:apache \
        params configfile="/etc/httpd/conf/httpd.conf" \
        op monitor interval="1min"
primitive ClusterIP ocf:heartbeat:IPaddr2 \
        params ip="192.168.122.101" cidr_netmask="32" \
        op monitor interval="30s"
<emphasis>primitive dlm ocf:pacemaker:controld \</emphasis>
<emphasis> op monitor interval="120s"</emphasis>
ms WebDataClone WebData \
        meta master-max="1" master-node-max="1" clone-max="2" clone-node-max="1" notify="true"
<emphasis>clone dlm-clone dlm \</emphasis>
<emphasis> meta interleave="true"</emphasis>
location prefer-pcmk-1 WebSite 50: pcmk-1
colocation WebSite-with-WebFS inf: WebSite WebFS
colocation fs_on_drbd inf: WebFS WebDataClone:Master
colocation website-with-ip inf: WebSite ClusterIP
order WebFS-after-WebData inf: WebDataClone:promote WebFS:start
order WebSite-after-WebFS inf: WebFS WebSite
order apache-after-ip inf: ClusterIP WebSite
property $id="cib-bootstrap-options" \
        dc-version="1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7" \
        cluster-infrastructure="openais" \
        expected-quorum-votes=”2” \
        stonith-enabled="false" \
        no-quorum-policy="ignore"
rsc_defaults $id="rsc-options" \
        resource-stickiness=”100”
 
[root@pcmk-1 ~]# <userinput>crm_resource --resource WebFS --set-parameter target-role --meta --parameter-value Stopped</userinput>
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Thu Sep  3 15:18:06 2009
Stack: openais
Current DC: pcmk-1 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
6 Resources configured.
============

Online: [ pcmk-1 pcmk-2 ]

Master/Slave Set: WebDataClone
        Masters: [ pcmk-1 ]
        Slaves: [ pcmk-2 ]
ClusterIP        (ocf::heartbeat:IPaddr):        Started pcmk-1
Clone Set: dlm-clone
        Started: [ pcmk-2 pcmk-1 ]
Clone Set: gfs-clone
        Started: [ pcmk-2 pcmk-1 ]
 
[root@pcmk-1 ~]# <userinput>mount /dev/drbd1 /mnt/</userinput>
[root@pcmk-1 ~]# <userinput>cat &lt;&lt;-END &gt;/mnt/index.html</userinput>
&lt;html&gt;
&lt;body&gt;My Test Site - GFS2&lt;/body&gt;
&lt;/html&gt;
END
[root@pcmk-1 ~]# <userinput>umount /dev/drbd1</userinput>
[root@pcmk-1 ~]# <userinput>drbdadm verify wwwdata</userinput>
[root@pcmk-1 ~]#
 
[root@pcmk-1 ~]# <userinput>yum install -y gfs2-utils gfs-pcmk</userinput>
Setting up Install Process
Resolving Dependencies
--&gt; Running transaction check
---&gt; Package gfs-pcmk.x86_64 0:3.0.5-2.fc12 set to be updated
--&gt; Processing Dependency: libSaCkpt.so.3(OPENAIS_CKPT_B.01.01)(64bit) for package: gfs-pcmk-3.0.5-2.fc12.x86_64
--&gt; Processing Dependency: dlm-pcmk for package: gfs-pcmk-3.0.5-2.fc12.x86_64
--&gt; Processing Dependency: libccs.so.3()(64bit) for package: gfs-pcmk-3.0.5-2.fc12.x86_64
--&gt; Processing Dependency: libdlmcontrol.so.3()(64bit) for package: gfs-pcmk-3.0.5-2.fc12.x86_64
--&gt; Processing Dependency: liblogthread.so.3()(64bit) for package: gfs-pcmk-3.0.5-2.fc12.x86_64
--&gt; Processing Dependency: libSaCkpt.so.3()(64bit) for package: gfs-pcmk-3.0.5-2.fc12.x86_64
---&gt; Package gfs2-utils.x86_64 0:3.0.5-2.fc12 set to be updated
--&gt; Running transaction check
---&gt; Package clusterlib.x86_64 0:3.0.5-2.fc12 set to be updated
---&gt; Package dlm-pcmk.x86_64 0:3.0.5-2.fc12 set to be updated
---&gt; Package openaislib.x86_64 0:1.1.0-1.fc12 set to be updated
--&gt; Finished Dependency Resolution

Dependencies Resolved

===========================================================================================
 Package                Arch               Version                   Repository        Size
===========================================================================================
Installing:
 gfs-pcmk               x86_64             3.0.5-2.fc12              custom           101 k
 gfs2-utils             x86_64             3.0.5-2.fc12              custom           208 k
Installing for dependencies:
 clusterlib             x86_64             3.0.5-2.fc12              custom            65 k
 dlm-pcmk               x86_64             3.0.5-2.fc12              custom            93 k
 openaislib             x86_64             1.1.0-1.fc12              fedora            76 k

Transaction Summary
===========================================================================================
Install       5 Package(s)
Upgrade       0 Package(s)

Total download size: 541 k
Downloading Packages:
(1/5): clusterlib-3.0.5-2.fc12.x86_64.rpm                                |  65 kB     00:00
(2/5): dlm-pcmk-3.0.5-2.fc12.x86_64.rpm                                  |  93 kB     00:00
(3/5): gfs-pcmk-3.0.5-2.fc12.x86_64.rpm                                  | 101 kB     00:00
(4/5): gfs2-utils-3.0.5-2.fc12.x86_64.rpm                                | 208 kB     00:00
(5/5): openaislib-1.1.0-1.fc12.x86_64.rpm                                |  76 kB     00:00
-------------------------------------------------------------------------------------------
Total                                                           992 kB/s | 541 kB     00:00
Running rpm_check_debug
Running Transaction Test
Finished Transaction Test
Transaction Test Succeeded
Running Transaction
  Installing     : clusterlib-3.0.5-2.fc12.x86_64                                       1/5 
  Installing     : openaislib-1.1.0-1.fc12.x86_64                                       2/5 
  Installing     : dlm-pcmk-3.0.5-2.fc12.x86_64                                         3/5 
  Installing     : gfs-pcmk-3.0.5-2.fc12.x86_64                                         4/5 
  Installing     : gfs2-utils-3.0.5-2.fc12.x86_64                                       5/5 

Installed:
  gfs-pcmk.x86_64 0:3.0.5-2.fc12                    gfs2-utils.x86_64 0:3.0.5-2.fc12

Dependency Installed:
  clusterlib.x86_64 0:3.0.5-2.fc12   dlm-pcmk.x86_64 0:3.0.5-2.fc12 
  openaislib.x86_64 0:1.1.0-1.fc12  

Complete!
[root@pcmk-1 x86_64]#
 
crm(GFS2)# <userinput>cib commit GFS2</userinput>
INFO: commited 'GFS2' shadow CIB to the cluster
crm(GFS2)# <userinput>quit</userinput>
bye
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Thu Sep  3 20:49:54 2009
Stack: openais
Current DC: pcmk-2 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
6 Resources configured.
============

Online: [ pcmk-1 pcmk-2 ]

WebSite (ocf::heartbeat:apache):        Started pcmk-2
Master/Slave Set: WebDataClone
        Masters: [ pcmk-1 ]
        Slaves: [ pcmk-2 ]
ClusterIP        (ocf::heartbeat:IPaddr):        Started pcmk-2
Clone Set: dlm-clone
        Started: [ pcmk-2 pcmk-1 ]
Clone Set: gfs-clone
        Started: [ pcmk-2 pcmk-1 ]
<emphasis>WebFS (ocf::heartbeat:Filesystem): Started pcmk-1</emphasis>
 
crm(GFS2)# <userinput>configure colocation WebSite-with-WebFS inf: WebSite WebFS</userinput>
crm(GFS2)# <userinput>configure colocation fs_on_drbd inf: WebFS WebDataClone:Master</userinput>
crm(GFS2)# <userinput>configure order WebFS-after-WebData inf: WebDataClone:promote WebFS:start</userinput>
crm(GFS2)# <userinput>configure order WebSite-after-WebFS inf: WebFS WebSite</userinput>
crm(GFS2)# <userinput>configure colocation WebFS-with-gfs-control INFINITY: WebFS gfs-clone</userinput>
crm(GFS2)# <userinput>configure order start-WebFS-after-gfs-control mandatory: gfs-clone WebFS</userinput>
crm(GFS2)# <userinput>configure show</userinput>
node pcmk-1
node pcmk-2
primitive WebData ocf:linbit:drbd \
        params drbd_resource="wwwdata" \
        op monitor interval="60s"
<emphasis>primitive WebFS ocf:heartbeat:Filesystem \</emphasis>
<emphasis> params device="/dev/drbd/by-res/wwwdata" directory="/var/www/html" fstype=”gfs2”</emphasis>
primitive WebSite ocf:heartbeat:apache \
        params configfile="/etc/httpd/conf/httpd.conf" \
        op monitor interval="1min"
primitive ClusterIP ocf:heartbeat:IPaddr2 \
        params ip="192.168.122.101" cidr_netmask="32" \
        op monitor interval="30s"
primitive dlm ocf:pacemaker:controld \
        op monitor interval="120s"
primitive gfs-control ocf:pacemaker:controld \
   params daemon=”gfs_controld.pcmk” args=”-g 0” \
        op monitor interval="120s"
ms WebDataClone WebData \
        meta master-max="1" master-node-max="1" clone-max="2" clone-node-max="1" notify="true"
clone dlm-clone dlm \
        meta interleave="true"
clone gfs-clone gfs-control \
        meta interleave="true"
colocation WebFS-with-gfs-control inf: WebFS gfs-clone
colocation WebSite-with-WebFS inf: WebSite WebFS
colocation fs_on_drbd inf: WebFS WebDataClone:Master
colocation gfs-with-dlm inf: gfs-clone dlm-clone
colocation website-with-ip inf: WebSite ClusterIP
order WebFS-after-WebData inf: WebDataClone:promote WebFS:start
order WebSite-after-WebFS inf: WebFS WebSite
order apache-after-ip inf: ClusterIP WebSite
order start-WebFS-after-gfs-control inf: gfs-clone WebFS
order start-gfs-after-dlm inf: dlm-clone gfs-clone
property $id="cib-bootstrap-options" \
        dc-version="1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7" \
        cluster-infrastructure="openais" \
        expected-quorum-votes=”2” \
        stonith-enabled="false" \
        no-quorum-policy="ignore"
rsc_defaults $id="rsc-options" \
        resource-stickiness=”100”
 
crm(active)# <userinput>cib commit active</userinput>
INFO: commited 'active' shadow CIB to the cluster
crm(active)# <userinput>quit</userinput>
bye
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Thu Sep  3 21:37:27 2009
Stack: openais
Current DC: pcmk-2 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
6 Resources configured.
============

Online: [ pcmk-1 pcmk-2 ]

Master/Slave Set: WebDataClone
        Masters: [ pcmk-1 pcmk-2 ]
Clone Set: dlm-clone
        Started: [ pcmk-2 pcmk-1 ]
Clone Set: gfs-clone
        Started: [ pcmk-2 pcmk-1 ]
<emphasis>Clone Set: WebIP</emphasis>
<emphasis> Started: [ pcmk-1 pcmk-2 ]</emphasis>
<emphasis>Clone Set: WebFSClone</emphasis>
<emphasis> Started: [ pcmk-1 pcmk-2 ]</emphasis>
<emphasis>Clone Set: WebSiteClone</emphasis>
<emphasis> Started: [ pcmk-1 pcmk-2 ]</emphasis>
     
crm(active)# <userinput>configure clone WebFSClone WebFS</userinput>
crm(active)# <userinput>configure clone WebSiteClone WebSite</userinput>
     
crm(active)# <userinput>configure edit WebDataClone</userinput>
     
crm(active)# <userinput>configure show</userinput>
node pcmk-1
node pcmk-2
primitive WebData ocf:linbit:drbd \
        params drbd_resource="wwwdata" \
        op monitor interval="60s"
primitive WebFS ocf:heartbeat:Filesystem \
        params device="/dev/drbd/by-res/wwwdata" directory="/var/www/html" fstype=”gfs2”
primitive WebSite ocf:heartbeat:apache \
        params configfile="/etc/httpd/conf/httpd.conf" \
        op monitor interval="1min"
primitive ClusterIP ocf:heartbeat:IPaddr2 \
        params ip=”192.168.122.101” cidr_netmask=”32” clusterip_hash=”sourceip” \
        op monitor interval="30s"
primitive dlm ocf:pacemaker:controld \
        op monitor interval="120s"
primitive gfs-control ocf:pacemaker:controld \
   params daemon=”gfs_controld.pcmk” args=”-g 0” \
        op monitor interval="120s"
ms WebDataClone WebData \
        meta master-max="2" master-node-max="1" clone-max="2" clone-node-max="1" notify="true"
<emphasis>clone WebFSClone WebFS</emphasis>
clone WebIP ClusterIP  \
        meta globally-unique=”true” clone-max=”2” clone-node-max=”2”
<emphasis>clone WebSiteClone WebSite</emphasis>
clone dlm-clone dlm \
        meta interleave="true"
clone gfs-clone gfs-control \
        meta interleave="true"
<emphasis>colocation WebFS-with-gfs-control inf: WebFSClone gfs-clone</emphasis>
<emphasis>colocation WebSite-with-WebFS inf: WebSiteClone WebFSClone</emphasis>
<emphasis>colocation fs_on_drbd inf: WebFSClone WebDataClone:Master</emphasis>
colocation gfs-with-dlm inf: gfs-clone dlm-clone
<emphasis>colocation website-with-ip inf: WebSiteClone WebIP</emphasis>
<emphasis>order WebFS-after-WebData inf: WebDataClone:promote WebFSClone:start</emphasis>
<emphasis>order WebSite-after-WebFS inf: WebFSClone WebSiteClone</emphasis>
<emphasis>order apache-after-ip inf: WebIP WebSiteClone</emphasis>
order start-WebFS-after-gfs-control inf: gfs-clone WebFSClone
order start-gfs-after-dlm inf: dlm-clone gfs-clone
property $id="cib-bootstrap-options" \
        dc-version="1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7" \
        cluster-infrastructure="openais" \
        expected-quorum-votes=”2” \
        stonith-enabled="false" \
        no-quorum-policy="ignore"
rsc_defaults $id="rsc-options" \
        resource-stickiness=”100”
     
crm(gfs-glue)# <userinput>configure colocation gfs-with-dlm INFINITY: gfs-clone dlm-clone</userinput>
crm(gfs-glue)# <userinput>configure order start-gfs-after-dlm mandatory: dlm-clone gfs-clone</userinput>
 
crm(gfs-glue)# <userinput>configure show</userinput>
node pcmk-1
node pcmk-2
primitive WebData ocf:linbit:drbd \
        params drbd_resource="wwwdata" \
        op monitor interval="60s"
primitive WebFS ocf:heartbeat:Filesystem \
        params device="/dev/drbd/by-res/wwwdata" directory="/var/www/html" fstype="ext4"
primitive WebSite ocf:heartbeat:apache \
        params configfile="/etc/httpd/conf/httpd.conf" \
        op monitor interval="1min"
primitive ClusterIP ocf:heartbeat:IPaddr2 \
        params ip="192.168.122.101" cidr_netmask="32" \
        op monitor interval="30s"
primitive dlm ocf:pacemaker:controld \
        op monitor interval="120s"
<emphasis>primitive gfs-control ocf:pacemaker:controld \</emphasis>
<emphasis> params daemon=”gfs_controld.pcmk” args=”-g 0” \</emphasis>
<emphasis> op monitor interval="120s"</emphasis>
ms WebDataClone WebData \
        meta master-max="1" master-node-max="1" clone-max="2" clone-node-max="1" notify="true"
clone dlm-clone dlm \
        meta interleave="true"
<emphasis>clone gfs-clone gfs-control \</emphasis>
<emphasis> meta interleave="true"</emphasis>
location prefer-pcmk-1 WebSite 50: pcmk-1
colocation WebSite-with-WebFS inf: WebSite WebFS
colocation fs_on_drbd inf: WebFS WebDataClone:Master
<emphasis>colocation gfs-with-dlm inf: gfs-clone dlm-clone</emphasis>
colocation website-with-ip inf: WebSite ClusterIP
order WebFS-after-WebData inf: WebDataClone:promote WebFS:start
order WebSite-after-WebFS inf: WebFS WebSite
order apache-after-ip inf: ClusterIP WebSite
<emphasis>order start-gfs-after-dlm inf: dlm-clone gfs-clone</emphasis>
property $id="cib-bootstrap-options" \
        dc-version="1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7" \
        cluster-infrastructure="openais" \
        expected-quorum-votes=”2” \
        stonith-enabled="false" \
        no-quorum-policy="ignore"
rsc_defaults $id="rsc-options" \
        resource-stickiness=”100”
crm(gfs-glue)# <userinput>cib commit gfs-glue</userinput>
INFO: commited 'gfs-glue' shadow CIB to the cluster
crm(gfs-glue)# <userinput>quit</userinput>
bye
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Thu Sep  3 20:49:54 2009
Stack: openais
Current DC: pcmk-2 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
6 Resources configured.
============

Online: [ pcmk-1 pcmk-2 ]

WebSite (ocf::heartbeat:apache):        Started pcmk-2
Master/Slave Set: WebDataClone
        Masters: [ pcmk-1 ]
        Slaves: [ pcmk-2 ]
ClusterIP        (ocf::heartbeat:IPaddr):        Started pcmk-2
Clone Set: dlm-clone
        Started: [ pcmk-2 pcmk-1 ]
<emphasis>Clone Set: gfs-clone</emphasis>
<emphasis> Started: [ pcmk-2 pcmk-1 ]</emphasis>
WebFS   (ocf::heartbeat:Filesystem):    Started pcmk-1
 
crm(stack-glue)# <userinput>cib commit stack-glue</userinput>
INFO: commited 'stack-glue' shadow CIB to the cluster
crm(stack-glue)# <userinput>quit</userinput>
bye
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Thu Sep  3 20:49:54 2009
Stack: openais
Current DC: pcmk-2 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
5 Resources configured.
============

Online: [ pcmk-1 pcmk-2 ]

WebSite (ocf::heartbeat:apache):        Started pcmk-2
Master/Slave Set: WebDataClone
        Masters: [ pcmk-1 ]
        Slaves: [ pcmk-2 ]
ClusterIP        (ocf::heartbeat:IPaddr):        Started pcmk-2
<emphasis>Clone Set: dlm-clone</emphasis>
<emphasis> Started: [ pcmk-2 pcmk-1 ]</emphasis>
WebFS   (ocf::heartbeat:Filesystem):    Started pcmk-2
 
mkfs.gfs2 -p lock_dlm -j 2 -t pcmk:web /dev/drbd1
[root@pcmk-1 ~]# <userinput>mkfs.gfs2 -t pcmk:web -p lock_dlm -j 2 /dev/vdb </userinput>
This will destroy any data on /dev/vdb.
It appears to contain: data

Are you sure you want to proceed? [y/n] y

Device:                    /dev/vdb
Blocksize:                 4096
Device Size                1.00 GB (131072 blocks)
Filesystem Size:           1.00 GB (131070 blocks)
Journals:                  2
Resource Groups:           2
Locking Protocol:          "lock_dlm"
Lock Table:                "pcmk:web"
UUID:                      6B776F46-177B-BAF8-2C2B-292C0E078613

[root@pcmk-1 ~]#
 
primitive ClusterIP ocf:heartbeat:IPaddr2 \ 
        params ip="192.168.122.101" cidr_netmask="32" clusterip_hash="sourceip" \
        op monitor interval="30s"
     添加 DLM 服务 添加 GFS2 服务 基本上所有的事情都已经准备就绪了。最新的DRBD是支持 Primary/Primary(主/主)模式的，并且我们的文件系统的是针对集群的。所有我们要做的事情就是重新配置我们的集群来使用它们(的先进功能)。 在参数行添加以下内容: 在我们对一个已存在的分区做任何操作之前，我们要确保它没有被挂载。我们告诉集群停止WebFS这个资源来确保这一点。这可以确保其他使用WebFS的资源会正确的依次关闭。 把 master-max 改为 2 转变为Active/Active 创建一个 GFS2 文件系统 创建并迁移数据到 GFS2 分区 在每个节点都执行以下命令。 首先我们要用 -p选项来指定我们用的是内核的DLM，然后我们用-j来表示我们为两个日志保留足够的空间(每个操作文件系统的节点各一个)。 GFS2要求运行两个服务，首先是用户空间访问内核的分布式锁管理(DLM)的接口。 DLM是用来统筹哪个节点可以处理某个特定的文件，并且与Pacemaker集成来得到节点之间的关系<footnote> <para> The list of nodes the cluster considers to be available </para> </footnote>和隔离能力。 以下是完整的配置 安装一个集群文件系统 - GFS2 最后，我们用-t来指定lock table的名称。这个字段的格式是 clustername:fsname(集群名称:文件系统名称)。fsname的话，我们只要用一个唯一的并且能描述我们这个集群的名称就好了，我们用默认的pcmk。 然后我们要把文件系统和apache资源变成clones。同样的 crm shell会自动更新相关约束。 注意 Apache and WebFS 两者都已经停止了。 请注意所有跟ClusterIP相关的限制都已经被更新到与WebIP相关，这是使用crm shell的另一个好处。 现在确保Pacemaker只在有dlm服务运行的节点上面启动 gfs-control 服务 现在集群的基层和集成部分都正常运行，我们现在创建一个GFS2分区 现在我们重新创建这个资源， 我们也要重建跟这个资源相关的约束条件，因为shell会自动删除跟WebFS相关的约束条件。 现在我们要告诉集群如何决定请求怎样分配给节点。我们要设置 clusterip_hash这个参数来实现它。 一旦DLM启动了，我们可以加上GFS2的控制进程了。 打开ClusterIP的配置 准备工作 重新配置 Pacemaker 为 Active/Active 8.5. 重新为集群配置GFS2 需求 看看配置文件有没有错误，然后退出shell看看集群的反应。 整合 Pacemaker-GFS2 完整的定义就像下面一样: TODO: Explain the meaning of the interleave option TODO: Put one node into standby to demonstrate failover 恢复测试 DLM控制进程需要在所有可用的集群节点上面运行，所以我们用shell交互模式来添加一个cloned类型的资源。 首先我们在各个节点上面安装GFS2。 最后要告诉集群现在允许把两个节点都提升为 Primary(换句话说 Master). 唯一的限制是我们要用一个针对集群的文件系统(我们之前用的ext4，它并不是这样一个文件系统)。 OCFS2或者GFS2都是可以的，但是在&DISTRO; &amp;DISTRO_VERSION;上面，我们用GFS2。 Active/Active集群一个主要的需求就是数据在两台机器上面都是可用并且是同步的。Pacemaker没有要求你怎么实现，你可以用SAN，但是自从DRBD支持多主模式，我们也可以用这个来实现。 另外一个服务是GFS2自身的控制进程，也是与Pacemaker集成来得到节点之间的关系。 然后再迁移数据到这个新的文件系统。现在我们创建一个跟上次不一样的主页。 如果我们不能访问这些服务，那做成 Active/Active是没有必要的，所以我们要先clone这个IP地址，克隆的IPaddr2资源用的是iptables规则来保证每个请求都只由一个节点来处理。附件的meta选项告诉集群我们要克隆多少个实例(每个节点一个"请求桶")。并且如果其他节点挂了，剩下的节点可以处理所有的请求。否则这些请求都会被丢弃。 这个操作会清除DRBD分区上面的所有数据，请备份重要的数据。 这次操作会改很多东西，所以我们再次使用交互模式 如果要更改集群的名称，找到包含name:pacemaker的配置文件区域，然后添加如下所示的选项即可。 用crm shell来创建gfs-control这个集群资源: 我们要为GFS2分区指定一系列附加的参数。 [root@pcmk-1 ~]# <userinput>configure edit  ClusterIP</userinput> clusterip_hash="sourceip" clustername: myname 