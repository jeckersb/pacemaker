��    8      �  O   �      �  �  �  �  �
    5  @   I    �  �  �  �  ;  �  �  �  �  ^  �  �  !$  U  �,  a   (0  �  �0  �  d3  �  T9  �   N<  U  =  c   \?  z   �?  G   ;@     �@     �@     �@  �   �@  �  �A  [   }C  �   �C  0   �D     �D     �D    �D  `   �E  C   ^F    �F  G   �G  x   �G  5   vH  �   �H  �   1I  q   �I     RJ  �   jJ  G   2K     zK  �   �K  ;   4L     pL  w   �L  )   �L  �   $M  l  �M  K   %O     qO  �   �O    NP  �  `Q  �  W    �Y  @   �\    ]  �  _  �  �b  �  [d  �  Of  ^  Ik  �  �p  U  Yy  a   �|  �  }  �  �  �  ۅ  �   Ո  U  ��  9   �  G   �  H   e�  
   ��     ��     Ќ  �   �  l  ��  J   �  j   L�  0   ��     �     ��    �  B   "�  =   e�    ��  M   ��  �    �  0   ��  v   ȓ  �   ?�  l   �     O�  �   f�  ]   �     q�  �   ��  ;   �     G�  {   T�     З  o   �  M  Z�  6   ��  
   ߙ  �   �               	   )      +          $          '   7           5   (       0   %          /           4         &      6                                      !      *   .             -                                                 2   
           ,              "       8   3   1   #    
[root@pcmk-1 ~]# <userinput>crm configure show</userinput>
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
ms WebDataClone WebData \
        meta master-max="1" master-node-max="1" clone-max="2" clone-node-max="1" notify="true"
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
[root@pcmk-1 ~]# <userinput>crm node online</userinput>
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Tue Sep  1 10:13:25 2009
Stack: openais
Current DC: pcmk-1 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
4 Resources configured.
============

<emphasis>Online: [ pcmk-1 pcmk-2 ]</emphasis>

ClusterIP        (ocf::heartbeat:IPaddr):        Started pcmk-2
WebSite (ocf::heartbeat:apache):        Started pcmk-2
Master/Slave Set: WebDataClone
        Masters: [ pcmk-2 ]
        Slaves: [ pcmk-1 ]
WebFS   (ocf::heartbeat:Filesystem):    Started pcmk-2
       
[root@pcmk-1 ~]# <userinput>crm node standby</userinput>
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Tue Sep  1 10:09:57 2009
Stack: openais
Current DC: pcmk-1 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
4 Resources configured.
============

<emphasis>Node pcmk-1: standby</emphasis>
Online: [ pcmk-2 ]

ClusterIP        (ocf::heartbeat:IPaddr):        <emphasis>Started pcmk-2</emphasis>
WebSite (ocf::heartbeat:apache):        <emphasis>Started pcmk-2</emphasis>
Master/Slave Set: WebDataClone
        <emphasis>Masters: [ pcmk-2 ]</emphasis>
        Stopped: [ WebData:1 ]
WebFS   (ocf::heartbeat:Filesystem):    <emphasis>Started pcmk-2</emphasis>
       
[root@pcmk-1 ~]# <userinput>crm</userinput>
cib crm(live)#
     
[root@pcmk-1 ~]# <userinput>crm</userinput>
crm(live)# <userinput>cib new fs</userinput>
INFO: fs shadow CIB created
crm(fs)# <userinput>configure primitive WebFS ocf:heartbeat:Filesystem \</userinput>
<userinput>        params device="/dev/drbd/by-res/wwwdata" directory="/var/www/html" fstype="ext4"</userinput>
crm(fs)# <userinput>configure colocation fs_on_drbd inf: WebFS WebDataClone:Master</userinput>
crm(fs)# <userinput>configure order WebFS-after-WebData inf: WebDataClone:promote WebFS:start</userinput>
     
[root@pcmk-1 ~]# <userinput>drbdadm -- --overwrite-data-of-peer primary wwwdata</userinput>
[root@pcmk-1 ~]# <userinput>cat /proc/drbd</userinput>
version: 8.3.6 (api:88/proto:86-90)
GIT-hash: f3606c47cc6fcf6b3f086e425cb34af8b7a81bbf build by root@pcmk-1, 2009-12-08 11:22:57
 1: cs:SyncSource ro:Primary/Secondary ds:UpToDate/<emphasis>Inconsistent</emphasis> C r----
    ns:2184 nr:0 dw:0 dr:2472 al:0 bm:0 lo:0 pe:0 ua:0 ap:0 ep:1 wo:b oos:10064
        [=====&gt;..............] sync'ed: 33.4% (10064/12248)K
        finish: 0:00:37 speed: 240 (240) K/sec
