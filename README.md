# open-cbgm
Fast, compact, open-source, TEI-compliant C++ implementation of the Coherence-Based Genealogical Method

[![Build Status](https://travis-ci.org/jjmccollum/open-cbgm.svg?branch=master)](https://travis-ci.org/jjmccollum/open-cbgm)
[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat)](https://choosealicense.com/licenses/mit/)

## About This Project

### Introduction

The Coherence-Based Genealogical Method (CBGM) is a novel approach to textual criticism, popularized by the Institut für Neutestamentliche Textforschung (INTF) for its use in the production of the _Editio Critica Maior_ (_ECM_) of the New Testament. It is a meta-method, combining methodology-dependent philological decisions from the user with efficient computer-based calculations to highlight genealogical relationships between different stages of the text. To establish genealogical relationships in the presence of contamination (understood to be a problem in the textual tradition of the New Testament), the CBGM makes a number of philosophical and methodological innovations, such as the abstracting of texts away from the manuscripts that preserve them (and the resulting rejection of hypothetical ancestors as used in traditional stemmata), the encoding of the textual critic's decisions in local stemmata of variants, and the use of coherence in textual flow to evaluate hypotheses about the priority of variant readings. 

To learn more about the CBGM, see Tommy Wasserman and Peter J. Gurry, _A New Approach to Textual Criticism: An Instroduction to the Coherence-Based Genealogical Method_, RBS 80 (Atlanta: SBL Press, 2017); Peter J. Gurry, _A Critical Examination of the Coherence-Based Genealogical Method in the New Testament_, NTTSD 55 (Leiden: Brill, 2017); and Gerd Mink, "Problems of a Highly Contaminated Tradition: The New Testament: Stemmata of Variants as a Source of Genealogy for Witnesses," in Pieter van Reenen, August den Hollander, and Margot van Mulken, eds., _Studies in Stemmatology II_ (Philadelphia, PA: Benjamins, 2004), 13–85.

### Design Philosophy

This is not the first software implementation of the CBGM. To our knowledge, the earliest software designed for this purpose is the Genealogical Queries tool developed by the INTF (http://intf.uni-muenster.de/cbgm2/GenQ.html). The underlying data is restricted to the Catholic Epistles, and both the collation data and the source code are inaccessible to the user. More recently, the INTF and the Cologne Center for eHumanities (CCeH) have made significant updates in a version used for Acts (https://ntg.cceh.uni-koeln.de/acts/ph4/). While this updated version is transparent with the collation data, the local stemmata are currently not editable, and the underlying code is not open-source. Most recently, Andrew Edmondson developed an open-source Python implementation of the CBGM (https://github.com/edmondac/CBGM, DOI 10.5281/zenodo.1296288), though his description implies that it is more intended for experimentation than for practical use: "It was designed for testing and changing the various algorithms and is not (therefore) the fastest user-facing package."

In light of the strengths and shortcomings of its predecessors, the open-cbgm library was developed with the following desired features in mind:
1. _open-source_: The code should be publicly available and free to be used and modified by anyone.
2. _compact_: The code should have a minimal memory footprint and minimal dependencies on external libraries. The input and output of the software should also be as concise as possible.
3. _fast_: The code should be optimized with performance and scalability in mind.
4. _compliant_: The input and output of the software should adhere as best as possible to established standards in the field, so as to facilitate its incorporation into established workflows.

To achieve the goal of feature (1), we have made our entire codebase available under the permissive MIT License. None of the functionality "under the hood" is obfuscated, and any user with access to the input file can customize local stemmata and connectivity parameters. (Incidentally, this fulfills a desideratum for customizability expressed by Wasserman and Gurry, _A New Approach to Textual Criticism_, 119–120.)

With respect to feature (2), our code makes use of only two external libraries: the pugixml lightweight XML-parsing library (https://github.com/zeux/pugixml) and the CRoaring compressed bitmap library (https://github.com/RoaringBitmap/CRoaring). The only input source for the software is an XML file encoding collation data; the size of the _ECM_ collation data for a single chapter of New Testament typically ranges between 100KB and 200KB. At this time, the software does not employ an external database for caching collation data, as it is not needed; nevertheless, we have endeavored to make the library modular enough to make it easy to incorporate such a feature. The outputs of the script are small .dot files (ranging from 4KB to 12KB each) containing rendering information for graphs.

For feature (3), we made performance-minded decisions at all levels of design. We chose the pugixml library to ensure that the software would parse potentially long XML input files quickly. Likewise, we used Roaring bitmaps from the CRoaring library to encode pregenealogical and genealogical relationships, both to leverage hardware accelerations for common computations involving those relationships and to ensure that performance would scale gracefully with the number of variation units covered in collation input files. We stored data keyed by witnesses in hash tables for efficient accesses on average. Due to combinatorial nature of the problem of substemma optimization, we implemented common heuristics known to be effective at solving similar types of problems in practice. As a result, the open-cbgm library can parse the _ECM_ collation of 3 John (consisting of 137 witnesses and 116 variation units), calculate the pre-genealogical and genealogical relationships between its witnesses, and generate a global stemma for the entire book in two-and-a-half minutes.

Finally, for feature (4), we ensured compliance with the XML standard of the Text Encoding Initiative (TEI) by designing the software to expect an input in TEI XML format used by the INTF and the International Greek New Testament Project (IGNTP) in their transcriptions and collations. We made this possible by encoding common features of the CBGM as additional TEI XML elements in the input collation file (e.g., the feature set <fs/> element for a variation unit's connectivity and the <graph/> element for its local stemma). For now, these elements must be added to a plain collation file in TEI XML format manually, but a more user-friendly interface for this task would not be too hard to develop, and it would fit nicely in the workflow between collation and applying the CBGM using this library. The .dot format used to describe output graphs is the standard used by the graphviz visualization software (https://www.graphviz.org/), which is the standard for existing CBGM software.

## Installation and Dependencies

As mentioned above, the open-cbgm library uses two external libraries (pugixml and CRoaring), but because these libraries are compact, we have included the necessary headers and scripts in the library for convenience. With respect to graph outputs, the open-cbgm library does not generate image files directly; instead, for the sake of flexibility, it generates textual graph description files in .dot format, which can subsequently be converted to image files in a variety of formats using graphviz. Platform-specific instructions for installing graphviz are provided below, and directions on how to get image files from the .dot outputs can be found in the "Usage" section. 

To begin setting up the library, you should download the Git repository. If you are operating from the command-line, you can do this with the command

    git clone git://github.com/jjmccollum/open-cbgm.git

Next, you need to build to project. How you do this will depend on your operating system. We will handle the cases below.

### Linux

To do this, you will need to have the autotools utilities for generating the configure.sh script. If you are using Linux or MacOS, then you may already have these tools installed. If not, the installation can be done in a single command. On Debian variants of Linux, the command is

    sudo apt-get install automake autoconf

Once you have autotools installed, you can complete the build with a few small lines. From the open-cbgm directory, enter the following commands:

    autoconf
    ./configure
    make
    
Once these commands have executed, you should have all of the executable scripts added to the open-cbgm directory.

If graphviz is not installed on your system, then you can install it via the command

    sudo apt-get install graphviz
    
### MacOS
    
The setup for MacOS is virtually identical to that of Linux. If you need to install autotools, it is recommended that you install the necessary packages with Homebrew (https://brew.sh/). If you are using Homebrew, simply enter the command

    brew install autoconf automake
    
Then, from the open-cbgm directory, enter the following commands:

    autoconf
    ./configure
    make
    
Once these commands have executed, you should have all of the executable scripts added to the open-cbgm directory.

If graphviz is not installed on your system, then you can install it via the command

    brew install graphviz

### Windows

The autotools utilities are not natively supported on Windows, so installation will take a few more steps. We recommend that you download Cygwin, a suite of GNU and open-source scripts that includes autotools. If you use the Chocolately package manager https://chocolatey.org/, then you can perform the installation from the command-line in a single command:

    choco install -y cygwin

This will install Cygwin in the C:\tools\cygwin directory. Alternatively, you can download Cygwin from the official website (https://www.cygwin.com/) and install it at the location of your choice. For what follows, we will assume that Cygwin is installed at C:\tools\cygwin.

**(TODO: Need to figure out the remaining steps.)**

Once these commands have executed, you should have all of the executable scripts added to the open-cbgm directory.

If graphviz is not installed on your system, then you can install it using Chocolately via the command

    choco install -y graphviz
    
Alternatively, you can download the library from https://www.graphviz.org/ and install it manually.

## Usage

When built, the open-cbgm library contains four executable scripts: compare\_witnesses, find\_relatives, optimize\_substemmata, and print\_graphs. The first two scripts correspond to modules with the same names in both versions of the INTF's Genealogical Queries tool. The third offers functionality that is offered only partially or not at all in the Genealogical Queries tool. The fourth generates graphs that can be displayed one variation unit at a time in the Genealogical Queries tool. We will provide usage examples and illustrations for each script in the subsections that follow.

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
