#!/usr/bin/env python3
 
from lxml import etree as et

"""
XML namespaces
"""
XML_NS = 'http://www.w3.org/XML/1998/namespace'
TEI_NS = 'http://www.tei-c.org/ns/1.0'

"""
Byzantine witness set
"""
BYZ_WITS = {'018', '020', '025', '049', '0142', '1', '18', '35', '43', '180', '206S', '252', '319', '330', '365', '398', '400', '424', '429', '468', '522', '607', '617', '996', '1175', '1448', '1490', '1501', '1609', '1661', '1729', '1799', '1827', '1831', '1874', '1890', '2423', '2718', 'L596', 'L921', 'L938', 'L1141', 'L1281'}

def fill_majority_witnesses(input_addr, output_addr):
    #Start parsing the input file:
    input_tree = et.parse(input_addr)
    input_root = input_tree.getroot()
    #Populate a List of witness numbers and a Dictionary mapping witness IDs to their positions in the List:
    wit_ids = []
    wit_id_to_ind = {}
    for witness in input_root.xpath('//tei:witness', namespaces={'tei': TEI_NS}):
        wit_id = witness.get('n')
        wit_ids.append(wit_id)
        wit_id_to_ind[wit_id] = len(wit_id_to_ind)
    #Get the book number of the books contained in the input XML file:
    input_book = input_root.xpath('//tei:div[@type=\'book\']', namespaces={'tei': TEI_NS})[0]
    book_n = input_book.get('n')
    #Get the chapter number of the chapter contained in the input XML file:
    input_chapter = input_root.xpath('//tei:div[@type=\'chapter\']', namespaces={'tei': TEI_NS})[0]
    chapter_n = input_chapter.get('n')
    #Initialize the output <TEI> XML element:
    tei = et.Element('{%s}TEI' % TEI_NS, nsmap={None: TEI_NS, 'xml': 'http://www.w3.org/XML/1998/namespace'})
    #Under this, add a <teiHeader> element under the TEI element:
    teiHeader = et.Element('{%s}teiHeader' % TEI_NS)
    tei.append(teiHeader)
    #Under this, add a <sourceDesc> element:
    sourceDesc = et.Element('{%s}sourceDesc' % TEI_NS)
    teiHeader.append(sourceDesc)
    #Under this, add a <listWit> element:
    listWit = et.Element('{%s}listWit' % TEI_NS)
    sourceDesc.append(listWit)
    #Under this, add a <witness> element for each witness:
    for wit_id in wit_ids:
        wit = et.Element('{%s}witness' % TEI_NS)
        wit.set('n', wit_id)
        listWit.append(wit)
    #Then, add a <text> element under the TEI element:
    text = et.Element('{%s}text' % TEI_NS)
    text.set('{%s}lang' % XML_NS, 'GRC')
    tei.append(text)
    #Under this, add a <body> element:
    body = et.Element('body')
    text.append(body)
    #Under this, add a <div> element representing the book:
    book_div = et.Element('div')
    book_div.set('type', 'book')
    book_div.set('n', book_n)
    body.append(book_div)
    #Under this, add a <div> element representing the chapter:
    chapter_div = et.Element('div')
    chapter_div.set('type', 'chapter')
    chapter_div.set('n', chapter_n)
    book_div.append(chapter_div)
    #Proceed for each variation unit:
    input_apps = input_root.xpath("//tei:app", namespaces={'tei': TEI_NS})
    for input_app in input_apps:
        #Get the label, connectivity feature, and local stema graph for this variation unit:
        label = input_app.xpath('tei:label', namespaces={'tei': TEI_NS})[0]
        fs = input_app.xpath('tei:fs', namespaces={'tei': TEI_NS})[0]
        graph = input_app.xpath('tei:graph', namespaces={'tei': TEI_NS})[0]
        #Determine which witnesses are already included in the positive part of the apparatus:
        positive_app_ids = set()
        rdgs = input_app.xpath('tei:rdg', namespaces={'tei': TEI_NS})
        negative_app_rdg_ind = -1 #index of the reading that uses a negative apparatus
        rdg_ind = 0
        for rdg in rdgs:
            rdg_wit_ids = rdg.get('wit').split()
            for rdg_wit_id in rdg_wit_ids:
                positive_app_ids.add(rdg_wit_id)
                if rdg_wit_id in ['...', 'Byz']:
                    negative_app_rdg_ind = rdg_ind
            rdg_ind += 1
        #Then populate a List of witnesses to be added: 
        negative_app_rdg_wit_ids = []
        if '...' in positive_app_ids:
            #Include all witnesses not found in the positive part of the apparatus:
            positive_app_ids.remove('...')
            negative_app_ids = set(wit_ids).difference(positive_app_ids)
            negative_app_rdg = rdgs[negative_app_rdg_ind]
            negative_app_rdg_wit_ids = negative_app_rdg.get('wit').replace('...', '').split() + list(negative_app_ids)
            negative_app_rdg_wit_ids.sort(key=lambda wit_id: wit_id_to_ind[wit_id])
        elif 'Byz' in positive_app_ids:
            #Include all Byzantine witnesses not found in the positive part of the apparatus:
            positive_app_ids.remove('Byz')
            negative_app_ids = BYZ_WITS.difference(BYZ_WITS.intersection(positive_app_ids))
            negative_app_rdg = rdgs[negative_app_rdg_ind]
            negative_app_rdg_wit_ids = negative_app_rdg.get('wit').split() + list(negative_app_ids) #keep the Byz siglum, as it's technically a witness
            negative_app_rdg_wit_ids.sort(key=lambda wit_id: wit_id_to_ind[wit_id])
        else:
            #Otherwise, do nothing, the apparatus is positive in this case and should not be missing anything:
            pass
        #Now populate the output <app> element:
        app = et.Element('{%s}app' % TEI_NS)
        #Add the label:
        app.append(label)
        #Add the readings, now with positive apparatus data:
        rdg_ind = 0
        for rdg in rdgs:
            if rdg_ind == negative_app_rdg_ind:
                #For the reading that previously had a negative apparatus, add a reading with a positive apparatus:
                positive_app_rdg = et.Element('{%s}rdg' % TEI_NS)
                positive_app_rdg.set('n', rdg.get('n'))
                if rdg.get('type') is not None:
                    positive_app_rdg.set('type', rdg.get('type'))
                positive_app_rdg.set('wit', ' '.join(negative_app_rdg_wit_ids))
                if rdg.text is not None:
                    positive_app_rdg.text = rdg.text
                app.append(positive_app_rdg)
            else:
                #Add all unmodified readings as-is
                app.append(rdg)
            rdg_ind += 1
        #Add the connectivity feature set and local stemma graph:
        app.append(fs)
        app.append(graph)
        #Add this variation unit to the book div:
        book_div.append(app)
    et.ElementTree(tei).write(output_addr, encoding='utf-8', xml_declaration=True, pretty_print=True)
    return