[root@pcmk-1 ~]# <userinput>cat /proc/drbd</userinput>
version: 8.3.6 (api:88/proto:86-90)
GIT-hash: f3606c47cc6fcf6b3f086e425cb34af8b7a81bbf build by root@pcmk-1, 2009-12-08 11:22:57
 1: <emphasis>cs:Connected ro:Primary/Secondary ds:UpToDate/UpToDate</emphasis> C r----
    ns:12248 nr:0 dw:0 dr:12536 al:0 bm:1 lo:0 pe:0 ua:0 ap:0 ep:1 wo:b oos:0
       
[root@pcmk-1 ~]# <userinput>drbdadm create-md wwwdata</userinput>
md_offset 12578816
al_offset 12546048
bm_offset 12541952

Found some data 
 ==&gt; This might destroy existing data! &lt;==

Do you want to proceed?
[need to type 'yes' to confirm] <userinput>yes</userinput>

Writing meta data...
initializing activity log
NOT initialized bitmap
New drbd meta data block successfully created.
success
       
[root@pcmk-1 ~]# <userinput>lvcreate -n drbd-demo -L 1G VolGroup</userinput>
  Logical volume "drbd-demo" created
[root@pcmk-1 ~]# <userinput>lvs</userinput>
  LV        VG       Attr   LSize   Origin Snap%  Move Log Copy%  Convert
  <emphasis>drbd-demo VolGroup -wi-a- 1.00G</emphasis>                                      
  lv_root   VolGroup -wi-ao   7.30G                                      
  lv_swap   VolGroup -wi-ao 500.00M
       
[root@pcmk-1 ~]# <userinput>mkfs.ext4 /dev/drbd1</userinput>
mke2fs 1.41.4 (27-Jan-2009)
Filesystem label=
OS type: Linux
Block size=1024 (log=0)
Fragment size=1024 (log=0)
3072 inodes, 12248 blocks
612 blocks (5.00%) reserved for the super user
First data block=1
Maximum filesystem blocks=12582912
2 block groups
8192 blocks per group, 8192 fragments per group
1536 inodes per group
Superblock backups stored on blocks: 
        8193

Writing inode tables: done                            
Creating journal (1024 blocks): done
Writing superblocks and filesystem accounting information: done

This filesystem will be automatically checked every 26 mounts or
180 days, whichever comes first.  Use tune2fs -c or -i to override.

Now mount the newly created filesystem so we can create our index file
mount /dev/drbd1 /mnt/
cat &lt;&lt;-END &gt;/mnt/index.html
&lt;html&gt;
&lt;body&gt;My Test Site - drbd&lt;/body&gt;
&lt;/html&gt;
END
umount /dev/drbd1
[root@pcmk-1 ~]# <userinput>mount /dev/drbd1 /mnt/</userinput>
[root@pcmk-1 ~]# <userinput>cat &lt;&lt;-END &gt;/mnt/index.html</userinput>
&gt; &lt;html&gt;
&gt; &lt;body&gt;My Test Site - drbd&lt;/body&gt;
&gt; &lt;/html&gt;
&gt; END
[root@pcmk-1 ~]# <userinput>umount /dev/drbd1</userinput>
       
[root@pcmk-1 ~]# <userinput>modprobe drbd</userinput>
[root@pcmk-1 ~]# <userinput>drbdadm up wwwdata</userinput>
[root@pcmk-1 ~]# <userinput>cat /proc/drbd</userinput>
version: 8.3.6 (api:88/proto:86-90)
GIT-hash: f3606c47cc6fcf6b3f086e425cb34af8b7a81bbf build by root@pcmk-1, 2009-12-08 11:22:57

<emphasis> 1: cs:WFConnection ro:Secondary/Unknown ds:Inconsistent/DUnknown C r--</emphasis>--
    ns:0 nr:0 dw:0 dr:0 al:0 bm:0 lo:0 pe:0 ua:0 ap:0 ep:1 wo:b oos:12248
[root@pcmk-1 ~]# 

Repeat on the second node
drbdadm --force create-md wwwdata 
modprobe drbd
drbdadm up wwwdata
cat /proc/drbd
[root@pcmk-2 ~]# <userinput>drbdadm --force create-md wwwdata</userinput>
Writing meta data...
initializing activity log
NOT initialized bitmap
New drbd meta data block successfully created.
success
[root@pcmk-2 ~]# <userinput>modprobe drbd</userinput>
WARNING: Deprecated config file /etc/modprobe.conf, all config files belong into /etc/modprobe.d/.
[root@pcmk-2 ~]# <userinput>drbdadm up wwwdata</userinput>
[root@pcmk-2 ~]# <userinput>cat /proc/drbd</userinput>
version: 8.3.6 (api:88/proto:86-90)
GIT-hash: f3606c47cc6fcf6b3f086e425cb34af8b7a81bbf build by root@pcmk-1, 2009-12-08 11:22:57

<emphasis> 1: cs:Connected ro:Secondary/Secondary ds:Inconsistent/Inconsistent C r----</emphasis>
    ns:0 nr:0 dw:0 dr:0 al:0 bm:0 lo:0 pe:0 ua:0 ap:0 ep:1 wo:b oos:12248
       
