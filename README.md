# GreenDanubeCloud_InDectSys
Green Danube Cloud InDectSys Repo

This repo contains all files which where used during the research project of Green Danube Cloud
This guide explains the major software contribution developed during the project.

## Overview

Cyber attacks can be differeniated between „targeted“ and „non-targeted“ threats. Targeted threats are threats through which an attacker gains targeted access to a previously defined system. This means that the goal is known. These attacks have been fended off by hardware firewalls, intrusion detection systems and anti-virus software since the 1980s. However, the attack vector has changed in recent years, not least due to the use of sophisticated security systems on the perimeter. Today systems are attacked arbitrarily. Today, as a rule, unknown software vulnerabilities are used that penetrate the weakest link in the chain. For example, a web browser on the client itself. This vulnerability prepares a system through what is known as an exploit, and then subsequently installs a virus. The malware makes it possible to fully control the infected device. And now that these devices are more and more mobile, the malware is unwittingly active behind heavily secured systems such as high-end firewalls and intrusion detection systems. However, who the client is remains undefined. (non-targetet). Once the infection has taken place, a camouflaged connection to the outside is established via encrypted channels and the person responsible is notified of the success. Today this process is even carried out largely automatically and has even been commercialized in the meantime. Access to infected systems can then be acquired in relevant forums and used specifically for misuse.  

<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/Bilder2.jpg"><img width="448" height="348" src="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/Bilder2.jpg"></a>

As a consequence, classic security measures that consider interaction at network level, such as firewalls or intrusion detection systems (IDS), will be less and less able to protect clients in the future. Ensuring correct and safe interaction is ultimately the responsibility of the client or a middleware. The primary goal of the following project is therefore to prevent potential attacks from the Internet by ensuring that they are detected very soon in the attack chain. This also represents the greatest innovation in this project compared to the state of the art (see Figure 2). Today it is only possible to a very limited extent to detect attacks in a very early stadium. The following figure shows the technical placement of the developments.

<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/Bilder.jpg"><img width="448" height="348" src="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/Bilder.jpg"></a>

The aim of the project is to develop methods for an Intrusion Detection System - IDS" which expands the technical capability of todas IDS available on the market in two directions. On the one hand, as already mentioned above, a potential attack should be detected at a very early stage, and on the other hand, the system should have techniques that have self-learning mechanisms.

Methods for an intrusion detection system (IDS) are to be expanded so that the following general goals can be achieved:

- Combination of signature and anomaly-based methods for the detection of attacks in network traffic
- Low latency
- High detection rates of attacks
- Low number of false alarms
- Use in cloud applications

The detailled goals of the project are:
Development of formal foundations
- Formal models for internet protocols
- Development of probabilistic methods
- Research of parallization of different cores
- Synthesis of formal model into HDL

Development for the automatic detection of inturders
- Signaturebased methods to intruder atempts
- Anomaly based methods
- Robust trainings methods to detect anomaly based intruders

Test and simulation environment
- Development to simulate different intrusion atempts
- Automatic generation of xml structure for testing

## Table of Contents

