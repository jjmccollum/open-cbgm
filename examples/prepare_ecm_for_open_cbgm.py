#!/usr/bin/env python3
"""Download and convert INTF Mark 3.5 and Acts 4 for open-cbgm.

Requires Python 3.11+ and pgdumplib==4.0.0:
    python -m pip install pgdumplib==4.0.0
    python prepare_ecm_for_open_cbgm.py

Outputs ecm-cbgm/mark.xml and ecm-cbgm/acts.xml plus JSON audit reports.
Pass --dataset mark or --dataset acts to prepare one book.

Each editorial clique remains a distinct reading node. Additional cliques
are tagged as split so open-cbgm can optionally treat them as equivalent.
Uncertain and undefined attestations are excluded from witness support.
Initial and unknown source markers are counted in the audit, and
reading-to-reading edges become arcs.
"""

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import shutil
from tempfile import NamedTemporaryFile
from urllib.request import urlopen
import xml.etree.ElementTree as ET

import pgdumplib


SOURCES = {'mark': {'name': 'Mark, INTF CBGM Phase 3.5',
          'kind': 'intf-pg-dump',
          'url': 'https://raw.githubusercontent.com/SCDH/intf-cbgm/80dfd03cf4f103d572ab0c58bde96ff49b690399/docker/backup/mark_ph35.dump',
          'sha256': 'd84833021c42930846db8404e3484ab4cb52da508dda40ceee71b1b855f46dfa',
          'witnesses': 211,
          'variation_units': 5626,
          'tei_sha256': '915f67948e83e3c8abbf5f406cd26fc545fd9d43c518b9855bb91db77ba0f3ab'},
 'acts': {'name': 'Acts, INTF CBGM Phase 4',
          'kind': 'intf-pg-dump',
          'url': 'https://raw.githubusercontent.com/SCDH/intf-cbgm/80dfd03cf4f103d572ab0c58bde96ff49b690399/docker/backup/acts_ph4.dump',
          'sha256': '2200362090c5e2002c762443f914b522ba8cbfcc2ab4e52725ee67fe9d3a4bf2',
          'witnesses': 185,
          'variation_units': 7445,
          'tei_sha256': '668cece2eaa050bef49a22e4ded3f1b956f97981d6da270588741567886f6ded'}}


def split_cliques(cliques, readings, passage):
    """Return all but the first clique of each editorial reading."""
    labels_by_text = defaultdict(set)
    groups = defaultdict(list)
    for reading in sorted(cliques):
        label, _ = reading
        groups[label].append(reading)
        # pugixml normalizes XML line endings and skips whitespace-only text.
        text = (readings[(passage, label)] or '').replace('\r\n', '\n').replace('\r', '\n')
        labels_by_text[text if text.strip(' \t\n') else ''].add(label)
    for labels in labels_by_text.values():
        if len(labels) > 1 and any(len(groups[label]) > 1 for label in labels):
            raise ValueError(f"Ambiguous split text at passage {passage}: {sorted(labels)}")
    return {reading for group in groups.values() for reading in group[1:]}