[root@pcmk-1 ~]# <userinput>yum install -y drbd-pacemaker</userinput>
Loaded plugins: presto, refresh-packagekit
Setting up Install Process
Resolving Dependencies
--&gt; Running transaction check
---&gt; Package drbd-pacemaker.x86_64 0:8.3.7-2.fc13 set to be updated
--&gt; Processing Dependency: drbd-utils = 8.3.7-2.fc13 for package: drbd-pacemaker-8.3.7-2.fc13.x86_64
--&gt; Running transaction check
---&gt; Package drbd-utils.x86_64 0:8.3.7-2.fc13 set to be updated
--&gt; Finished Dependency Resolution

Dependencies Resolved

=================================================================================
 Package                Arch           Version              Repository      Size
=================================================================================
Installing:
 drbd-pacemaker         x86_64         8.3.7-2.fc13         fedora          19 k
Installing for dependencies:
 drbd-utils             x86_64         8.3.7-2.fc13         fedora         165 k

Transaction Summary
=================================================================================
Install       2 Package(s)
Upgrade       0 Package(s)

Total download size: 184 k
Installed size: 427 k
Downloading Packages:
Setting up and reading Presto delta metadata
fedora/prestodelta                                        | 1.7 kB     00:00     
Processing delta metadata
Package(s) data still to download: 184 k
(1/2): drbd-pacemaker-8.3.7-2.fc13.x86_64.rpm             |  19 kB     00:01     
(2/2): drbd-utils-8.3.7-2.fc13.x86_64.rpm                 | 165 kB     00:02     
---------------------------------------------------------------------------------
Total                                             45 kB/s | 184 kB     00:04     
Running rpm_check_debug
Running Transaction Test
Transaction Test Succeeded
Running Transaction
  Installing     : drbd-utils-8.3.7-2.fc13.x86_64                            1/2 
  Installing     : drbd-pacemaker-8.3.7-2.fc13.x86_64                        2/2 

Installed:
  drbd-pacemaker.x86_64 0:8.3.7-2.fc13                                           

Dependency Installed:
  drbd-utils.x86_64 0:8.3.7-2.fc13                                               

Complete!
[root@pcmk-1 ~]#
     
[root@pcmk-2 ~]# <userinput>lvs</userinput>
  LV      VG       Attr   LSize   Origin Snap%  Move Log Copy%  Convert
  lv_root VolGroup -wi-ao   7.30G                                      
  lv_swap <emphasis>VolGroup</emphasis> -wi-ao 500.00M                                      
[root@pcmk-2 ~]# <userinput>lvcreate -n drbd-demo -L 1G VolGroup</userinput>
 <emphasis> Logical volume "drbd-demo" created</emphasis>
[root@pcmk-2 ~]# <userinput>lvs</userinput>
  LV        VG       Attr   LSize   Origin Snap%  Move Log Copy%  Convert
  <emphasis>drbd-demo VolGroup -wi-a- 1.00G </emphasis>                                     
  lv_root   VolGroup -wi-ao   7.30G                                      
  lv_swap   VolGroup -wi-ao 500.00M
       
cib crm(live)# <userinput>cib new drbd</userinput>
INFO: drbd shadow CIB created
crm(drbd)#
     
crm(drbd)# <userinput>cib commit drbd</userinput>
INFO: commited 'drbd' shadow CIB to the cluster
crm(drbd)# <userinput>quit</userinput>
bye
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Tue Sep  1 09:37:13 2009
Stack: openais
Current DC: pcmk-1 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
3 Resources configured.
============

Online: [ pcmk-1 pcmk-2 ]

ClusterIP        (ocf::heartbeat:IPaddr):        Started pcmk-1
WebSite (ocf::heartbeat:apache):        Started pcmk-1
<emphasis>Master/Slave Set: WebDataClone</emphasis>
<emphasis> Masters: [ pcmk-2 ]</emphasis>
<emphasis> Slaves: [ pcmk-1 ]</emphasis>
     
crm(drbd)# <userinput>configure primitive WebData ocf:linbit:drbd params drbd_resource=wwwdata \</userinput>
<userinput>        op monitor interval=60s</userinput>
crm(drbd)# <userinput>configure ms WebDataClone WebData meta master-max=1 master-node-max=1 \</userinput>
<userinput>        clone-max=2 clone-node-max=1 notify=true</userinput>
crm(drbd)# <userinput>configure show</userinput>
node pcmk-1
node pcmk-2
<emphasis>primitive WebData ocf:linbit:drbd \</emphasis>
<emphasis> params drbd_resource="wwwdata" \</emphasis>
<emphasis> op monitor interval="60s"</emphasis>
primitive WebSite ocf:heartbeat:apache \
        params configfile="/etc/httpd/conf/httpd.conf" \
        op monitor interval="1min"
primitive ClusterIP ocf:heartbeat:IPaddr2 \
        params ip="192.168.122.101" cidr_netmask="32" \
        op monitor interval="30s"
