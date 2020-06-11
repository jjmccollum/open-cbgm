# open-cbgm
Fast, compact, open-source, TEI-compliant C++ implementation of the Coherence-Based Genealogical Method

[![Build Status](https://travis-ci.com/jjmccollum/open-cbgm.svg?token=nZWB24v9ybTTZm4tWaqm&branch=master)](https://travis-ci.com/jjmccollum/open-cbgm)
[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat)](https://choosealicense.com/licenses/mit/)

## About This Project

### Introduction

The Coherence-Based Genealogical Method (CBGM) is a novel approach to textual criticism, popularized by the Institut für Neutestamentliche Textforschung (INTF) for its use in the production of the _Editio Critica Maior_ (_ECM_) of the New Testament. It is a meta-method, combining methodology-dependent philological decisions from the user with efficient computer-based calculations to highlight genealogical relationships between different stages of the text. To establish genealogical relationships in the presence of contamination (understood to be a problem in the textual tradition of the New Testament), the CBGM makes a number of philosophical and methodological innovations, such as the abstracting of texts away from the manuscripts that preserve them (and the resulting rejection of hypothetical ancestors as used in traditional stemmata), the encoding of the textual critic's decisions in local stemmata of variants, and the use of coherence in textual flow to evaluate hypotheses about the priority of variant readings. 

To learn more about the CBGM, see Tommy Wasserman and Peter J. Gurry, _A New Approach to Textual Criticism: An Introduction to the Coherence-Based Genealogical Method_, RBS 80 (Atlanta: SBL Press, 2017); Peter J. Gurry, _A Critical Examination of the Coherence-Based Genealogical Method in the New Testament_, NTTSD 55 (Leiden: Brill, 2017); Andrew Charles Edmondson, "An Analysis of the Coherence-Based Genealogical Method Using Phylogenetics" (PhD diss., University of Birmingham, 2019); and Gerd Mink, "Problems of a Highly Contaminated Tradition: The New Testament: Stemmata of Variants as a Source of Genealogy for Witnesses," in Pieter van Reenen, August den Hollander, and Margot van Mulken, eds., _Studies in Stemmatology II_ (Philadelphia, PA: Benjamins, 2004), 13–85.

### Design Philosophy

This is not the first software implementation of the CBGM. To our knowledge, the earliest software designed for this purpose is the Genealogical Queries tool developed by the INTF (http://intf.uni-muenster.de/cbgm2/GenQ.html). The underlying data is restricted to the Catholic Epistles, and both the collation data and the source code are inaccessible to the user. More recently, the INTF and the Cologne Center for eHumanities (CCeH) have made significant updates in a version used for Acts (https://ntg.cceh.uni-koeln.de/acts/ph4/). While this updated version is transparent with the collation data, the local stemmata are currently not editable, and the underlying code is not open-source. Most recently, Andrew Edmondson developed an open-source Python implementation of the CBGM (https://github.com/edmondac/CBGM, [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.1296288.svg)](https://doi.org/10.5281/zenodo.1296288)), but according to his own description, "It was designed for testing and changing the various algorithms and is not (therefore) the fastest user-facing package."

In light of the strengths and shortcomings of its predecessors, the open-cbgm library was developed with the following desired features in mind:
1. _open-source_: The code should be publicly available and free to be used and modified by anyone.
2. _compact_: The code should have a minimal memory footprint and minimal dependencies on external libraries. The input and output of the software should also be as concise as possible.
3. _fast_: The code should be optimized with performance and scalability in mind.
4. _compliant_: The input and output of the software should adhere as best as possible to established standards in the field, so as to facilitate its incorporation into established workflows.

To achieve the goal of feature (1), we have made our entire codebase available under the permissive MIT License. None of the functionality "under the hood" is obfuscated, and any user with access to the input file can customize local stemmata and connectivity parameters. (Incidentally, this fulfills a desideratum for customizability expressed by Wasserman and Gurry, _A New Approach to Textual Criticism_, 119–120.)

With respect to feature (2), our code makes use of only four external libraries: the cxxopts command-line argument parsing library (https://github.com/jarro2783/cxxopts), the pugixml XML-parsing library (https://github.com/zeux/pugixml), the SQLite database library (https://www.sqlite.org), and the CRoaring compressed bitmap library (https://github.com/RoaringBitmap/CRoaring). The first two libraries were deliberately chosen for their lightweight nature, the third boasts a relatively small memory footprint, and the fourth was an easy choice for its excellent performance benchmarks. The only input source for the software is an XML file encoding collation data; the size of the _ECM_ collation data for a single chapter of New Testament typically ranges between 100KB and 200KB. The size of a genealogical cache SQLite database generated by this library will typically take up several MB of space. The outputs of the script are small .dot files (ranging from 4KB to 12KB each) containing rendering information for graphs.

For feature (3), we made performance-minded decisions at all levels of design. We chose the pugixml library to ensure that the software would parse potentially long XML input files quickly. SQLite databases are known for their speed, and following Edmondson's lead, we have optimized our database's query performance with denormalization. Likewise, we used Roaring bitmaps from the CRoaring library to encode pregenealogical and genealogical relationships, both to leverage hardware accelerations for common computations involving those relationships and to ensure that performance would scale gracefully with the number of variation units covered in collation input files. We stored data keyed by witnesses in hash tables for efficient accesses on average. Due to combinatorial nature of the problem of substemma optimization, we implemented common heuristics known to be effective at solving similar types of problems in practice. As a result, the open-cbgm library can parse the _ECM_ collation of 3 John (consisting of 137 witnesses and 116 variation units), calculate the pre-genealogical and genealogical relationships between its witnesses, and write this data to the cache in just over two minutes, and it can generate a global stemma for the entire book in under ten seconds on a desktop computer.

Finally, for feature (4), we ensured compliance with the XML standard of the Text Encoding Initiative (TEI) by designing the software to expect an input in the TEI XML format used by the INTF and the International Greek New Testament Project (IGNTP) in their transcriptions and collations. (For specific guidelines, see TEI Consortium, eds. \[2019\], _TEI P5: Guidelines for Electronic Text Encoding and Interchange (version 3.6.0)_, manual, TEI Consortium, http://www.tei-c.org/Guidelines/P5/ \[accessed 1 January 2020\], and Houghton, H.A.G. \[2016\], _IGNTP Guidelines for XML Transcriptions of New Testament Manuscripts (version 1.5)_, manual, International Greek New Testament Project \[unpublished\].) We made this possible by encoding common features of the CBGM as additional TEI XML elements in the input collation file (e.g., the feature set `<fs/>` element for a variation unit's connectivity and the `<graph/>` element for its local stemma). Sample collation files demonstrating this encoding can be found in the examples directory.

The .dot format used to describe output graphs is the standard used by the graphviz visualization software (https://www.graphviz.org/), which is the standard for existing CBGM software.

## Installation and Dependencies

As mentioned above, the open-cbgm library uses four external libraries (cxxopts, pugixml, sqlite3, and roaring), but for convenience and performance, we have included the necessary headers and scripts in the library for convenience. With respect to graph outputs, the open-cbgm library does not generate image files directly; instead, for the sake of flexibility, it generates textual graph description files in .dot format, which can subsequently be converted to image files in a variety of formats using graphviz. Platform-specific instructions for installing graphviz are provided below, and directions on how to get image files from the .dot outputs can be found in the "Usage" section.

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

If graphviz is not installed on your system, then you can install it using Chocolately via the command

    choco install -y graphviz
    
Alternatively, you can download the library from https://www.graphviz.org/ and install it manually.

## Usage

When built, the open-cbgm library contains seven executable scripts: populate\_db, compare\_witnesses, find\_relatives, optimize\_substemmata, print\_local\_stemma, print\_textual\_flow, and print\_global\_stemma. The first script reads the input XML file containing collation and local stemma data and uses it to populate a local database from which the remaining scripts can quickly retrieve needed data. The second and third correspond to modules with the same names in both versions of the INTF's Genealogical Queries tool. The fourth offers functionality that is offered only partially or not at all in the Genealogical Queries tool. The fifth, sixth, and seventh generate graphs similar to those offered by the Genealogical Queries tool. We will provide usage examples and illustrations for each script in the subsections that follow. For these examples, we assume that you have copied all executables from their locations in the build directory to the main open-cbgm directory; you can do this by hand or on the command-line via the `cp` command. The example commands appear as they would be entered on Linux and MacOS; for Windows, you will need to add the `.exe` suffix to the executable.

### Population of the Genealogical Cache

The populate\_db script reads the input collation XML file, calculates genealogical relationships between all pairs of witnesses, and writes this and other data needed for common CBGM tasks to a SQLite database. Typically, this process will take at least a few minutes, depending on the number of variation units and witnesses in the collation, but the use of a database is intended to make this process one-time work. The script takes the input XML file as a required command-line argument, and it also accepts the following optional arguments for processing the data:
- `-t` or `--threshold`, which will set a threshold of minimum extant passages for witnesses to be included from the collation. For example, the argument `-t 100` will filter out any witnesses extant in fewer than 100 passages.
- `-z` followed by a reading type (e.g., `-z defective`), which will treat readings of that type as trivial for the purposes of witness comparison (so using the example already provided, a defective or orthographic subvariant of a reading would be considered to agree with that reading). This argument can be repeated with different reading types (e.g., `-z defective -z orthographic`).
- `-Z`, followed by a reading type (e.g., `-Z lac`), which will ignore readings of that type, excluding them from variation units and local stemmata. Any witnesses having such readings will be treated as lacunose in the variation units where they have them. This argument can be repeated with different reading types (e.g., `-Z lac -Z ambiguous`).
- `--merge-splits`, which will treat split attestations of the same reading as equivalent for the purposes of witness comparison.

Finally, please note that at this time, the current database must be overwritten, or a separate one must be created, in order to incorporate any changes to the processing options or to the local stemmata.

As an example, if we wanted to create a new database called cache.db using the 3\_john\_collation.xml collation file in the examples directory, excluding ambiguous readings and witnesses with fewer than 100 extant readings while treating orthographic and defective subvariation as trivial, then we would use the following command:

	./populate_db -t 100 -z defective -z orthographic -Z ambiguous examples/3_john_collation.xml cache.db

We note that the 3\_john\_collation.xml file encodes lacunae implicitly (so that any witness not attesting to a specified reading is assumed to be lacunose), so lacunae do not have to be excluded explicitly. If we were using a minimally modified output from the ITSEE collation editor (https://github.com/itsee-birmingham/standalone_collation_editor), like the john\_6\_23\_collation.xml example, then we would want to specify that readings of type "lac" be excluded:

    ./populate_db -t 100 -z defective -z orthographic -Z lac -Z ambiguous examples/john_6_23_collation.xml cache.db

To illustrate the effects of the processing arguments, we present several versions of the local stemma for the variation unit at 3 John 1:4/22–26, along with the commands used to populate the database containing their data. In the local stemmata presented below, dashed arrows represent edges of weight 0.

	./populate_db examples/3_john_collation.xml cache.db

![3 John 1:4/22–26, no processing](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26-local-stemma.png)

	./populate_db -z ambiguous -z defective examples/3_john_collation.xml cache.db

![3 John 1:4/22–26, ambiguous and defective readings trivial](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26-local-stemma-amb-def.png)

	./populate_db -Z ambiguous --merge-splits examples/3_john_collation.xml cache.db

![3 John 1:4/22–26, ambiguous readings dropped and split readings merged](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26-local-stemma-drop-merge.png)

	./populate_db -z defective -Z ambiguous --merge-splits examples/3_john_collation.xml cache.db

![3 John 1:4/22–26, ambiguous readings dropped, split readings merged, defective readings trivial](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26-local-stemma-drop-merge-def.png)

In the sections that follow, we will assume that the genealogical cache has been populated using the `-Z ambiguous` argument.

### Comparison of Witnesses

The compare\_witnesses script is based on the "Comparison of Witnesses" module of the Genealogical Queries tool, but our implementation adds some flexibility. While the Genealogical Queries module only allows for the comparison of two witnesses at a time, compare\_witnesses can compare a given witness with any number of other specified witnesses. If our genealogical cache is stored in the database cache.db and we wanted to compare the witness with ID 5 with the witnesses with IDs 03, 35, 88, 453, 1611, and 1739, then we would enter the following command from the open-cbgm directory:

    ./compare_witnesses cache.db 5 03 35 88 453 1611 1739
    
The output of the script will resemble that of the Genealogical Queries module, but with additional rows in the comparison table. The rows are sorted in descending order of their number of agreements with the primary witness.

![Comparison of witness 5 with specified witnesses](https://github.com/jjmccollum/open-cbgm/blob/master/images/compare_witnesses_5_03_35_88_453_1611_1739.png)

We can go even further. If we do not specify any other witnesses explicitly, then the script will compare the one witness specified with all other witnesses taken from the collation. So the command

    ./compare_witnesses cache.db 5
    
will return a table of genealogical comparisons between the witness with ID 5 and all other collated witnesses, as shown below.

![Comparison of witness 5 with all other witnesses](https://github.com/jjmccollum/open-cbgm/blob/master/images/compare_witnesses_5.png)

### Finding Relatives

The find\_relatives script is based on the "Comparison of Witnesses" module of the Genealogical Queries tool, but our implementation adds some flexibility. For a given witness and variation unit address, the script outputs a table of genealogical comparisons between the given witness and all other collated witnesses, just like the compare\_witnesses script does, but with an additional column indicating the readings of the other witnesses at the given variation unit. Following our earlier examples, if we want to list the relatives of witness 5 at 3 John 1:4/22–26 (whose ID in the XML collation file is "B25K1V4U22-26"), then we would enter

    ./find_relatives cache.db 5 B25K1V4U22-26
    
This will produce an output like the one shown below.

![Relatives of witness 5 at 3 John 1:4/22–26](https://github.com/jjmccollum/open-cbgm/blob/master/images/find_relatives_5.png)

If we want to filter the results for just those supporting reading d at this variation unit (which is how the Genealogical Queries module works by default), then we would add the optional argument `-r d` before the required arguments as follows:

    ./find_relatives cache.db -r d 5 B25K1V4U22-26
    
This will produce the output shown below.

![Relatives of witness 5 with reading d at 3 John 1:4/22–26](https://github.com/jjmccollum/open-cbgm/blob/master/images/find_relatives_5_rdg_d.png)

### Substemma Optimization

In order to construct a global stemma, we need to isolate, for each witness, the most promising candidates for its ancestors in the global stemma. This process is referred to as _substemma optimization_. In order for a set of potential ancestors to constitute a feasible substemma for a witness, every extant reading in the witness must be explained by a reading in at least one of the potential ancestors. In order for a set of potential ancestors to constitute an optimal substemma for a witness, a particular objective function of the ancestors in the substemma must be minimized. 

To the best of our knowledge, existing implementations of the CBGM lack a well-defined objective function. To alleviate this problem and to address some other methodological limitations of existing substemma optimization procedures, the open-cbgm library uses a specially defined genealogical cost to provide an effective objective function. In the context of a local stemma, we define the _genealogical cost_ of one reading arising from another as the length of the shortest path from the prior reading to the posterior reading in the local stemma. If no path between the readings exists, then the cost is undefined. In the context of a witness and one of its potential ancestors, then the genealogical cost is defined as the sum of genealogical costs at every variation unit where they are defined. In practice, substemmata with lower genealogical costs tend to exhibit more parsimony (i.e., they consist of few ancestors) and explain more of the witness's variants by agreement than other substemmata.

To get the optimal substemma for witness 5 in 3 John, we would enter the following command:

    ./optimize_substemmata cache.db 5
    
This will produce the output displayed below.

![Optimal substemma of witness 5 in 3 John](https://github.com/jjmccollum/open-cbgm/blob/master/images/optimize_substemmata_5.png)

The `COST` column contains the total genealogical cost of the substemma. The `AGREE` column contains the total number of variants in the primary witness that can be explained by agreement with at least one stemmatic ancestor.

This method is guaranteed to return at least one substemma that is both feasible and optimal, as long as the witness under consideration does not have equal priority to the _Ausgangstext_ (which typically only happens for fragmentary witnesses) and none of its readings have unclear sources. (Both problems can be solved by filtering out witnesses that are too lacunose and ensuring that all readings are explained in all local stemmata.) In some cases, there may be multiple valid substemmata with the same cost, or there may be a valid substemma with a higher cost that we would consider preferable on other grounds (e.g., it has a significantly higher value in the `AGREE` column. If we have an upper bound on the costs of substemmata we want to consider, then we can enumerate all feasible substemmata within that bound instead. For instance, if we want to consider all feasible substemmata for witness 5 with costs at or below 10, then we would use the optional argument `-b 10` before the required argument as follows:

    ./optimize_substemmata -b 10 cache.db 5
    
This will produce the following output:

![Substemmata of witness 5 with costs within 10 in 3 John](https://github.com/jjmccollum/open-cbgm/blob/master/images/optimize_substemmata_5_bound_10.png)

Be aware that specifying too high an upper bound may cause the procedure to take a long time.

### Generating Graphs

The two main steps in the iterative workflow of the CBGM are the formulation of hypotheses about readings in local stemmata and the evaluation and refinement of these hypotheses using textual flow diagrams. Ideally, the end result of the process will be a global stemma consisting of all witnesses and their optimized substemmata. The open-cbgm library has full functionality to generate textual graph description files for all diagrams used in the method.

All of this is accomplished with the print\_local\_stemma, print\_textual\_flow, and print\_global\_stemma scripts. The first requires at least one input (the genealogical cache database), but it can accept one or more variation unit IDs as additional arguments, in which case it will only generate graphs for the local stemmata at those variation units. If no variation units are specified, then local stemmata will be generated for all of them. In addition, it accepts an optional `--weights` argument, which will add edge weights to the generated local stemma graphs. To generate the local stemma graph for 3 John 1:4/22–26, like the ones included above, you can use the command

    ./print_local_stemma cache.db B25K1V4U22-26

The print\_textual\_flow script accepts the same positional inputs (the database and, if desired, a list of specific variation units  whose textual flow diagrams are desired), along with the following optional arguments indicating which specific graph types to generate:
- `--flow`, which will generate complete textual flow diagrams. A complete textual flow diagram contains all witnesses, highlighting edges of textual flow that involve changes in readings.
- `--attestations`, which will generate coherence in attestations textual flow diagrams for all readings in each variation unit. A coherence in attestations diagram highlights the genealogical coherence of a reading by displaying just the witnesses that support a given reading and any witnesses with different readings that are textual flow ancestors of these witnesses.
- `--variants`, which will generate coherence in variant passages textual flow diagrams. A coherence in variant passages diagram highlights just the textual flow relationships that involve changes in readings.

These arguments can be provided in any combination. If none of them is provided, then it is assumed that the user wants all graphs to be generated. In addition, a `--strengths` argument can be provided, which will format textual flow edges to highlight flow strength, per Edmondson's recommendation.

The print\_global\_stemma script requires at least one input (the database), but it also accepts an optional `--format-edges` argument, which will draw edges connecting stemmatic ancestors to their descendants as dotted, dashed, or solid based on the proportion of passages where they agree. It optimizes the substemmata of all witnesses (choosing the first option in case of ties), then combines the substemmata into a single global stemma. While this will produce a complete global stemma automatically, the resulting graph should be considered a "first-pass" result; users are strongly encouraged to run the optimize\_substemmata script for individual witnesses and modify the graph according to their judgment.

The generated outputs are not image files, but .dot files, which contain textual descriptions of the graphs. To render the images from these files, we must use the `dot` program from the graphviz library. As an example, if the graph description file for the local stemma of 3 John 1:4/22–26 is B25K1V4U22-26-local-stemma.dot, then the command

    dot -Tpng B25K1V4U22-26-local-stemma.dot -O
    
will generate a PNG image file called B25K1V4U22-26-local-stemma.dot.png. (If you want to specify your own output file name, use the `-o` argument followed by the file name you want.)

Sample images of local stemmata have already been included at the beginning of the "Usage" section. For the sake of completeness, we have included sample images of the other types of graphs below.

Complete textual flow diagram for 3 John 1:4/22–26:
![3 John 1:4/22–26 complete textual flow diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26-textual-flow.png)

Coherence in attestations diagrams for all readings in 3 John 1:4/22–26:
![3 John 1:4/22–26a coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26Ra-coherence-attestations.png)
![3 John 1:4/22–26af coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26Raf-coherence-attestations.png)
![3 John 1:4/22–26b coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26Rb-coherence-attestations.png)
![3 John 1:4/22–26c coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26Rc-coherence-attestations.png)
![3 John 1:4/22–26d coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26Rd-coherence-attestations.png)
![3 John 1:4/22–26d2 coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26Rd2-coherence-attestations.png)

Coherence in variant passages diagram for 3 John 1:4/22–26:
![3 John 1:4/22–26 coherence in variant passages diagram](https://github.com/jjmccollum/open-cbgm/blob/master/images/B25K1V4U22-26-coherence-variants.png)

Complete global stemma for 3 John (multiple roots are due to readings with unclear sources):
![3 John global stemma](https://github.com/jjmccollum/open-cbgm/blob/master/images/global-stemma.png)

Complete global stemma for 3 John, with fragmentary witnesses and variants with unclear reading sources excluded:
![3 John global stemma](https://github.com/jjmccollum/open-cbgm/blob/master/images/global-stemma-connected.png)

## Future Development

While we feel that we have achieved all of the basic objectives with the current release of the open-cbgm library, there is clearly room for improvement on a number of fronts. We will briefly outline what we consider to be the clearest and most promising avenues of future development.

1. User interface: While we have attempted to provide thorough documentation on the usage of the library's executable scripts, we realize that command-line interfaces are not particularly user-friendly, especially for users whose background is in the humanities and not in computer science. A graphical user interface (GUI) based on the library would be ideal. A relatively straightforward approach would be one like that of the INTF's Genealogical Queries tool: the library executes all necessary computations on a server, and it sends results to and receives requests from a client-side webpage, which uses standard JavaScript libraries like D3 (https://d3js.org/) to render the input forms and results in a user-friendy graphical format.
2. Pre-CBGM workflow: Presently, the user must add encodings of connectivity values and local stemmata to each variation unit in a TEI XML collation file manually. While this allows practitioners to customize the method fully, the process is not user-friendly. An interface for adding or modifying local stemmata and setting or updating connectivity values for variation units in a collation file would fit nicely between existing tools that collate TEI XML transcriptions (e.g., those at https://itsee-wce.birmingham.ac.uk/) and the CBGM software.
