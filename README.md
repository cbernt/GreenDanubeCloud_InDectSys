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

text

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