<emphasis>ms WebDataClone WebData \</emphasis>
<emphasis> meta master-max="1" master-node-max="1" clone-max="2" clone-node-max="1" notify="true"</emphasis>
location prefer-pcmk-1 WebSite 50: pcmk-1
colocation website-with-ip inf: WebSite ClusterIP
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
crm(fs)# <userinput>cib commit fs</userinput>
INFO: commited 'fs' shadow CIB to the cluster
crm(fs)# <userinput>quit</userinput>
bye
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Tue Sep  1 10:08:44 2009
Stack: openais
Current DC: pcmk-1 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
4 Resources configured.
============

Online: [ pcmk-1 pcmk-2 ]

ClusterIP        (ocf::heartbeat:IPaddr):        Started pcmk-1
<emphasis>WebSite (ocf::heartbeat:apache): Started pcmk-1</emphasis>
Master/Slave Set: WebDataClone
        Masters: [ pcmk-1 ]
        Slaves: [ pcmk-2 ]
<emphasis>WebFS (ocf::heartbeat:Filesystem): Started pcmk-1</emphasis>
     
crm(fs)# <userinput>configure colocation WebSite-with-WebFS inf: WebSite WebFS</userinput>
crm(fs)# <userinput>configure order WebSite-after-WebFS inf: WebFS WebSite</userinput>
     
global { 
  usage-count yes; 
}
common {
  protocol C;
}
resource wwwdata {
  meta-disk internal;
  device    /dev/drbd1;
  syncer {
    verify-alg sha1;
  }
  net { 
    allow-two-primaries; 
  }
 <emphasis> on pcmk-1</emphasis> {
    disk      /dev/mapper/<emphasis>VolGroup</emphasis>-drbd--demo;
    address   192.168.122.101<emphasis>:7789;</emphasis> 
  }
  <emphasis>on</emphasis> 
<emphasis>pcmk-2</emphasis> {
    disk      /dev/mapper/<emphasis>VolGroup</emphasis>-drbd--demo;
    address   192.168.122.102<emphasis>:7789;</emphasis>      
  }
}
       After reviewing the new configuration, we again upload it and watch the cluster put it into effect. Be sure to use the names and addresses of <emphasis>your</emphasis> nodes if they differ from the ones used in this guide. Before we configure DRBD, we need to set aside some disk for it to use. Configure DRBD Configure the Cluster for DRBD Create A Partition for DRBD Detailed information on the directives used in this configuration (and other alternatives) is available from <ulink url="http://www.drbd.org/users-guide/ch-configure.html">http://www.drbd.org/users-guide/ch-configure.html</ulink> Even if you’re serving up static websites, having to manually synchronize the contents of that website to all the machines in the cluster is not ideal. For dynamic websites, such as a wiki, its not even an option. Not everyone care afford network-attached storage but somehow the data needs to be kept in sync. Enter DRBD which can be thought of as network based RAID-1. See <ulink url="http://www.drbd.org/">http://www.drbd.org</ulink>/ for more details. First we launch the shell. The prompt will change to indicate you’re in interactive mode. If you have more than 1Gb free, feel free to use it. For this guide however, 1Gb is plenty of space for a single html file and sufficient for later holding the GFS2 metadata. Include details on adding a second DRBD resource Initialize and Load DRBD Install the DRBD Packages Next we must create a working copy or the current configuration. This is where all our changes will go. The cluster will not see any of them until we say its ok. Notice again how the prompt changes, this time to indicate that we’re no longer looking at the live cluster. Notice that our resource stickiness settings prevent the services from migrating back to pcmk-1. Now load the DRBD kernel module and confirm that everything is sane Now that DRBD is functioning we can configure a Filesystem resource to use it. In addition to the filesystem’s definition, we also need to tell the cluster where it can be located (only on the DRBD Primary) and when it is allowed to start (after the Primary was promoted). Now we can create our DRBD clone and display the revised configuration. Now we need to tell DRBD which set of data to use. Since both sides contain garbage, we can run the following on pcmk-1: Once again we’ll use the shell’s interactive mode Once we’re happy with the changes, we can tell the cluster to start using them and use crm_mon to check everything is functioning. Once we’ve done everything we needed to on pcmk-1 (in this case nothing, we just wanted to see the resources move), we can allow the node to be a full cluster member again. One handy feature of the crm shell is that you can use it in interactive mode to make several changes atomically. Populate DRBD with Data Put the local node into standby mode and observe the cluster move all the resources to the other node. Note also that the node’s status will change to indicate that it can no longer host resources. Repeat this on the second node, be sure to use the same size partition. Replicated Storage with DRBD Since its inclusion in the upstream 2.6.33 kernel, everything needed to use DRBD ships with &DISTRO; &amp;DISTRO_VERSION;. All you need to do is install it: TODO: Explain the reason for the allow-two-primaries option Testing Migration There is no series of commands for build a DRBD configuration, so simply copy the configuration below to /etc/drbd.conf Time to review the updated configuration: We also need to tell the cluster that Apache needs to run on the same machine as the filesystem and that it must be active before Apache can start. We could shut down the active node again, but another way to safely simulate recovery is to put the node into what is called “standby mode”. Nodes in this state tell the cluster that they are not allowed to run resources. Any resources found active there will be moved elsewhere. This feature can be particularly useful when updating the resources’ packages. With the configuration in place, we can now perform the DRBD initialization Write the DRBD Config pcmk-1 is now in the Primary state which allows it to be written to. Which means its a good point at which to create a filesystem and populate it with some data to serve up via our WebSite resource. Project-Id-Version: 0
