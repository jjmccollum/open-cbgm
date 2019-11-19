/*
 * apparatus_test.cpp
 *
 *  Created on: Nov 8, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>

#include "pugixml.h"
#include "apparatus.h"
#include "witness.h"

using namespace std;

string TEST_XML = "<?xml version='1.0'?> \
<cx:apparatus xmlns:cx=\"http://interedition.eu/collatex/ns/1.0\" xmlns=\"http://www.tei-c.org/ns/1.0\"> \
	<TEI> \
		<teiHeader> \
			<sourceDesc> \
				<listWit> \
					<witness id=\"RP\"/> \
					<witness id=\"274\"/> \
				</listWit> \
			</sourceDesc> \
		</teiHeader> \
		<text lang=\"GRC\"> \
			<body> \
				<div type=\"book\" n=\"B01\"> \
					<div type=\"incipit\" n=\"B01incipit\"> \
						<ab wit=\"#RP #274\"><app><rdg n=\"a\" wit=\"#274\">ευαγγελιον</rdg><rdg n=\"b\" wit=\"#RP\"/><fs><f name=\"connectivity\"><numeric value=\"10\"/></f></fs><graph><label>Variation Unit 1</label><node n=\"a\"/><node n=\"b\"/><arc from=\"b\" to=\"a\"/></graph></app>κατα ματθαιον</ab> \
					</div> \
					<div type=\"chapter\" n=\"B01K1\"> \
						<ab n=\"B01K1V1\" wit=\"#RP #274\">βιβλος γενεσεως ιησου χριστου υιου δαυιδ υιου αβρααμ</ab> \
						<ab n=\"B01K1V2\" wit=\"#RP #274\">αβρααμ εγεννησεν τον ισαακ ισαακ δε εγεννησεν τον ιακωβ ιακωβ δε εγεννησεν τον ιουδαν και τους αδελφους αυτου</ab> \
						<ab n=\"B01K1V3\" wit=\"#RP #274\">ιουδας δε εγεννησεν τον φαρες και τον ζαρα εκ της θαμαρ φαρες δε εγεννησεν τον εσρωμ εσρωμ δε εγεννησεν τον αραμ</ab> \
						<ab n=\"B01K1V4\" wit=\"#RP #274\">αραμ δε εγεννησεν τον<app><rdg n=\"a\" wit=\"#274\" cause=\"paleographic_confusion\">αμιναδαμ αμιναδαμ</rdg><rdg n=\"b\" wit=\"#RP\">αμιναδαβ αμιναδαβ</rdg><fs><f name=\"connectivity\"><numeric value=\"10\"/></f></fs><graph><label>Variation Unit 2</label><node n=\"a\"/><node n=\"b\"/><arc from=\"b\" to=\"a\"/></graph></app>δε εγεννησεν τον ναασσων ναασσων δε εγεννησεν τον σαλμων</ab> \
					</div> \
				</div> \
			</body> \
		</text> \
	</TEI> \
</cx:apparatus>";

void test_witness() {
	cout << "Running test_witness..." << endl;
	//Initialize an XML string:
	string xml = TEST_XML;
	//Parse it:
	pugi::xml_document doc;
	doc.load_string(xml.c_str());
	apparatus app = apparatus(doc.child("cx:apparatus"));
	//Now initialize the witnesses using this apparatus:
	witness rp = witness("RP", app);
	cout << "id: " << rp.get_id() << endl;
	cout << "agreements with 274: " << rp.get_agreements_for_witness("274").toString() << endl;
	cout << "explained readings from 274: " << rp.get_explained_readings_for_witness("274").toString() << endl;
	witness ga_274 = witness("274", app);
	cout << "id: " << ga_274.get_id() << endl;
	cout << "agreements with RP: " << ga_274.get_agreements_for_witness("RP").toString() << endl;
	cout << "explained readings from RP: " << ga_274.get_explained_readings_for_witness("RP").toString() << endl;
	cout << "Done." << endl;
	return;
}

int main() {
	/**
	 * Entry point to the script. Calls all test methods.
	 */
	test_witness();
}
