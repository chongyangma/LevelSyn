LevelSyn
========

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/5c6f50a130a04b6e954b4037eb5ae415)](https://app.codacy.com/gh/chongyangma/LevelSyn?utm_source=github.com&utm_medium=referral&utm_content=chongyangma/LevelSyn&utm_campaign=Badge_Grade_Settings)
[![Build Status](https://travis-ci.com/chongyangma/LevelSyn.svg?branch=master)](https://travis-ci.com/chongyangma/LevelSyn) [![Build status](https://ci.appveyor.com/api/projects/status/rppyp7wpf1un5dse?svg=true)](https://ci.appveyor.com/project/chongyangma/levelsyn)

This repository contains the source code and example data of the following publication:

> Game Level Layout from Design Specification
>
> [Chongyang Ma](http://chongyangma.com/), [Nicholas Vining](http://www.cs.ubc.ca/~nvining/), [Sylvain Lefebvre](https://www.antexel.com/sylefeb-research/), [Alla Sheffer](http://www.cs.ubc.ca/~sheffa/)
>
> In _Computer Graphics Forum (Proceedings of Eurographics 2014)_
>
> [Project page](http://chongyangma.com/publications/gl/index.html),
> [Paper](http://chongyangma.com/publications/gl/2014_gl_preprint.pdf),
> [Slides](http://chongyangma.com/publications/gl/2014_gl_slides.pdf),
> [BibTex](http://chongyangma.com/publications/gl/2014_gl_bib.txt)

Requirements
------------

The code is cross-platform and has been tested under Windows (MSVC), Linux and Mac OS X. Compiling from scratch requires the installation of [CMake](https://cmake.org/) and [Boost C++ Libraries](http://www.boost.org/). Additional third-party dependencies (included in this repo as submodules) are:
*   [Clipper 6.4.2](https://github.com/skyrpex/clipper) for polygon intersection computation
*   [TinyXML-2](https://github.com/leethomason/tinyxml2) for loading/saving xml files

Usage
-----

Launching the compiled application from command line without any argument will print the usage information (MSVC version):

```bash
levels.exe graph.xml templates.xml config.txt [target_solution_number]
```

Contact information
-------------------

Questions? Bug reports? Please send email to Chongyang Ma chongyangm@gmail.com .