POT-Creation-Date: 2010-12-15T23:32:37
PO-Revision-Date: 2010-12-16 00:32+0800
Last-Translator: Charlie Chen <laneovcc@gmail.com>
Language-Team: None
Language: 
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit
 
[root@pcmk-1 ~]# <userinput>crm configure show</userinput>
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
ms WebDataClone WebData \
        meta master-max="1" master-node-max="1" clone-max="2" clone-node-max="1" notify="true"
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
[root@pcmk-1 ~]# <userinput>crm node online</userinput>
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Tue Sep  1 10:13:25 2009
Stack: openais
Current DC: pcmk-1 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
4 Resources configured.
============

<emphasis>Online: [ pcmk-1 pcmk-2 ]</emphasis>

ClusterIP        (ocf::heartbeat:IPaddr):        Started pcmk-2
WebSite (ocf::heartbeat:apache):        Started pcmk-2
Master/Slave Set: WebDataClone
        Masters: [ pcmk-2 ]
        Slaves: [ pcmk-1 ]
WebFS   (ocf::heartbeat:Filesystem):    Started pcmk-2
       
[root@pcmk-1 ~]# <userinput>crm node standby</userinput>
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Tue Sep  1 10:09:57 2009
Stack: openais
Current DC: pcmk-1 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
4 Resources configured.
============

<emphasis>Node pcmk-1: standby</emphasis>
Online: [ pcmk-2 ]

ClusterIP        (ocf::heartbeat:IPaddr):        <emphasis>Started pcmk-2</emphasis>
WebSite (ocf::heartbeat:apache):        <emphasis>Started pcmk-2</emphasis>
Master/Slave Set: WebDataClone
        <emphasis>Masters: [ pcmk-2 ]</emphasis>
        Stopped: [ WebData:1 ]
WebFS   (ocf::heartbeat:Filesystem):    <emphasis>Started pcmk-2</emphasis>
       
[root@pcmk-1 ~]# <userinput>crm</userinput>
cib crm(live)#
     
[root@pcmk-1 ~]# <userinput>crm</userinput>
crm(live)# <userinput>cib new fs</userinput>
INFO: fs shadow CIB created
crm(fs)# <userinput>configure primitive WebFS ocf:heartbeat:Filesystem \</userinput>
<userinput>        params device="/dev/drbd/by-res/wwwdata" directory="/var/www/html" fstype="ext4"</userinput>
crm(fs)# <userinput>configure colocation fs_on_drbd inf: WebFS WebDataClone:Master</userinput>
crm(fs)# <userinput>configure order WebFS-after-WebData inf: WebDataClone:promote WebFS:start</userinput>
     
[root@pcmk-1 ~]# <userinput>drbdadm -- --overwrite-data-of-peer primary wwwdata</userinput>
[root@pcmk-1 ~]# <userinput>cat /proc/drbd</userinput>
version: 8.3.6 (api:88/proto:86-90)
GIT-hash: f3606c47cc6fcf6b3f086e425cb34af8b7a81bbf build by root@pcmk-1, 2009-12-08 11:22:57
 1: cs:SyncSource ro:Primary/Secondary ds:UpToDate/<emphasis>Inconsistent</emphasis> C r----
    ns:2184 nr:0 dw:0 dr:2472 al:0 bm:0 lo:0 pe:0 ua:0 ap:0 ep:1 wo:b oos:10064
        [=====&gt;..............] sync'ed: 33.4% (10064/12248)K
        finish: 0:00:37 speed: 240 (240) K/sec
[root@pcmk-1 ~]# <userinput>cat /proc/drbd</userinput>
version: 8.3.6 (api:88/proto:86-90)
GIT-hash: f3606c47cc6fcf6b3f086e425cb34af8b7a81bbf build by root@pcmk-1, 2009-12-08 11:22:57
 1: <emphasis>cs:Connected ro:Primary/Secondary ds:UpToDate/UpToDate</emphasis> C r----
    ns:12248 nr:0 dw:0 dr:12536 al:0 bm:1 lo:0 pe:0 ua:0 ap:0 ep:1 wo:b oos:0
       
[root@pcmk-1 ~]# <userinput>drbdadm create-md wwwdata</userinput>
md_offset 12578816
al_offset 12546048
bm_offset 12541952

Found some data 
 ==&gt; This might destroy existing data! &lt;==

Do you want to proceed?
[need to type 'yes' to confirm] <userinput>yes</userinput>

Writing meta data...
initializing activity log
NOT initialized bitmap
New drbd meta data block successfully created.
success
       
