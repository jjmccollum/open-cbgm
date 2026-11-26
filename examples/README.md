# Full-book ECM CBGM examples

These TEI inputs contain all variation units flagged as variant in the respective CBGM snapshots from the Institute for New Testament Textual Research (INTF). Witness counts include the virtual witnesses A (_Ausgangstext_) and MT (Majority Text).

| TEI file | INTF snapshot | Variation units | Witnesses | Witness attestations | Reading-to-reading edges |
| --- | --- | ---: | ---: | ---: | ---: |
| [ecm_mark_ph35.xml](ecm_mark_ph35.xml) | Mark Phase 3.5 | 5,626 | 211 | 934,302 | 12,730 |
| [ecm_acts_ph4.xml](ecm_acts_ph4.xml) | Acts Phase 4 | 7,445 | 185 | 968,148 | 13,014 |

The sources are `mark_ph35.dump` and `acts_ph4.dump` from the INTF's [public CBGM database backups](https://github.com/SCDH/intf-cbgm/tree/80dfd03cf4f103d572ab0c58bde96ff49b690399/docker/backup). These are converted research inputs. Cite the corresponding INTF ECM edition and CBGM snapshot when using them.

The conversion joins each unambiguous, defined witness attestation to its editorial clique and retains the reading-to-reading edges in each local stemma. Uncertain or undefined attestations are omitted. The INTF's `*` and `?` source markers carry editorial information and are omitted from the TEI graph edges. Distinct cliques remain distinct TEI readings, so open-cbgm can distinguish cliques of a single reading label where the INTF agreement calculation groups them. The benchmark uses open-cbgm's default comparison model, unit edge weights, and unlimited connectivity.

| INTF source file | SHA-256 |
| --- | --- |
| `mark_ph35.dump` | `d84833021c42930846db8404e3484ab4cb52da508dda40ceee71b1b855f46dfa` |
| `acts_ph4.dump` | `2200362090c5e2002c762443f914b522ba8cbfcc2ab4e52725ee67fe9d3a4bf2` |

## Benchmark textual flow

From the repository root, build and run the opt-in benchmark. The worker count defaults to one. The same count applies to witness comparisons and textual-flow construction.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build --config Release --target textual_flow_benchmark --parallel
./test/benchmark_ecm.sh ./build/bin/textual_flow_benchmark 8
```

For each book, the benchmark reports elapsed wall time for XML parsing and apparatus initialization, witness comparisons (including potential ancestor ranking), and textual-flow graph construction. It reports the total through graph construction and times serialization separately. The script discards the serialized TSV. Run the executable directly to keep the graphs in collation order or to benchmark another file.

```sh
./build/bin/textual_flow_benchmark examples/ecm_mark_ph35.xml 8 > mark-flow.tsv
```

## Benchmark exact substemma search

On Linux, use a checkout that includes [PR #21](https://github.com/jjmccollum/open-cbgm/pull/21) and configure the build as above with `BUILD_TESTS=ON`.

```sh
cmake --build build --config Release --target substemma_benchmark --parallel
./build/bin/substemma_benchmark examples/ecm_mark_ph35.xml 8 4 > mark-substemmata.tsv
./build/bin/substemma_benchmark examples/ecm_acts_ph4.xml 8 4 > acts-substemmata.tsv
```

The final two arguments set the worker count and the timeout in seconds per witness.
Use `0` for an uncapped run. The benchmark calls `get_substemmata(0, true)` for
each witness and times the search separately from witness initialization.
It reports outcome counts and peak per-process resident memory on stderr and
writes per-witness results to stdout.