def convert(source, output):
    dump = pgdumplib.load(source)

    def table(name):
        return dump.table_data('ntg', name)

    passages = {}
    for row in table('passages'):
        pid, book, start, end, _, variant = row[:6]
        if variant == 't':
            passages[pid] = (int(book), int(start), int(end))

    witnesses = dict((row[0], row[2]) for row in table('manuscripts'))
    if len(set(witnesses.values())) != len(witnesses) or any(
        not siglum or any(c.isspace() for c in siglum) for siglum in witnesses.values()
    ):
        raise ValueError('Witness sigla must be unique, non-empty tokens')

    readings = {(row[0], row[1]): row[2] if len(row) > 2 else '' for row in table('readings')}
    nodes = defaultdict(set)
    declared_cliques = defaultdict(set)
    for pid, label, clique, *_ in table('cliques'):
        if pid in passages:
            declared_cliques[pid].add((label, clique))
            if not label.startswith('z'):
                nodes[pid].add((label, clique))

    edges = defaultdict(set)
    root_markers = Counter()
    for pid, label, clique, prior_label, prior_clique, *_ in table('locstem'):
        if pid not in passages:
            continue
        target = (label, clique)
        if target not in declared_cliques[pid]:
            raise ValueError(('Unknown local-stemma target', pid, target))
        nodes[pid].add(target)
        if prior_label in ('*', '?'):
            root_markers[prior_label] += 1
            continue
        prior = (prior_label, prior_clique)
        if prior not in declared_cliques[pid] or prior == target:
            raise ValueError(('Invalid local-stemma source', pid, prior, target))
        nodes[pid].add(prior)
        edges[pid].add((prior, target))

    # Like the INTF labez matrix, only unambiguous, defined CBGM
    # attestations count as extant. Preserve their editorial clique via the join.
    counts = Counter()
    uncertain = set()
    eligible = {}
    for row in table('apparatus'):
        manuscript, pid, label, cbgm, _, certainty = row[:6]
        if pid not in passages:
            continue
        key = (pid, manuscript)
        if certainty != '1':
            uncertain.add(key)
            counts['uncertain_apparatus_rows'] += 1
        elif cbgm == 't' and not label.startswith('z'):
            if key in eligible:
                raise ValueError(('Multiple eligible attestations', key))
            eligible[key] = label
        else:
            counts['excluded_certain_rows'] += 1
    for key in uncertain:
        eligible.pop(key, None)

    assignments = defaultdict(dict)
    for manuscript, pid, label, clique, *_ in table('ms_cliques'):
        if eligible.get((pid, manuscript)) != label:
            continue
        reading = (label, clique)
        if reading not in nodes[pid] or manuscript in assignments[pid]:
            raise ValueError(('Invalid attested clique', pid, manuscript, reading))
        assignments[pid][manuscript] = reading
    if sum(map(len, assignments.values())) != len(eligible):
        raise ValueError('Some eligible attestations have no clique')

    def reading_id(reading):
        return '-'.join(reading)

    root = ET.Element('TEI', xmlns='http://www.tei-c.org/ns/1.0')
    witness_list = ET.SubElement(
        ET.SubElement(ET.SubElement(ET.SubElement(root, 'teiHeader'), 'fileDesc'), 'sourceDesc'),
        'listWit',
    )
    ordered_witnesses = sorted(witnesses, key=int)
    for manuscript in ordered_witnesses:
        ET.SubElement(witness_list, 'witness', n=witnesses[manuscript])
    body = ET.SubElement(ET.SubElement(root, 'text'), 'body')
    ordered_passages = sorted(passages, key=lambda pid: (*passages[pid], int(pid)))
    splits = {pid: split_cliques(nodes[pid], readings, pid) for pid in ordered_passages}
    for pid in ordered_passages:
        app = ET.SubElement(body, 'app', n='p' + pid)
        note = ET.SubElement(app, 'note')
        book, start, end = passages[pid]
        ET.SubElement(note, 'label').text = f'{book}/{start}-{end} (INTF {pid})'
        graph = ET.SubElement(note, 'graph')
        supporters = defaultdict(list)
        for manuscript in ordered_witnesses:
            reading = assignments[pid].get(manuscript)
            if reading is not None:
                supporters[reading].append(witnesses[manuscript])
        for reading in sorted(nodes[pid]):
            rid = reading_id(reading)
            attrs = {'n': rid}
            if supporters[reading]:
                attrs['wit'] = ' '.join(supporters[reading])
            if reading in splits[pid]:
                attrs['type'] = 'split'
            ET.SubElement(app, 'rdg', attrs).text = readings[(pid, reading[0])]
            ET.SubElement(graph, 'node', n=rid)
        for prior, target in sorted(edges[pid]):
            ET.SubElement(graph, 'arc', {'from': reading_id(prior), 'to': reading_id(target)})

    ET.indent(root, space='  ')
    xml = ET.tostring(root, encoding='UTF-8', xml_declaration=True)
    # Match the empty-element formatting already used by the examples.
    output.write_bytes(xml.replace(b' />', b'/>') + b'\n')

    # Read the emitted XML back and compare its assignments and graph topology
    # to the selected source relations at every passage.
    ns = {'t': 'http://www.tei-c.org/ns/1.0'}
    reread = ET.parse(output)
    assert [el.get('n') for el in reread.findall('.//t:witness', ns)] == [
        witnesses[mid] for mid in ordered_witnesses
    ]
    apps = reread.findall('.//t:app', ns)
    assert [app.get('n') for app in apps] == ['p' + pid for pid in ordered_passages]
    for pid, app in zip(ordered_passages, apps):
        actual = {
            (siglum, rdg.get('n'))
            for rdg in app.findall('t:rdg', ns)
            for siglum in rdg.get('wit', '').split()
        }
        expected = {(witnesses[mid], reading_id(rdg)) for mid, rdg in assignments[pid].items()}
        assert actual == expected
        assert {rdg.get('n') for rdg in app.findall('t:rdg', ns) if rdg.get('type') == 'split'} == {
            reading_id(reading) for reading in splits[pid]
        }
        assert {e.get('n') for e in app.findall('t:note/t:graph/t:node', ns)} == {
            reading_id(rdg) for rdg in nodes[pid]
        }
        assert {
            (arc.get('from'), arc.get('to')) for arc in app.findall('t:note/t:graph/t:arc', ns)
        } == {(reading_id(a), reading_id(b)) for a, b in edges[pid]}

    return {
        'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
        'xml_sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
        'witnesses': len(witnesses),
        'passages': len(passages),
        'passages_with_graph_nodes': sum(bool(nodes[pid]) for pid in passages),
        'clique_nodes': sum(map(len, nodes.values())),
        'split_cliques': sum(map(len, splits.values())),
        'reading_to_reading_edges': sum(map(len, edges.values())),
        'source_markers': dict(root_markers),
        'eligible_attestations': len(eligible),
        'uncertain_witness_passages': len(uncertain),
        **counts,
    }