1. [Requirements](#requirements)
2. [Concepts](#concepts)
3. [Getting started] (#getting) 
4. [Install](#install)
5. [Content](#content)

## Requirements
General requirements are:

Operation systems:
You need to have different OS running either native on a comptuer or in virtual containers:
- Windows 8
- Ubuntu
- MacOS
- Android
- QNX

Attack simulator:
Some parts of the software require different software packages installed to simulate attacks, be careful using them
- KaliLinux
- BlackTrack Linux
- BlackBox Linux
- Cybork Linux
You might also get a test account for IXA.

Browsers:
- Firefox
- Internet Explorer
- Opera
- Safari 
- Google Chrome

Sandboxes:
More information can be found <a href="https://securityintelligence.com/comparing-free-online-malware-analysis-sandboxes/">here</a> 
- Anubis
- VXStream
- Virustotal
- Malwr

## Concepts
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/Bilder1.jpg"><img width="448" height="348" src="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/Bilder1.jpg"></a>
The above concept was the core of the project and can be used as guide line for the software in this repro.
Requirements
-	Protects against especially cross-site-scripting attacks (the widely spread attack form)
-	should be available to Mobile, Desktop and Server
-	Analysis of web traffic http / https (later)
-	Protects company client devices from executing XSS
-	WE WANT TO prevent XSS code (dynamic or persistent) to be executed by the browser -> this will automatically prevent downloading of external malware

Feature definition
-	Ubuntu LTS in its current version shall be used (support will cover 5 years)
-	Squid as proxy
-	Our solution as squid module which shall be packet according to Ubuntu packet rules 
-	An appliance which extend existing solutions like lastline, checkpoint, etc.
-	Alarm via E-Mail
-	Syslog and/or Splunk integration
-	Dynamic aka. Reflected (incl. DOM) XSS Detection and Blocking
-	Static aka. Persistent XSS Detection and Blocking
-	XSS Detection through
-	Anomaly Detection
    - XML Detection
         - Javascript Anomaly Detection 
         - vb script anomaly detection 
         - url parameter anomaly detection
         - Website references to external urls
         - Normality has to be determined for each and every site
    - DNS rating (block everything to and from known bad urls)
    - Regular Expression (Signature)
    - URL Sanitization
    - External target script -> file download, 
    - CSS (style) detection for hidden content (size, zero, ….)
    - Cookie access

## Getting started

Environment creation is demonstrated for a Windows platform but should work similarly on Linux as well.

-	VirtualBox 4.0.26	
	http://download.virtualbox.org/virtualbox/4.0.26/VirtualBox-4.0.26-95035-Win.exe
-	Vagrant  1.7.1
	https://releases.hashicorp.com/vagrant/1.7.1/vagrant_1.7.1.msi
- 	Putty	
-	> 50GB free HDD
-	> 6GB RAM

1. Install VirtualBox 
2. Install Vagrant
3. Clone this repository using git, tortoisegit, etc.

4.  Press <kbd>Win</kbd>+<kbd>R</kbd> then enter `cmd` to open Windows Command Prompt
5.  Input: `vagrant plugin install vagrant-disksize` and hit <kbd>enter</kbd>
6.	Input: `cd \<path to this repo>\` to the root folder (where Vagrantfile is located)
7.	Input: `vagrant up develop` and hit <kbd>enter</kbd>	
	** This may take a long time to complete a couple of hours.**	
8.	if prompted to select a Network Adapter: 
	select the one you got the IP address from and connect to your network with (e.g. Realtek PCIe GBE Family Controller, Intel(R) Wireless-AC, ...)
9.	Input: `vagrant provision`
	After VM provisioning is complete (this is building the VM once and may take a long time) the VM is started automatically.	
	*You can also verify that the VM is running by opening the VirtualBox UI, but **do not** manually change any of the VMs settings using the UI!*

10.	Check the console windows for a localhost port 2222, 2200, etc.
11. Use Putty to connect to localhost and the above port

user: vagrant 
pass: key file located in <root path>\.vagrant\machines\develop\virtualbox\private_key

The key file is created after the first `vagrant up develop` command.
To use the key file on windows with putty it must be converted first
- download putty and putty key generator here: http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html
- start putty key generator
- Menu "Conversions" -> "Import Key"
- go to the private key location (above) and select the private_key file
- click the "Save private key" button in the Actions section
- accept warnings about no password specified and enter a filename e.g. private_key.ppk
- start putty
- host: localhost -> port: 22xx (the local port is printed by vagrant in command prompt)
- in the "Category" tree select: Connection -> SSH -> Auth
- in the "Authentication paramters" section click the "Browse..." button and select the private_key.ppk file
- click "open" button
- specify username (the default ssh user is printed by vagrant in command prompt)

12. Configure Squid `$ sudo nano /etc/squid3/squid.conf`

```
# ACCESS CONTROLS OPTIONS
# ====================
#
acl QUERY urlpath_regex -i cgi-bin \? \.php$ \.asp$ \.shtml$ \.cfm$ \.cfml$ \.phtml$ \.php3$ localhost
acl all src
acl localnet src 10.0.0.0/8
acl localnet src 192.168.0.0/24 # LAN mreza
# acl localhost src 127.0.0.1/32
acl safeports port 21 70 80 210 280 443 488 563 591 631 777 901 81 3128 1025-65535
acl sslports port 443 563 81 2087 10000
# acl manager proto cache_object
acl purge method PURGE
acl connect method CONNECT
acl ym dstdomain .messenger.yahoo.com .psq.yahoo.com
acl ym dstdomain .us.il.yimg.com .msg.yahoo.com .pager.yahoo.com
acl ym dstdomain .rareedge.com .ytunnelpro.com .chat.yahoo.com
acl ym dstdomain .voice.yahoo.com
acl ymregex url_regex yupdater.yim ymsgr myspaceim
#
http_access deny ym
http_access deny ymregex
http_access allow manager localhost
http_access deny manager
http_access allow purge localhost
http_access deny purge
http_access deny !safeports
http_access deny CONNECT !sslports
http_access allow localhost
http_access allow localnet
http_access deny all
#
# NETWORK OPTIONS
# —————
#
http_port 3128 transparent
#
# OPTIONS WHICH AFFECT THE CACHE SIZE
# ==============================
#
cache_mem 8 MB
maximum_object_size_in_memory 32 KB
memory_replacement_policy heap GDSF
cache_replacement_policy heap LFUDA
cache_dir aufs /home/squid/cache 10000 14 256
maximum_object_size 128000 KB
cache_swap_low 95
cache_swap_high 99
#
# LOGFILE PATHNAMES AND CACHE DIRECTORIES
# ==================================
#
access_log /var/log/squid3/access.log
cache_log /home/squid/cache/cache.log
#cache_log /dev/null
cache_store_log none
logfile_rotate 5
log_icp_queries off
#
# OPTIONS FOR TUNING THE CACHE
# ========================
#
cache deny QUERY
refresh_pattern ^ftp: 1440 20% 10080 reload-into-ims
refresh_pattern ^gopher: 1440 0% 1440
refresh_pattern -i \.(gif|png|jp?g|ico|bmp|tiff?)$ 10080 95% 43200 override-expire override-lastmod reload-into-ims ignore-no-cache ignore-private
refresh_pattern -i \.(rpm|cab|deb|exe|msi|msu|zip|tar|xz|bz|bz2|lzma|gz|tgz|rar|bin|7z|doc?|xls?|ppt?|pdf|nth|psd|sis)$ 10080 90% 43200 override-expire override-lastmod reload-into-ims ignore-no-cache ignore-private
refresh_pattern -i \.(avi|iso|wav|mid|mp?|mpeg|mov|3gp|wm?|swf|flv|x-flv|axd)$ 43200 95% 432000 override-expire override-lastmod reload-into-ims ignore-no-cache ignore-private
refresh_pattern -i \.(html|htm|css|js)$ 1440 75% 40320
refresh_pattern -i \.index.(html|htm)$ 0 75% 10080
refresh_pattern -i (/cgi-bin/|\?) 0 0% 0
refresh_pattern . 1440 90% 10080
#
quick_abort_min 0 KB
quick_abort_max 0 KB
quick_abort_pct 100
store_avg_object_size 13 KB
#
# HTTP OPTIONS
# ===========
vary_ignore_expire on
#
# ANONIMITY OPTIONS
# ===============
#
request_header_access From deny all
request_header_access Server deny all
request_header_access Link deny all
request_header_access Via deny all
request_header_access X-Forwarded-For deny all
#
# TIMEOUTS
# =======
#
forward_timeout 240 second
connect_timeout 30 second
peer_connect_timeout 5 second
read_timeout 600 second
request_timeout 60 second
shutdown_lifetime 10 second
#
# ADMINISTRATIVE PARAMETERS
# =====================
#
cache_mgr Amer
cache_effective_user proxy
cache_effective_group proxy
httpd_suppress_version_string on
visible_hostname Amer
#
#ftp_list_width 32
ftp_passive on
ftp_sanitycheck on
#
# DNS OPTIONS
# ==========
#
dns_timeout 10 seconds
dns_nameservers 192.168.1.1 8.8.8.8 8.8.4.4 # DNS Serveri
#
# MISCELLANEOUS
# ===========
#
memory_pools off
client_db off
reload_into_ims on
coredump_dir /cache
pipeline_prefetch on
offline_mode off
#
#Marking ZPH
#==========
#zph_mode tos
#zph_local 0x04
#zph_parent 0
#zph_option 136
### END CONFIGURATION ###
```

13. prepare the iptables: `$ sudo nano /etc/iptables/rules.v4`
```
*nat
-A PREROUTING -i eth0 -p tcp -m tcp --dport 80 -j DNAT --to-destination 192.168.0.1:3128
-A PREROUTING -i eth0 -p tcp -m tcp --dport 80 -j REDIRECT --to-ports 3128
-A POSTROUTING -s 192.168.0.0/24 -o eth1 -j MASQUERADE
COMMIT
```
14. load iptables `$ sudo iptables-restore < /etc/iptables/rules.v4`
15. `$ nano /etc/rc.local` and add the following line below the "exit 0" line
```
iptables -t nat -A POSTROUTING -s 192.168.0.0/24 -o eth1 -j MASQUERADE
```
16.	Restart the vm. In the Windows console window input: `vagrant halt develop` and hit <kbd>enter</kbd>
	*this will shutdown the VM*	




## Install

After installing the Vagrant file. As a general guide line follow the installation instructions the README of the subdirectories or contained in the relevante rar files.
Further instructions can be found in the documentation files in the https://github.com/cbernt/GreenDanubeCloud_InDectSys/tree/master/Green%20Danube folder.
Starting point for the project is to install the required software packages and then depending on the method which should exaimined and further developed the documentation in the mentioned folder.

## Content

 Logo  | Name  | Description | Comment                            
 :-------------------------------------: | :----------: | :--------------------------------------: | :---------- 
 <a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/tree/master/Android_Eval"><img width="48" height="48" src="https://avatars1.githubusercontent.com/u/32689599?s=200&v=4"></a> | Android_eval | scripts containing Adroid evaulation methods |  see docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/tree/master/CAP_Prototype"><img width="48" height="48" src="https://upload.wikimedia.org/wikipedia/commons/b/bb/Gitea_Logo.svg"></a> | CAP prototype | main repro containing most of the scripts and methods | see repro and docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/tree/master/Green%20Danube"><img width="48" height="48" src="https://upload.wikimedia.org/wikipedia/commons/b/bb/Gitea_Logo.svg"></a> | Green Danube | repro containing most of the documentation | see docu inside  |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/tree/master/browser_plugins"><img width="48" height="48" src="https://upload.wikimedia.org/wikipedia/commons/b/bb/Gitea_Logo.svg"></a> | brower_plugins | script to evaluate browser behaviour for different attack vectors | see repro and docu  |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/tree/master/caffe2.msvsfix"><img width="48" height="48" src="https://img.stackshare.io/service/6821/13072719.png"></a> | Caffe2 | Caffe2 toolbox to train NN | see repro and docu  |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/tree/master/cuckoo/cuckoo_detection"><img width="48" height="48" src="https://avatars2.githubusercontent.com/u/1032683?s=200&v=4"></a> | Cuckoo | Cuckoo sandbox analysis tool and scripts | see <a href="https://cuckoosandbox.org/">Cuckoo documentation</a> and docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/tree/master/gists"><img width="48" height="48" src="https://upload.wikimedia.org/wikipedia/commons/b/bb/Gitea_Logo.svg"></a> | gists | Definiton and scripts of atttacks vectors | see repro and docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/tree/master/iOS_eval"><img width="48" height="48" src="https://upload.wikimedia.org/wikipedia/commons/a/ac/IOS-Emblem.jpg"></a> | iOS_eval | scripts containing iOS evaluation methods | see docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/tree/master/ixia"><img width="48" height="48" src="https://upload.wikimedia.org/wikipedia/commons/2/22/Ixia_logo.svg"></a> | Ixia | scripts containing ixia configuration to simulate attacks | see docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/tree/master/qnx_eval"><img width="48" height="48" src="https://logos-download.com/wp-content/uploads/2019/06/QNX_Software_Systems_Logo.png"></a> | QNX_eval | scripts containing QNX evaulation methods |  see docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/tree/master/vxstream"><img width="48" height="48" src="https://upload.wikimedia.org/wikipedia/commons/b/bb/Gitea_Logo.svg"></a> | VXStream | scripts containing VXStream sandbox scripts and modifications |  see docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/Greasyspoon%20ICAP%20server%20scenario.mp4"><img width="48" height="48" src="https://upload.wikimedia.org/wikipedia/commons/b/bb/Gitea_Logo.svg"></a> | Greasyspoon_ICAP | scripts containing cap definitions | see docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/anubis.rar"><img width="48" height="48" src="https://upload.wikimedia.org/wikipedia/commons/b/bb/Gitea_Logo.svg"></a> | anubis.rar | scripts containing Anubis sandbox scripts and modifications | see rar file and docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/eCAP%20library%20scenario%20480p.mp4"><img width="48" height="48" src="https://upload.wikimedia.org/wikipedia/commons/b/bb/Gitea_Logo.svg"></a> | eCAP_library_scenario.mp4 | scripts containing cap definitions | see rar file and docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/linux_macos_eval.rar"><img width="48" height="48" src="https://upload.wikimedia.org/wikipedia/commons/3/35/Tux.svg"></a> | linux_macos_eval.rar | scripts containing Linux and MacOS evaulation methods | see rar file and docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/malwr.rar"><img width="48" height="48" src="https://www.saashub.com/images/app/service_logos/45/40b71d312d64/large.png?1555756824"></a> | malwr.rar | scripts containing Malwr sandbox scripts and modifications | see rar file and docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/matconvnet.rar"><img width="48" height="48" src="https://upload.wikimedia.org/wikipedia/commons/thumb/2/21/Matlab_Logo.png/667px-Matlab_Logo.png"></a> | matconvnet.rar | Matlab scripts to train, apply and analyze NN | see rar file and docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/ms_cntk.rar"><img width="48" height="48" src="https://upload.wikimedia.org/wikipedia/en/b/b9/Microsoft-CogTK.png"></a> | ms_cntk.rar | Microsoft toolbox to train, apply and analyze machine learning methods | see rar file and docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/rapidminer_eval.rar"><img width="48" height="48" src="https://1xltkxylmzx3z8gd647akcdvov-wpengine.netdna-ssl.com/wp-content/uploads/2016/06/rapidminer-logo-retina.png"></a> | rapidminer_eval.rar | Rapidminer toolbox to train, apply and analyze machine learning methods | see rar file and docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/spidermonkey.rar"><img width="48" height="48" src="https://avatars2.githubusercontent.com/u/43163769?s=280&v=4"></a> | spidermonkey.rar | scripts containing spidermonkey sandbox scripts and modifications | see rar file and docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/squid_matlab.rar"><img width="48" height="48" src="https://veesp.com/assets/blog/how-to-setup-squid-on-ubuntu.png"></a> | squid_matlab.rar | Matlab scripts to connect analyze data traffic using squid | see rar file and docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/virustotal.rar"><img width="48" height="48" src="https://res-5.cloudinary.com/crunchbase-production/image/upload/c_lpad,h_256,w_256,f_auto,q_auto:eco/vxwo4yr27optg1jldgjf"></a> | virustotal.rar | scripts containing Virustotal sandbox scripts and modifications | see rar file and docu |
<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/win_eval.rar"><img width="48" height="48" src="https://upload.wikimedia.org/wikipedia/commons/0/0a/Unofficial_Windows_logo_variant_-_2002%E2%80%932012_%28Multicolored%29.svg"></a> | win_eval.rar | scripts containing Windows evaulation methods and modifications | see rar file and docu |

## Support
Feel free to contact us
