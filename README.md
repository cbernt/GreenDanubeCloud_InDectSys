# GreenDanubeCloud_InDectSys
Green Danube Cloud InDectSys Repo

This repo contains all files which where used during the research project of Green Danube Cloud
This guide explains the major software contribution developed during the project
## Table of Contents

1. [Install](#install)
2. [Introduction](#introduction)
3. [Concepts](#concepts)


webpack is a bundler for modules. The main purpose is to bundle JavaScript
files for usage in a browser, yet it is also capable of transforming, bundling,
or packaging just about any resource or asset.

**TL;DR**

- Bundles [ES Modules](https://www.2ality.com/2014/09/es6-modules-final.html), [CommonJS](http://wiki.commonjs.org/), and [AMD](https://github.com/amdjs/amdjs-api/wiki/AMD) modules (even combined).
- Can create a single bundle or multiple chunks that are asynchronously loaded at runtime (to reduce initial loading time).
- Dependencies are resolved during compilation, reducing the runtime size.
- Loaders can preprocess files while compiling, e.g. TypeScript to JavaScript, Handlebars strings to compiled functions, images to Base64, etc.
- Highly modular plugin system to do whatever else your application requires.

### Get Started

#### JSON

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