[root@pcmk-1 ~]# <userinput>lvcreate -n drbd-demo -L 1G VolGroup</userinput>
  Logical volume "drbd-demo" created
[root@pcmk-1 ~]# <userinput>lvs</userinput>
  LV        VG       Attr   LSize   Origin Snap%  Move Log Copy%  Convert
  <emphasis>drbd-demo VolGroup -wi-a- 1.00G</emphasis>                                      
  lv_root   VolGroup -wi-ao   7.30G                                      
  lv_swap   VolGroup -wi-ao 500.00M
       
[root@pcmk-1 ~]# <userinput>mkfs.ext4 /dev/drbd1</userinput>
mke2fs 1.41.4 (27-Jan-2009)
Filesystem label=
OS type: Linux
Block size=1024 (log=0)
Fragment size=1024 (log=0)
3072 inodes, 12248 blocks
612 blocks (5.00%) reserved for the super user
First data block=1
Maximum filesystem blocks=12582912
2 block groups
8192 blocks per group, 8192 fragments per group
1536 inodes per group
Superblock backups stored on blocks: 
        8193

Writing inode tables: done                            
Creating journal (1024 blocks): done
Writing superblocks and filesystem accounting information: done

This filesystem will be automatically checked every 26 mounts or
180 days, whichever comes first.  Use tune2fs -c or -i to override.

Now mount the newly created filesystem so we can create our index file
mount /dev/drbd1 /mnt/
cat &lt;&lt;-END &gt;/mnt/index.html
&lt;html&gt;
&lt;body&gt;My Test Site - drbd&lt;/body&gt;
&lt;/html&gt;
END
umount /dev/drbd1
[root@pcmk-1 ~]# <userinput>mount /dev/drbd1 /mnt/</userinput>
[root@pcmk-1 ~]# <userinput>cat &lt;&lt;-END &gt;/mnt/index.html</userinput>
&gt; &lt;html&gt;
&gt; &lt;body&gt;My Test Site - drbd&lt;/body&gt;
&gt; &lt;/html&gt;
&gt; END
[root@pcmk-1 ~]# <userinput>umount /dev/drbd1</userinput>
       
[root@pcmk-1 ~]# <userinput>modprobe drbd</userinput>
[root@pcmk-1 ~]# <userinput>drbdadm up wwwdata</userinput>
[root@pcmk-1 ~]# <userinput>cat /proc/drbd</userinput>
version: 8.3.6 (api:88/proto:86-90)
GIT-hash: f3606c47cc6fcf6b3f086e425cb34af8b7a81bbf build by root@pcmk-1, 2009-12-08 11:22:57

<emphasis> 1: cs:WFConnection ro:Secondary/Unknown ds:Inconsistent/DUnknown C r--</emphasis>--
    ns:0 nr:0 dw:0 dr:0 al:0 bm:0 lo:0 pe:0 ua:0 ap:0 ep:1 wo:b oos:12248
[root@pcmk-1 ~]# 

Repeat on the second node
drbdadm --force create-md wwwdata 
modprobe drbd
drbdadm up wwwdata
cat /proc/drbd
[root@pcmk-2 ~]# <userinput>drbdadm --force create-md wwwdata</userinput>
Writing meta data...
initializing activity log
NOT initialized bitmap
New drbd meta data block successfully created.
success
[root@pcmk-2 ~]# <userinput>modprobe drbd</userinput>
WARNING: Deprecated config file /etc/modprobe.conf, all config files belong into /etc/modprobe.d/.
[root@pcmk-2 ~]# <userinput>drbdadm up wwwdata</userinput>
[root@pcmk-2 ~]# <userinput>cat /proc/drbd</userinput>
version: 8.3.6 (api:88/proto:86-90)
GIT-hash: f3606c47cc6fcf6b3f086e425cb34af8b7a81bbf build by root@pcmk-1, 2009-12-08 11:22:57

<emphasis> 1: cs:Connected ro:Secondary/Secondary ds:Inconsistent/Inconsistent C r----</emphasis>
    ns:0 nr:0 dw:0 dr:0 al:0 bm:0 lo:0 pe:0 ua:0 ap:0 ep:1 wo:b oos:12248
       
[root@pcmk-1 ~]# <userinput>yum install -y drbd-pacemaker</userinput>
Loaded plugins: presto, refresh-packagekit
Setting up Install Process
Resolving Dependencies
--&gt; Running transaction check
---&gt; Package drbd-pacemaker.x86_64 0:8.3.7-2.fc13 set to be updated
--&gt; Processing Dependency: drbd-utils = 8.3.7-2.fc13 for package: drbd-pacemaker-8.3.7-2.fc13.x86_64
--&gt; Running transaction check
---&gt; Package drbd-utils.x86_64 0:8.3.7-2.fc13 set to be updated
--&gt; Finished Dependency Resolution

Dependencies Resolved

=================================================================================
 Package                Arch           Version              Repository      Size
=================================================================================
Installing:
 drbd-pacemaker         x86_64         8.3.7-2.fc13         fedora          19 k
