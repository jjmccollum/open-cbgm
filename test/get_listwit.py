#!/usr/bin/env python3
 
import argparse
from lxml import etree as et

def get_listwit(input_addr):
    #Start parsing the file:
    tree = et.parse(input_addr)
    #Track the namespaces for later:
    root = tree.getroot()
    nsmap = root.nsmap
    tei_ns = nsmap[None]
    #Proceed for each reading:
    wit_ids = set()
    rdgs = root.xpath('//tei:rdg', namespaces={'tei': tei_ns})
    for rdg in rdgs:
        wit_ptrs = rdg.get('wit').split()
        for wit_ptr in wit_ptrs:
            wit_id = wit_ptr[1:]
            wit_ids.add(wit_id)
    wit_ids_list = list(wit_ids)
    wit_ids_list.sort()
    print(wit_ids_list)
    return
