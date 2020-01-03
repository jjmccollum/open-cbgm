# open-cbgm
Fast, compact, open-source, TEI-compliant C++ implementation of the Coherence-Based Genealogical Method

[![Build Status](https://travis-ci.com/jjmccollum/open-cbgm.svg?token=nZWB24v9ybTTZm4tWaqm&branch=master)](https://travis-ci.com/jjmccollum/open-cbgm)
[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat)](https://choosealicense.com/licenses/mit/)

## About This Project

### Introduction

The Coherence-Based Genealogical Method (CBGM) is a novel approach to textual criticism, popularized by the Institut für Neutestamentliche Textforschung (INTF) for its use in the production of the _Editio Critica Maior_ (_ECM_) of the New Testament. It is a meta-method, combining methodology-dependent philological decisions from the user with efficient computer-based calculations to highlight genealogical relationships between different stages of the text. To establish genealogical relationships in the presence of contamination (understood to be a problem in the textual tradition of the New Testament), the CBGM makes a number of philosophical and methodological innovations, such as the abstracting of texts away from the manuscripts that preserve them (and the resulting rejection of hypothetical ancestors as used in traditional stemmata), the encoding of the textual critic's decisions in local stemmata of variants, and the use of coherence in textual flow to evaluate hypotheses about the priority of variant readings. 

To learn more about the CBGM, see Tommy Wasserman and Peter J. Gurry, _A New Approach to Textual Criticism: An Instroduction to the Coherence-Based Genealogical Method_, RBS 80 (Atlanta: SBL Press, 2017); Peter J. Gurry, _A Critical Examination of the Coherence-Based Genealogical Method in the New Testament_, NTTSD 55 (Leiden: Brill, 2017); and Gerd Mink, "Problems of a Highly Contaminated Tradition: The New Testament: Stemmata of Variants as a Source of Genealogy for Witnesses," in Pieter van Reenen, August den Hollander, and Margot van Mulken, eds., _Studies in Stemmatology II_ (Philadelphia, PA: Benjamins, 2004), 13–85.

### Design Philosophy

This is not the first software implementation of the CBGM. To our knowledge, the earliest software designed for this purpose is the Genealogical Queries tool developed by the INTF (http://intf.uni-muenster.de/cbgm2/GenQ.html). The underlying data is restricted to the Catholic Epistles, and both the collation data and the source code are inaccessible to the user. More recently, the INTF and the Cologne Center for eHumanities (CCeH) have made significant updates in a version used for Acts (https://ntg.cceh.uni-koeln.de/acts/ph4/). While this updated version is transparent with the collation data, the local stemmata are currently not editable, and the underlying code is not open-source. Most recently, Andrew Edmondson developed an open-source Python implementation of the CBGM (https://github.com/edmondac/CBGM, [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.1296288.svg)](https://doi.org/10.5281/zenodo.1296288)), but according to his own description, "It was designed for testing and changing the various algorithms and is not (therefore) the fastest user-facing package."

In light of the strengths and shortcomings of its predecessors, the open-cbgm library was developed with the following desired features in mind:
1. _open-source_: The code should be publicly available and free to be used and modified by anyone.
2. _compact_: The code should have a minimal memory footprint and minimal dependencies on external libraries. The input and output of the software should also be as concise as possible.
3. _fast_: The code should be optimized with performance and scalability in mind.
4. _compliant_: The input and output of the software should adhere as best as possible to established standards in the field, so as to facilitate its incorporation into established workflows.

To achieve the goal of feature (1), we have made our entire codebase available under the permissive MIT License. None of the functionality "under the hood" is obfuscated, and any user with access to the input file can customize local stemmata and connectivity parameters. (Incidentally, this fulfills a desideratum for customizability expressed by Wasserman and Gurry, _A New Approach to Textual Criticism_, 119–120.)

With respect to feature (2), our code makes use of only three external libraries: the cxxopts command-line argument parsing library (https://github.com/jarro2783/cxxopts), the pugixml XML-parsing library (https://github.com/zeux/pugixml), and the CRoaring compressed bitmap library (https://github.com/RoaringBitmap/CRoaring). The first two libraries were deliberately chosen for their lightweight nature, and the third was an easy choice for its excellent performance benchmarks. The only input source for the software is an XML file encoding collation data; the size of the _ECM_ collation data for a single chapter of New Testament typically ranges between 100KB and 200KB. At this time, the software does not employ an external database for caching collation data, as it is not needed; nevertheless, we have endeavored to make the library modular enough to make it easy to incorporate such a feature. The outputs of the script are small .dot files (ranging from 4KB to 12KB each) containing rendering information for graphs.

For feature (3), we made performance-minded decisions at all levels of design. We chose the pugixml library to ensure that the software would parse potentially long XML input files quickly. Likewise, we used Roaring bitmaps from the CRoaring library to encode pregenealogical and genealogical relationships, both to leverage hardware accelerations for common computations involving those relationships and to ensure that performance would scale gracefully with the number of variation units covered in collation input files. We stored data keyed by witnesses in hash tables for efficient accesses on average. Due to combinatorial nature of the problem of substemma optimization, we implemented common heuristics known to be effective at solving similar types of problems in practice. As a result, the open-cbgm library can parse the _ECM_ collation of 3 John (consisting of 137 witnesses and 116 variation units), calculate the pre-genealogical and genealogical relationships between its witnesses, and generate a global stemma for the entire book in two-and-a-half minutes on a desktop computer.

Finally, for feature (4), we ensured compliance with the XML standard of the Text Encoding Initiative (TEI) by designing the software to expect an input in the TEI XML format used by the INTF and the International Greek New Testament Project (IGNTP) in their transcriptions and collations. (For specific guidelines, see TEI Consortium, eds., _TEI P5: Guidelines for Electronic Text Encoding and Interchange_ 3.6.0, 16 July 2019, TEI Consortium, http://www.tei-c.org/Guidelines/P5/ \[accessed 1 January 2020\], and Myshrall, Amy C. and Kevern, Rachel and Houghton, H.A.G. \[2016\], _IGNTP Guidelines for the Transcription of Manuscripts using the Online Transcription Editor_, manual, International Greek New Testament Project, Birmingham \[Unpublished\].) We made this possible by encoding common features of the CBGM as additional TEI XML elements in the input collation file (e.g., the feature set `<fs/>` element for a variation unit's connectivity and the `<graph/>` element for its local stemma). An example for a variation unit in 3 John is shown below.

```
<app n="B25K1V1U2">
    <label>3Jo 1:1/2</label>
    <rdg n="a" wit="A Byz 01 03 018 020 025 044 049 0142 1 5 6 18 33 35 43 61 69 81 88 93 94 1180 181 206S 218 252 254 307 319 321 323 326 330 365 378 398 400 424 429 431 436 442 453 459 468 522 607 614 617 621 623 629 630 642 720 808 876 915 918 945 996 1067 1127 1175 1241 1243 1270 1292 1297 1359 1409 1448 1490 1501 1505 1523 1524 1563 1595 1609 1611 1661 1678 1718 1729 1735 1739 1751 1799 1827 1831 1832 1836 1837 1838 1842 1844 1845 1846 1852 1874 1875 1881 1890 2138 2147 2186 2200 2298 2344 2374 2412 2423 2464 2492 2541 2544 2652 2718 2774 2805 L596 L921 L938 L1141 L1281">ο</rdg>
    <rdg n="b" wit="467 2243 2818" />
    <fs>
        <f name="connectivity">
            <numeric value="10" />
        </f>
    </fs>
    <graph type="directed">
        <node n="a" />
        <node n="b" />
        <arc from="a" to="b" />
    </graph>
</app>
```

The .dot format used to describe output graphs is the standard used by the graphviz visualization software (https://www.graphviz.org/), which is the standard for existing CBGM software.

## Installation and Dependencies

As mentioned above, the open-cbgm library uses three external libraries (cxxopts, pugixml, and CRoaring), but because these libraries are compact, we have included the necessary headers and scripts in the library for convenience. With respect to graph outputs, the open-cbgm library does not generate image files directly; instead, for the sake of flexibility, it generates textual graph description files in .dot format, which can subsequently be converted to image files in a variety of formats using graphviz. Platform-specific instructions for installing graphviz are provided below, and directions on how to get image files from the .dot outputs can be found in the "Usage" section.

To begin setting up the library, you should clone the Git repository. To do this from the command-line, you'll need to install Git (https://git-scm.com/). To clone this repository from the command-line, use the command

    git clone git://github.com/jjmccollum/open-cbgm.git

You should now have the contents of the repository in a directory called "open-cbgm." From the command-line, enter this directory with the command

    cd open-cbgm

From here, you need to build the project. The precise details of how to do this will depend on your operating system, but in all cases, you will need to have the CMake toolkit installed. We will provide platform-specific installation instructions below.

### Linux

To install CMake on Debian variants of Linux, the command is

    sudo apt-get install cmake

Once you have CMake installed, you can complete the build with a few small lines. From the open-cbgm directory, create a build directory to store the build, enter it, and complete the build using the following commands:

    mkdir build
    cd build
    cmake ..
    make
    
Once these commands have executed, you should have all of the executable scripts added to the open-cbgm/build/src directory.

If graphviz is not installed on your system, then you can install it via the command

    sudo apt-get install graphviz
    
### MacOS
    
The setup for MacOS is virtually identical to that of Linux. If you want to install CMake from the command-line, it is recommended that you install the necessary packages with Homebrew (https://brew.sh/). If you are using Homebrew, simply enter the command

    brew install cmake
    
Then, from the open-cbgm directory, enter the following commands:

    mkdir build
    cd build
    cmake ..
    make
    
Once these commands have executed, you should have all of the executable scripts added to the open-cbgm/build/src directory.

If graphviz is not installed on your system, then you can install it via the command

    brew install graphviz

### Windows

If you want to install CMake from the command-line, it is recommended that you install it with the Chocolately package manager (https://chocolatey.org/). If you are using Chocolately, simply enter the command

    choco install -y cmake

If you do this, you'll want to make sure that your `PATH` environment variable includes the path to CMake. Alternatively, you can download the CMake user interface from the official website (https://cmake.org/) and use that.

How you proceed from here depends on whether you compile the code using Microsoft Visual Studio or a suite of open-source tools like MinGW. You can install the Community version of Microsft Visual Studio 2019 for free by downloading it from https://visualstudio.microsoft.com/vs/, or you can install it from the command-line via

    choco install -y visualstudio2019community
    
(Note that you will need to restart your computer after installing Visual Studio.) When you install Visual Studio, make sure you include the C++ Desktop Development workload necessary for building with CMake. If you install from the command-line using Chocolatey, you can do this with the command

    choco install -y visualstudio2019-workload-nativedesktop

You can install MinGW by downloading it from http://www.mingw.org/, or you can install it from the command-line via

    choco install -y mingw
    
If you are compiling with Visual Studio, then from the open-cbgm directory, enter the following commands:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release
    
Once these commands have executed, you should have all of the executable scripts added to the open-cbgm\\build\\src\\Release directory. Note that they will have the Windows `.exe` suffix, unlike the executables on Linux and MacOS.

If you are compiling with MinGW, then from the open-cbgm directory, enter the following commands:

    mkdir build
    cd build
    cmake -DCMAKE_SH="CMAKE_SH-NOTFOUND" -G "MinGW Makefiles" ..
    mingw32-make
    
Once these commands have executed, you should have all of the executable scripts added to the open-cbgm\\build\\src directory. Note that they will have the Windows `.exe` suffix, unlike the executables on Linux and MacOS.
    
**(TODO: Need to debug MinGW build!)**

If graphviz is not installed on your system, then you can install it using Chocolately via the command

    choco install -y graphviz
    
Alternatively, you can download the library from https://www.graphviz.org/ and install it manually.

## Usage

When built, the open-cbgm library contains four executable scripts: compare\_witnesses, find\_relatives, optimize\_substemmata, and print\_graphs. The first two scripts correspond to modules with the same names in both versions of the INTF's Genealogical Queries tool. The third offers functionality that is offered only partially or not at all in the Genealogical Queries tool. The fourth generates graphs that can be displayed one variation unit at a time in the Genealogical Queries tool. We will provide usage examples and illustrations for each script in the subsections that follow. For these examples, we assume that you have copied all four executables from their locations in the build directory to the main open-cbgm directory; you can do this by hand or on the command-line via the `cp` command. The example commands appear as they would be entered on Linux and MacOS; for Windows, add the `.exe` suffix to the executable.

All of these scripts take the input collation XML file as a required command-line argument, and they all accept the following optional arguments related to processing it:
- `-t` or `--threshold`, which will set a threshold of minimum extant passages for witnesses to be included from the collation. For example, the argument `-t 100` will filter out any witnesses extant in fewer than 100 passages.
- `--split`, `--orth`, and `--def`, which will treat readings labelled in the XML input as split, orthographic, or defective as distinct for the purposes of witness comparison. These flags can be provided in any combination.

To illustrate the difference, we present two versions of the local stemma for the variation unit at 3 John 1:4/22–26, one with split readings counted as distinct (i.e., using the argument `--split`), and the other with split readings and defective readings counted as distinct (i.e., using argument `--split --def`).

![3 John 1:4/22–26, split readings distinct](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26-local-stemma-split.png)
![3 John 1:4/22–26, split and defective readings distinct](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26-local-stemma-split-def.png)

### Comparison of Witnesses

The compare\_witnesses script is based on the "Comparison of Witnesses" module of the Genealogical Queries tool, but our implementation adds some flexibility. While the Genealogical Queries module only allows for the comparison of two witnesses at a time, compare\_witnesses can compare a given witness with any number of other specified witnesses. If our input XML collation file is the file example/3\_john\_collation.xml and we wanted to compare witness with ID 5 with the witnesses with IDs 03, 35, 88, 453, 1611, and 1739, then we would enter the following command from the open-cbgm directory:

    ./compare_witnesses examples/3_john_collation.xml 5 03 35 88 453 1611 1739
    
The output of the script will resemble that of the Genealogical Queries module, but with additional rows in the comparison table. The rows are sorted in descending order of their PERC values (i.e., their percentage of agreement with the primary witness in the passages where both witnesses are extant).

![Comparison of witness 5 with specified witnesses](https://github.com/jjmccollum/open-cbgm/blob/master/images/compare_witnesses_5_03_35_88_453_1611_1739.png)

We can go even further. If we do not specify any other witnesses explicitly, then the script will compare the one witness specified with all other witnesses taken from the collation. So the command

    ./compare_witnesses examples/3_john_collation.xml 5
    
will return a table of genealogical comparisons between the witness with ID 5 and all other collated witnesses, as shown below.

![Comparison of witness 5 with all other witnesses](https://github.com/jjmccollum/open-cbgm/blob/master/images/compare_witnesses_5.png)

Naturally, a comparison with all other witnesses will take longer to process than one with one or a few other witnesses, but the difference in practice is between three seconds and a fraction of a second.

### Finding Relatives

The find\_relatives script is based on the "Comparison of Witnesses" module of the Genealogical Queries tool, but our implementation adds some flexibility. For a given witness and variation unit address, the script outputs a table of genealogical comparisons between the given witness and all other collated witnesses, just like the compare\_witnesses script does, but with an additional column indicating the readings of the other witnesses at the given variation unit. Following our earlier examples, if we want to list the relatives of witness 5 at 3 John 1:4/22–26 (the variation unit whose corresponding ID in the XML collation file is "B25K1V4U22-26"), then we would enter

    ./find_relatives examples/3_john_collation.xml 5 B25K1V4U22-26
    
This will produce an output like the one shown below.

![Relatives of witness 5 at 3 John 1:4/22–26](https://github.com/jjmccollum/open-cbgm/blob/master/images/find_relatives_5.png)

If we want to filter the results for just those supporting reading d at this variation unit (which is how the Genealogical Queries module works by default), then we would add the optional argument `-r d` before the required arguments as follows:

    ./find_relatives examples/3_john_collation.xml -r d 5 B25K1V4U22-26
    
This will produce the output shown below.

![Relatives of witness 5 with reading d at 3 John 1:4/22–26](https://github.com/jjmccollum/open-cbgm/blob/master/images/find_relatives_5_rdg_d.png)

### Substemma Optimization

In order to construct a global stemma, we need to isolate, for each witness, the most promising candidates for its ancestors in the global stemma. This process is referred to as _substemma optimization_. In order for a set of potential ancestors to constitute a feasible substemma for a witness, every extant reading in the witness must be explained by a reading in at least one of the potential ancestors. In order for a set of potential ancestors to constitute an optimal substemma for a witness, the total number of disagreements between that witness and each of its potential ancestors must be minimal.

To get the optimal substemma for witness 5 in 3 John, we would enter the following command:

    ./optimize_substemmata examples/3_john_collation.xml 5
    
This will produce the output displayed below.

![Optimal substemma of witness 5 in 3 John](https://github.com/jjmccollum/open-cbgm/blob/master/images/optimize_substemmata_5.png)

This method is guaranteed to return a single substemma that is both feasible and optimal. In some cases, though, there may be multiple valid substemmata with the same cost, or there may be a valid substemma with a higher cost that we would consider preferable on other philological grounds. If we have an upper bound on the costs of substemmata we want to consider, then we can enumerate all feasible substemmata within that bound instead. For instance, if we want to consider all feasible substemmata for witness 5 with costs at or below 17, then we would use the optional argument `-b 17` before the required argument as follows:

    ./optimize_substemmata -b 17 examples/3_john_collation.xml 5
    
This will produce an output like the one pictured below.

![Substemmata of witness 5 with costs within 17 in 3 John](https://github.com/jjmccollum/open-cbgm/blob/master/images/optimize_substemmata_5_bound_17.png)

Be aware that specifying too high an upper bound may cause the procedure to take a long time.

### Generating Graphs

The two main steps in the iterative workflow of the CBGM are the formulation of hypotheses about readings in local stemmata and the evaluation and refinement of these hypotheses using textual flow diagrams. Ideally, the end result of the process will be a global stemma consisting of all witnesses and their optimized substemmata. The open-cbgm library has full functionality to generate textual graph description files for local stemmata, textual flow diagrams, and global stemmata.

All of this is accomplished with the print\_graphs script. It accepts the following optional arguments indicating which specific graph types to generate:
- `--local`, which will generate local stemmata graphs for all variation units.
- `--flow`, which will generate complete textual flow diagrams for all variation units. A complete textual flow diagram contains all witnesses, highlighting edges of textual flow that involve changes in readings.
- `--attestations`, which will generate coherence in attestations textual flow diagrams for all readings in all variation units. A coherence in attestations diagram highlights just the witnesses that support a given reading and any witnesses with different readings that are textual flow ancestors of these witnesses.
- `--variants`, which will generate coherence in variant passages textual flow diagrams for all variation units. A coherence in variant passages diagram highlights just the textual flow relationships that involve changes in readings.
- `--global`, which will generate a single global stemma graph.

These arguments can be provided in any combination. If none of them is provided, then it is assumed that the user wants all graphs to be generated. So the command

    ./print_graphs examples/3_john_collation.xml
    
will generate all graphs for the 3 John collation, while

    ./print_graphs --local examples/3_john_collation.xml
    
will generate just the local stemma graphs, and

    ./print_graphs --flow --attestations --variants examples/3_john_collation.xml
    
will generate all three types of textual flow diagrams.

The generated outputs are not image files, but .dot files, which contain textual descriptions of the graphs. To render the image files from these files, we must use the dot program from the graphviz library. As an example, if the graph description file for the local stemma of 3 John 1:4/22–26 is B25K1V4U22-26-local-stemma.dot, then the command

    dot -Tpng B25K1V4U22-26-local-stemma.dot -O
    
will generate a PNG image file called B25K1V4U22-26-local-stemma.dot.png. (If you want to specify your own output file name, use the `-o` argument followed by the file name you want.)

Sample images of local stemmata have already been included at the beginning of the "Usage" section. For the sake of completeness, we have included sample images of the other types of graphs below.

Complete textual flow diagram for 3 John 1:4/22–26:
![3 John 1:4/22–26 complete textual flow diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26-textual-flow.png)

Coherence in attestations diagrams for all readings in 3 John 1:4/22–26 (substantive readings only):
![3 John 1:4/22–26a coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26Ra-coherence-attestations.png)
![3 John 1:4/22–26b coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26Rb-coherence-attestations.png)
![3 John 1:4/22–26c coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26Rc-coherence-attestations.png)
![3 John 1:4/22–26d coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26Rd-coherence-attestations.png)

Coherence in variant passages diagram for 3 John 1:4/22–26:
![3 John 1:4/22–26 coherence in variant passages diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26-coherence-variants.png)

Complete global stemma for 3 John:
![3 John 1:4/22–26a coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/global-stemma.png)

## Future Development

While we feel that we have achieved all of the basic objectives with the current release of the open-cbgm library, there is clearly room for improvement on a number of fronts. We will briefly outline what we consider to be the clearest and most promising avenues of future development.

1. User interface: While we have attempted to provide thorough documentation on the usage of the library's executable scripts, we realize that command-line interfaces are not particularly user-friendly, especially for users whose background is in the humanities and not in computer science. A graphical user interface (GUI) based on the library would be ideal. A relatively straightforward approach would be one like that of the INTF's Genealogical Queries tool: the library executes all necessary computations on a server, and it sends results to and receives requests from a client-side webpage, which uses standard JavaScript libraries like D3 (https://d3js.org/) to render the input forms and results in a user-friendy graphical format.
2. Genealogical cache: As the library is tested on more extensive corpora of collation data, it will likely become more prudent to separate the one-time work of parsing the input XML file and calculating the genealogical relationships between witnesses from the routine tasks handled by the executable scripts. A database for caching genealogical relationships would facilitate such a solution. There is strong precedent for the use of databases for this purpose, as this is the approach taken in the Genealogical Queries tool and in Andrew Edmondson's Python implementation of the CBGM. Following Edmondson's example, the SQLite C library (https://www.sqlite.org/) would probably be the best choice for our codebase. If a database is incorporated into the library, then the input-related command-line arguments common to all of the scripts (namely, the `-t`, `--split`, `--orth`, and `--def` arguments) should be removed from these scripts and relegated to a separate script for initially populating the database. The use of a database would also enable the efficient generation of graphs for individual variation units, so the print\_graphs script could be modified not to require the generation of graphs for all variation units at one time.
3. Pre-CBGM workflow: Presently, the user must add encodings of connectivity values and local stemmata to each variation unit in a TEI XML collation file manually. While this allows practitioners to customize the method fully, the process is not user-friendly. An interface for adding or modifying local stemmata and setting or updating connectivity values to variation units in a collation file would fit nicely between existing tools that collate TEI XML transcriptions (e.g., those at https://itsee-wce.birmingham.ac.uk/) and the CBGM software.