Installing for dependencies:
 drbd-utils             x86_64         8.3.7-2.fc13         fedora         165 k

Transaction Summary
=================================================================================
Install       2 Package(s)
Upgrade       0 Package(s)

Total download size: 184 k
Installed size: 427 k
Downloading Packages:
Setting up and reading Presto delta metadata
fedora/prestodelta                                        | 1.7 kB     00:00     
Processing delta metadata
Package(s) data still to download: 184 k
(1/2): drbd-pacemaker-8.3.7-2.fc13.x86_64.rpm             |  19 kB     00:01     
(2/2): drbd-utils-8.3.7-2.fc13.x86_64.rpm                 | 165 kB     00:02     
---------------------------------------------------------------------------------
Total                                             45 kB/s | 184 kB     00:04     
Running rpm_check_debug
Running Transaction Test
Transaction Test Succeeded
Running Transaction
  Installing     : drbd-utils-8.3.7-2.fc13.x86_64                            1/2 
  Installing     : drbd-pacemaker-8.3.7-2.fc13.x86_64                        2/2 

Installed:
  drbd-pacemaker.x86_64 0:8.3.7-2.fc13                                           

Dependency Installed:
  drbd-utils.x86_64 0:8.3.7-2.fc13                                               

Complete!
[root@pcmk-1 ~]#
     
[root@pcmk-2 ~]# <userinput>lvs</userinput>
  LV      VG       Attr   LSize   Origin Snap%  Move Log Copy%  Convert
  lv_root VolGroup -wi-ao   7.30G                                      
  lv_swap <emphasis>VolGroup</emphasis> -wi-ao 500.00M                                      
[root@pcmk-2 ~]# <userinput>lvcreate -n drbd-demo -L 1G VolGroup</userinput>
 <emphasis> Logical volume "drbd-demo" created</emphasis>
[root@pcmk-2 ~]# <userinput>lvs</userinput>
  LV        VG       Attr   LSize   Origin Snap%  Move Log Copy%  Convert
  <emphasis>drbd-demo VolGroup -wi-a- 1.00G </emphasis>                                     
  lv_root   VolGroup -wi-ao   7.30G                                      
  lv_swap   VolGroup -wi-ao 500.00M
       
cib crm(live)# <userinput>cib new drbd</userinput>
INFO: drbd shadow CIB created
crm(drbd)#
     
crm(drbd)# <userinput>cib commit drbd</userinput>
INFO: commited 'drbd' shadow CIB to the cluster
crm(drbd)# <userinput>quit</userinput>
bye
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Tue Sep  1 09:37:13 2009
Stack: openais
Current DC: pcmk-1 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
3 Resources configured.
============

Online: [ pcmk-1 pcmk-2 ]

ClusterIP        (ocf::heartbeat:IPaddr):        Started pcmk-1
WebSite (ocf::heartbeat:apache):        Started pcmk-1
<emphasis>Master/Slave Set: WebDataClone</emphasis>
<emphasis> Masters: [ pcmk-2 ]</emphasis>
<emphasis> Slaves: [ pcmk-1 ]</emphasis>
     
crm(drbd)# <userinput>configure primitive WebData ocf:linbit:drbd params drbd_resource=wwwdata \</userinput>
<userinput>        op monitor interval=60s</userinput>
crm(drbd)# <userinput>configure ms WebDataClone WebData meta master-max=1 master-node-max=1 \</userinput>
<userinput>        clone-max=2 clone-node-max=1 notify=true</userinput>
crm(drbd)# <userinput>configure show</userinput>
node pcmk-1
node pcmk-2
<emphasis>primitive WebData ocf:linbit:drbd \</emphasis>
<emphasis> params drbd_resource="wwwdata" \</emphasis>
<emphasis> op monitor interval="60s"</emphasis>
primitive WebSite ocf:heartbeat:apache \
        params configfile="/etc/httpd/conf/httpd.conf" \
        op monitor interval="1min"
primitive ClusterIP ocf:heartbeat:IPaddr2 \
        params ip="192.168.122.101" cidr_netmask="32" \
        op monitor interval="30s"
<emphasis>ms WebDataClone WebData \</emphasis>
<emphasis> meta master-max="1" master-node-max="1" clone-max="2" clone-node-max="1" notify="true"</emphasis>
location prefer-pcmk-1 WebSite 50: pcmk-1
colocation website-with-ip inf: WebSite ClusterIP
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
crm(fs)# <userinput>cib commit fs</userinput>
INFO: commited 'fs' shadow CIB to the cluster
crm(fs)# <userinput>quit</userinput>
bye
[root@pcmk-1 ~]# <userinput>crm_mon</userinput>
============
Last updated: Tue Sep  1 10:08:44 2009
Stack: openais
Current DC: pcmk-1 - partition with quorum
Version: 1.0.5-462f1569a43740667daf7b0f6b521742e9eb8fa7
2 Nodes configured, 2 expected votes
4 Resources configured.
============

Online: [ pcmk-1 pcmk-2 ]

