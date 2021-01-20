# GreenDanubeCloud_InDectSys
Green Danube Cloud InDectSys Repo

This repo contains all files which where used during the research project of Green Danube Cloud
This guide explains the major software contribution developed during the project.


<a href="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/Bilder.jpg"><img width="448" height="348" src="https://github.com/cbernt/GreenDanubeCloud_InDectSys/blob/master/Bilder.jpg"></a>

The goals of the project are:
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
2. [Install](#install)
3. [Concepts](#concepts)

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
    - Cookie access (??? Which script accesses the cookie, may be hard the injected code is allowed to execute inside the browser’s domain sandbox)

[![NPM Version][npm-image]][npm-url]
[![Build Status][travis-image]][travis-url]
[![Downloads Stats][npm-downloads]][npm-url]

### JSON

|                                                                                Name                                                                                |    Status    | Install Size  | Description                             |
| :----------------------------------------------------------------------------------------------------------------------------------------------------------------: | :----------: | :-----------: | :-------------------------------------- |
|             <a href="https://github.com/webpack-contrib/json-loader"><img width="48" height="48" src="https://worldvectorlogo.com/logos/json.svg"></a>             | ![json-npm]  | ![json-size]  | Loads a JSON file (included by default) |
| <a href="https://github.com/webpack-contrib/json5-loader"><img width="48" height="10.656" src="https://cdn.rawgit.com/json5/json5-logo/master/json5-logo.svg"></a> | ![json5-npm] | ![json5-size] | Loads and transpiles a JSON 5 file      |
|             <a href="https://github.com/awnist/cson-loader"><img width="48" height="48" src="https://worldvectorlogo.com/logos/coffeescript.svg"></a>              | ![cson-npm]  | ![cson-size]  | Loads and transpiles a CSON file        |

[json-npm]: https://img.shields.io/npm/v/json-loader.svg
[json-size]: https://packagephobia.com/badge?p=json-loader
[json5-npm]: https://img.shields.io/npm/v/json5-loader.svg
[json5-size]: https://packagephobia.com/badge?p=json5-loader
[cson-npm]: https://img.shields.io/npm/v/cson-loader.svg
[cson-size]: https://packagephobia.com/badge?p=cson-loader

<!-- Markdown link & img dfn's -->
[npm-image]: https://img.shields.io/npm/v/datadog-metrics.svg?style=flat-square
[npm-url]: https://npmjs.org/package/datadog-metrics
[npm-downloads]: https://img.shields.io/npm/dm/datadog-metrics.svg?style=flat-square
[travis-image]: https://img.shields.io/travis/dbader/node-datadog-metrics/master.svg?style=flat-square
[travis-url]: https://travis-ci.org/dbader/node-datadog-metrics
[wiki]: https://github.com/yourname/yourproject/wiki
