# open-cbgm
Fast, compact, open-source, TEI-compliant C++ implementation of the Coherence-Based Genealogical Method

[![Build Status](https://travis-ci.com/jjmccollum/open-cbgm.svg?token=nZWB24v9ybTTZm4tWaqm&branch=master)](https://travis-ci.com/jjmccollum/open-cbgm)
[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat)](https://choosealicense.com/licenses/mit/)

## About This Project

### Introduction

The Coherence-Based Genealogical Method (CBGM) is a novel approach to textual criticism, popularized by the Institut für Neutestamentliche Textforschung (INTF) for its use in the production of the _Editio Critica Maior_ (_ECM_) of the New Testament. It is a meta-method, combining methodology-dependent philological decisions from the user with efficient computer-based calculations to highlight genealogical relationships between different stages of the text. To establish genealogical relationships in the presence of contamination (understood to be a problem in the textual tradition of the New Testament), the CBGM makes a number of philosophical and methodological innovations, such as the abstracting of texts away from the manuscripts that preserve them (and the resulting rejection of hypothetical ancestors as used in traditional stemmata), the encoding of the textual critic's decisions in local stemmata of variants, and the use of coherence in textual flow to evaluate hypotheses about the priority of variant readings. 

To learn more about the CBGM, see Tommy Wasserman and Peter J. Gurry, _A New Approach to Textual Criticism: An Introduction to the Coherence-Based Genealogical Method_, RBS 80 (Atlanta: SBL Press, 2017); Peter J. Gurry, _A Critical Examination of the Coherence-Based Genealogical Method in the New Testament_, NTTSD 55 (Leiden: Brill, 2017); Andrew Charles Edmondson, "An Analysis of the Coherence-Based Genealogical Method Using Phylogenetics" (PhD diss., University of Birmingham, 2019); and Gerd Mink, "Problems of a Highly Contaminated Tradition: The New Testament: Stemmata of Variants as a Source of Genealogy for Witnesses," in Pieter van Reenen, August den Hollander, and Margot van Mulken, eds., _Studies in Stemmatology II_ (Philadelphia, PA: Benjamins, 2004), 13–85.

### Design Philosophy

This is not the first software implementation of the CBGM. To our knowledge, the earliest software designed for this purpose is the Genealogical Queries tool developed by the INTF (http://intf.uni-muenster.de/cbgm2/GenQ.html). The underlying data is restricted to the Catholic Epistles, and both the collation data and the source code are inaccessible to the user. More recently, the INTF and the Cologne Center for eHumanities (CCeH) have made significant updates in a version used for Acts (https://github.com/cceh/ntg, web app at https://ntg.cceh.uni-koeln.de/acts/ph4/). While this updated version transparently presents its underlying collation data, the local stemmata are currently not editable, and the underlying code is not open-source. Most recently, Andrew Edmondson developed an open-source Python implementation of the CBGM (https://github.com/edmondac/CBGM, [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.1296288.svg)](https://doi.org/10.5281/zenodo.1296288)), but according to his own description, "It was designed for testing and changing the various algorithms and is not (therefore) the fastest user-facing package."

In light of the strengths and shortcomings of its predecessors, the open-cbgm library was developed with the following desired features in mind:
1. _open-source_: The code should be publicly available and free to be used and modified by anyone.
2. _compact_: The code should have a minimal memory footprint and minimal dependencies on external libraries. The input and output of the software should also be as concise as possible.
3. _fast_: The code should be optimized with performance and scalability in mind.
4. _compliant_: The input and output of the software should adhere as best as possible to established standards in the field, so as to facilitate its incorporation into established workflows.

To achieve the goal of feature (1), we have made our entire codebase available under the permissive MIT License. None of the functionality "under the hood" is obfuscated, and any user with access to the input file can customize local stemmata and connectivity parameters. (Incidentally, this fulfills a desideratum for customizability expressed by Wasserman and Gurry, _A New Approach to Textual Criticism_, 119–120.)

With respect to feature (2), the core libary makes use of only three external libraries: the cxxopts command-line argument parsing library (https://github.com/jarro2783/cxxopts), which within the core library is used only for test-running purposes; the pugixml library (https://github.com/zeux/pugixml), which allows for fast parsing of potentially large XML input files; and the CRoaring compressed bitmap library (https://github.com/RoaringBitmap/CRoaring), to encode pregenealogical and genealogical relationships succinctly and in a way that facilitates fast computation. The first two libraries were deliberately chosen for their lightweight nature, and the third was an easy choice for its excellent performance benchmarks. The only input source for the software is an XML file encoding collation data; the size of the _ECM_ collation data for a single chapter of the New Testament typically ranges between 100KB and 200KB.

For feature (3), we made performance-minded decisions at all levels of design. In addition to our deliberate choices of the external libraries mentioned above, we designed our own code with the same goal in mind. We stored data keyed by witnesses in hash tables for efficient accesses on average. Due to the combinatorial nature of the problem of substemma optimization, we implemented common heuristics known to be effective at solving similar types of problems in practice. As a result, the open-cbgm library can parse the _ECM_ collation of 3 John (consisting of 137 witnesses and 116 variation units), calculate the pre-genealogical and genealogical relationships between its witnesses,and write this data to the cache in just over two minutes, and it can generate a preliminary global stemma for the entire book in under ten seconds on a desktop computer.

Finally, for feature (4), we ensured compliance with the XML standard of the Text Encoding Initiative (TEI) by designing the software to expect an input in the TEI XML format used by the INTF and the International Greek New Testament Project (IGNTP) in their transcriptions and collations. (For specific guidelines, see TEI Consortium, eds. \[2019\], _TEI P5: Guidelines for Electronic Text Encoding and Interchange (version 3.6.0)_, manual, TEI Consortium, http://www.tei-c.org/Guidelines/P5/ \[accessed 1 January 2020\], and Houghton, H.A.G. \[2016\], _IGNTP Guidelines for XML Transcriptions of New Testament Manuscripts (version 1.5)_, manual, International Greek New Testament Project \[unpublished\].) We made this possible by encoding common features of the CBGM as additional TEI XML elements in the input collation file (e.g., the feature set `<fs/>` element for a variation unit's connectivity and the `<graph/>` element for its local stemma). Sample collation files demonstrating this encoding can be found in the examples directory.

The local stemma, textual flow, and global stemma classes are equipped with methods to serialize the contents of their graphs in .dot format. This format is the standard used by the graphviz visualization software (https://www.graphviz.org/), which in turn is the standard used by existing CBGM software.

## Installation and Dependencies

As mentioned above, the open-cbgm library uses three external libraries (cxxopts, pugixml, and roaring), but for convenience and performance, we have included the necessary headers and scripts in the library. No separate installation of these libraries is necessary.

With respect to graph outputs, the open-cbgm library does not generate image files directly; instead, for the sake of flexibility, it is designed to generate textual graph description files in .dot format, which can subsequently be converted to image files in a variety of formats using graphviz. Platform-specific instructions for installing graphviz and directions on how to get image files from .dot outputs using graphviz can be found at https://www.graphviz.org.

To install the library on your system, you can clone this Git repository or download its contents in a .zip archive (click the "Code" tab near the top of this page). If you have Git installed (https://git-scm.com/), you can clone this repository from the command-line using the command

    git clone git://github.com/jjmccollum/open-cbgm.git

This will copy the latest version of the repository to a directory called "open-cbgm" in your current directory. From the command-line, enter the new directory with the command

    cd open-cbgm

From here, you need to build the project. The precise details of how to do this will depend on your operating system, but in all cases, you will need to have the CMake toolkit installed. The platform-specific installation instructions at https://github.com/jjmccollum/open-cbgm-standalone can be used for this repository.

In the interest of modularity, and to facilitate the incorporation of this library into other applications and APIs, this repository contains only the core classes of the library, without a user-facing interface. A standalone command-line interface that uses a SQLite database as a genealogical cache is available at https://github.com/jjmccollum/open-cbgm-standalone. The core library is included in that interface's git repository as a submodule and can be installed within it using the installation instructions on that page. A web-facing API is also in the works; updates will be posted in the issues for this repository.