def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def verify_checksum(path, expected):
    actual = sha256(path)
    if actual != expected:
        raise ValueError(f"Checksum mismatch for {path}: expected {expected}, got {actual}")


def fetch(url, destination, expected):
    if destination.exists():
        verify_checksum(destination, expected)
        return
    with NamedTemporaryFile(dir=destination.parent, delete=False) as temp:
        candidate = Path(temp.name)
        try:
            with urlopen(url, timeout=60) as response:
                shutil.copyfileobj(response, temp)
            temp.close()
            verify_checksum(candidate, expected)
            candidate.replace(destination)
        finally:
            candidate.unlink(missing_ok=True)


def count_tei(path):
    root = ET.parse(path).getroot()
    return {
        "witnesses": len(root.findall("./{*}teiHeader/{*}fileDesc/{*}sourceDesc/{*}listWit/{*}witness")),
        "variation_units": len(root.findall(".//{*}app")),
        "units_with_local_graph": len(root.findall(".//{*}app/{*}note/{*}graph")),
    }


def prepare(key, source, output, local_source_dir):
    filename = source["url"].rsplit("/", 1)[-1]
    source_path = (local_source_dir or output / "sources") / filename
    if local_source_dir:
        verify_checksum(source_path, source["sha256"])
    else:
        fetch(source["url"], source_path, source["sha256"])

    xml_path = output / f"{key}.xml"
    with NamedTemporaryFile(dir=output, delete=False) as staged:
        candidate = Path(staged.name)
    try:
        audit = convert(source_path, candidate)
        verify_checksum(candidate, source["tei_sha256"])
        counts = count_tei(candidate)
        for field in ("witnesses", "variation_units"):
            if source[field] != counts[field]:
                raise ValueError(f"Unexpected {field} in {source['name']}: {counts[field]}")
        if counts["units_with_local_graph"] != counts["variation_units"]:
            raise ValueError(f"Missing local graphs in {source['name']}")
        candidate.replace(xml_path)
    finally:
        candidate.unlink(missing_ok=True)

    (output / f"{key}.json").write_text(
        json.dumps({"source": source, **audit, **counts}, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    print(f"{xml_path}: {counts['witnesses']} witnesses, {counts['variation_units']} units", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dataset", choices=[*SOURCES, "all"], default="all")
    parser.add_argument("--output", type=Path, default=Path("ecm-cbgm"))
    parser.add_argument("--local-source-dir", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    if not args.local_source_dir:
        (args.output / "sources").mkdir(exist_ok=True)
    books = SOURCES if args.dataset == "all" else (args.dataset,)
    for key in books:
        prepare(key, SOURCES[key], args.output, args.local_source_dir)


if __name__ == "__main__":
    main()