ClusterIP        (ocf::heartbeat:IPaddr):        Started pcmk-1
<emphasis>WebSite (ocf::heartbeat:apache): Started pcmk-1</emphasis>
Master/Slave Set: WebDataClone
        Masters: [ pcmk-1 ]
        Slaves: [ pcmk-2 ]
<emphasis>WebFS (ocf::heartbeat:Filesystem): Started pcmk-1</emphasis>
     
crm(fs)# <userinput>configure colocation WebSite-with-WebFS inf: WebSite WebFS</userinput>
crm(fs)# <userinput>configure order WebSite-after-WebFS inf: WebFS WebSite</userinput>
     
global { 
  usage-count yes; 
}
common {
  protocol C;
}
resource wwwdata {
  meta-disk internal;
  device    /dev/drbd1;
  syncer {
    verify-alg sha1;
  }
  net { 
    allow-two-primaries; 
  }
 <emphasis> on pcmk-1</emphasis> {
    disk      /dev/mapper/<emphasis>VolGroup</emphasis>-drbd--demo;
    address   192.168.122.101<emphasis>:7789;</emphasis> 
  }
  <emphasis>on</emphasis> 
<emphasis>pcmk-2</emphasis> {
    disk      /dev/mapper/<emphasis>VolGroup</emphasis>-drbd--demo;
    address   192.168.122.102<emphasis>:7789;</emphasis>      
  }
}
       看完以后，我们提交它并看看有没有生效。 请注意要替换掉name和address选项以符合您的试验环境。 在我们设置之前，我们要创建一些空的磁盘分区给它。 配置DRBD 在集群中配置DRBD 为DRBD创建一个分区 想知道配置文件的详细信息，请访问 <ulink url="http://www.drbd.org/users-guide/ch-configure.html">http://www.drbd.org/users-guide/ch-configure.html</ulink> 就算你用的是静态站点，手工在各个节点之间同步文件也不是个好主意。如果是动态站点，那根本不会考虑这个。用NAS不是所有人都能负担得起，但是有些数据还是要同步。用用DRBD: 它被认为是网络RAID-1。访问 See <ulink url="http://www.drbd.org/">http://www.drbd.org</ulink>/获得更详细介绍 首先我们打开shell。提示会指出你现在是在交互模式下。 如果你有1Gb以上的空间，就用那么多吧， 在这个指南中根本用不到这么多空间。 Include details on adding a second DRBD resource 初始化并载入DRBD 安装DRBD软件包 然后我们创建一个当前配置文件的副本。我们在这个副本里更改配置。直到我们提交这个副本之前集群不会应用这些更改。请注意提示符的变更，现在它指出我们看到的已经不是当前(live)集群的配置文件。 注意我们设置的资源黏性值阻止了资源迁移回pcmk-1 现在讲DRBD的模块载入内核并检测是不是都正常 现在DRBD已经工作了，我们可以配置一个Filesystem资源来使用它。 此外，对于这个文件系统的定义，同样的我们告诉集群这个文件系统能在哪运行(主DRBD运行的节点)以及什么时候可以启动(在主DRBD启动以后)。 现在我们可以创建DRBD clone,然后看看修改过后的配置文件。 现在我们要告诉DRBD要用那个数据(那个节点作为主)。因为两边都有一些废数据，我们要在pcmk-1上面执行一下命令。 我们再一次的使用交互模式的crm shell 一旦你确认这些修改没问题，我们就提交这个副本，然后用crm_mon来看看修改是否生效了。 当我在pcmk-1上面操作完了－－本例中没有任何操作，我们只是想让资源移动移动－－我们可以让节点变回正常的集群成员。 crm shell一个便捷的特性是可以工作在交互模式下并自动的变更配置中的相关部分。 向DRBD中添加数据 把一个本地节点设置为standby模式并观察集群把所有资源移动到另外一个节点了。并且注意节点的状态改变为不能运行任何的资源。 在另外一个节点上面执行相同的操作，请确保使用了相同大小的分区。 用DRBD同步存储 在2.6.33以上的内核中，所以DRBD要的东西都在 &DISTRO; &amp;DISTRO_VERSION;中存在了，你只要安装它就好了。 TODO: Explain the reason for the allow-two-primaries option 迁移测试 没有命令来自动生成DRBD配置文件，所以我们要简单的拷贝下面的配置文件并粘贴到/etc/drbd.conf 审视一下你的配置: 我们也要告诉集群Apache也要运行在同样的节点上，而且文件系统要在Apache之前启动。 我们可以再次关掉在运行的那个节点，但是安全的方法是把节点设置为standby模式。节点在这个状态下面等于告诉集群它不能运行任何资源，任何在这个节点上面运行的资源都会被移动到其他地方。这个特性在更新资源安装包的时候特别的方便。(确实！) 配置完成以后，我们可以来执行初始化了 配置DRBD pcmk-1现在是处于Primary(主)状态了，它允许写入了。这意味着可以在上面创建文件系统并把一些数据放进去，并且用WebSite这个资源来展现。 