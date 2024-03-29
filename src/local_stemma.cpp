/*
 * local_stemma.cpp
 *
 *  Created on: Oct 31, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <list>
#include <queue>
#include <set> //used instead of unordered_set because pair does not have a default hash function and readings are few enough for tree structures to be more efficient
#include <map> //used instead of unordered_map because readings are few enough for tree structures to be more efficient

#include "pugixml.hpp"
#include "local_stemma.h"

using namespace std;
using namespace pugi;

/**
 * Populates a given shortest paths map
 * by applying Dijkstra's algorithm to the given graph represented by an adjacency map.
 */
void populate_shortest_paths(const map<string, list<local_stemma_edge>> & adjacency_map, map<pair<string, string>, local_stemma_path> & shortest_paths) {
	//Proceed for each vertex:
	for (pair<string, list<local_stemma_edge>> kv : adjacency_map) {
		string s = kv.first;
		//Initialize a min priority queue of paths proceeding from this source:
		auto compare = [](local_stemma_path p1, local_stemma_path p2) {
			return p1.weight > p2.weight;
		};
		priority_queue<local_stemma_path, vector<local_stemma_path>, decltype(compare)> queue = priority_queue<local_stemma_path, vector<local_stemma_path>, decltype(compare)>(compare);
		local_stemma_path p0;
		p0.prior = s;
		p0.posterior = s;
		p0.weight = 0;
		p0.cardinality = 0;
		queue.push(p0);
		//Add a shortest path of length 0 from the current source to itself:
		shortest_paths[pair<string, string>(s, s)] = p0;
		//Then proceed by best-first search:
		while (!queue.empty()) {
			//Pop the minimum-length path from the queue:
			local_stemma_path p = queue.top();
			queue.pop();
			//Then proceed for each edge proceeding from the destination vertex:
			for (local_stemma_edge e : adjacency_map.at(p.posterior)) {
				local_stemma_path q;
				q.prior = s;
				q.posterior = e.posterior;
				q.weight = p.weight + e.weight;
				q.cardinality = e.weight > 0 ? p.cardinality + 1 : p.cardinality;
				//Check if a path from the source to the endpoint of this edge has already been processed:
				pair<string, string> endpoints = pair<string, string>(q.prior, q.posterior);
				if (shortest_paths.find(endpoints) == shortest_paths.end()) {
					//If not, then add this path to the shortest paths map:
					shortest_paths[endpoints] = q;
					//Then add it to the queue:
					queue.push(q);
				}
				else {
					//If so, then update the length and cardinality of the current entry in the shortest paths map if necessary:
					local_stemma_path shortest_path = shortest_paths.at(endpoints);
					if (q.weight < shortest_path.weight) {
						shortest_paths[endpoints] = q;
					}
				}
			}
		}
	}
	return;
}

/**
 * Default constructor.
 */
local_stemma::local_stemma() {

}

/**
 * Constructs a local stemma from a <graph/> XML element using the parent variation_unit's ID and label.
 * A set of split reading pairs may be specified, between which new edges with lengths of 0 will be added.
 * A set of trivial readings may also be specified, whose in-edges will be assigned weights of 0.
 * A set of dropped readings may also be specified, whose vertices and edges will not be added.
 */
local_stemma::local_stemma(const xml_node & xml, const string & vu_id, const string & vu_label, const set<pair<string, string>> & split_pairs, const set<string> & trivial_readings, const set<string> & dropped_readings) {
	//Populate the vertex and edge lists:
	vertices = list<local_stemma_vertex>();
	edges = list<local_stemma_edge>();
	//At the same time, populate a set of distinct root node IDs:
	set<string> distinct_roots = set<string>();
	//Set the ID:
	id = vu_id;
	//Set the label:
	label = vu_label;
	//Add a vertex for each <node/> element:
	for (xml_node node : xml.children("node")) {
		//If the node lacks an "n" attribute, then do not add it:
		if (!node.attribute("n")) {
			continue;
		}
		string node_id = node.attribute("n").value();
		//If the node represents a dropped reading, then don't add it:
		if (dropped_readings.find(node_id) != dropped_readings.end()) {
			continue;
		}
		local_stemma_vertex v;
		v.id = node_id;
		vertices.push_back(v);
		distinct_roots.insert(node_id);
	}
	//Add an edge for each <arc/> element:
	for (xml_node arc : xml.children("arc")) {
		local_stemma_edge e;
		e.prior = arc.attribute("from").value();
		e.posterior = arc.attribute("to").value();
		//Don't add any self-loops (self-loops will be introduced only for shortest path calculation):
		if (e.prior == e.posterior) {
			continue;
		}
		//Don't add edges with dropped endpoints:
		if (dropped_readings.find(e.prior) != dropped_readings.end() || dropped_readings.find(e.posterior) != dropped_readings.end()) {
			continue;
		}
		//If the destination vertex is in the trivial set, then set the weight to 0:
		if (trivial_readings.find(e.posterior) != trivial_readings.end()) {
			e.weight = 0;
		}
		//Otherwise, use the value provided in the <label/> element under the <arc/> element, if there is one:
		else if (arc.child("label") && arc.child("label").text()) {
			e.weight = arc.child("label").text().as_float();
		}
		//Otherwise, use the default value of 1:
		else {
			e.weight = 1;
		}
		//Remove the destination node from the set of potential root nodes, and add the edge to the graph:
		distinct_roots.erase(e.posterior);
		edges.push_back(e);
	}
	//Add edges in both directions for all specified split pairs:
	for (pair<string, string> split_pair : split_pairs) {
		local_stemma_edge e1;
		e1.prior = split_pair.first;
		e1.posterior = split_pair.second;
		e1.weight = 0;
		local_stemma_edge e2;
		e2.prior = split_pair.second;
		e2.posterior = split_pair.first;
		e2.weight = 0;
		edges.push_back(e1);
		edges.push_back(e2);
	}
	//Now construct an ordered list of root vertex IDs from the set of distinct root IDs left over:
	roots = list<string>();
	for (local_stemma_vertex v: vertices) {
		if (distinct_roots.find(v.id) != distinct_roots.end()) {
			roots.push_back(v.id);
		}
	} 
	//Construct an adjacency map from the graph:
	map<string, list<local_stemma_edge>> adjacency_map = map<string, list<local_stemma_edge>>();
	for (local_stemma_vertex v: vertices) {
		adjacency_map[v.id] = list<local_stemma_edge>();
	}
	for (local_stemma_edge e : edges) {
		adjacency_map[e.prior].push_back(e);
	}
	//Now use Dijkstra's algorithm to populate the map of shortest paths:
	paths = map<pair<string, string>, local_stemma_path>();
	populate_shortest_paths(adjacency_map, paths);
}

/**
 * Constructs a local stemma from a variation unit ID, label, and lists of vertices and edges populated using the genealogical cache.
 */
local_stemma::local_stemma(const string & _id, const string & _label, const list<local_stemma_vertex> & _vertices, const list<local_stemma_edge> & _edges) {
	id = _id;
	label = _label;
	vertices = _vertices;
	edges = _edges;
	//Populate a set of distinct root node IDs:
	set<string> distinct_roots = set<string>();
	for (local_stemma_vertex v : vertices) {
		distinct_roots.insert(v.id);
	}
	for (local_stemma_edge e : edges) {
		distinct_roots.erase(e.posterior);
	}
	//Now construct an ordered list of root vertex IDs from the set of distinct root IDs left over:
	roots = list<string>();
	for (local_stemma_vertex v: vertices) {
		if (distinct_roots.find(v.id) != distinct_roots.end()) {
			roots.push_back(v.id);
		}
	}
	//Construct an adjacency map from the graph:
	map<string, list<local_stemma_edge>> adjacency_map = map<string, list<local_stemma_edge>>();
	for (local_stemma_vertex v: vertices) {
		adjacency_map[v.id] = list<local_stemma_edge>();
	}
	for (local_stemma_edge e : edges) {
		adjacency_map[e.prior].push_back(e);
	}
	//Now use Dijkstra's algorithm to populate the map of shortest paths:
	paths = map<pair<string, string>, local_stemma_path>();
	populate_shortest_paths(adjacency_map, paths);
}

/**
 * Default destructor.
 */
local_stemma::~local_stemma() {

}

/**
 * Returns the ID for this local_stemma.
 */
string local_stemma::get_id() const {
	return id;
}

/**
 * Returns the label for this local_stemma.
 */
string local_stemma::get_label() const {
	return label;
}

/**
 * Returns the list of vertices for this local_stemma.
 */
list<local_stemma_vertex> local_stemma::get_vertices() const {
	return vertices;
}

/**
 * Returns the list of edges for this local_stemma.
 */
list<local_stemma_edge> local_stemma::get_edges() const {
	return edges;
}

/**
 * Return the list of root nodes for this local_stemma.
 */
list<string> local_stemma::get_roots() const {
	return roots;
}

/**
 * Return the map of shortest paths for this local_stemma.
 */
map<pair<string, string>, local_stemma_path> local_stemma::get_paths() const {
	return paths;
}

/**
 * Given two reading IDs, checks if a path exists between them in the local stemma.
 */
bool local_stemma::path_exists(const string & r1, const string & r2) const {
	return paths.find(pair<string, string>(r1, r2)) != paths.end();
}

/**
 * Given two reading IDs, returns the shortest path between them in the local stemma.
 * It is assumed that a path exists between the two readings.
 */
local_stemma_path local_stemma::get_path(const string & r1, const string & r2) const {
	return paths.at(pair<string, string>(r1, r2));
}

/**
 * Given two reading IDs, checks if they have a common ancestor reading.
 */
bool local_stemma::common_ancestor_exists(const string & r1, const string & r2) const {
	if (path_exists(r1, r2) || path_exists(r2, r1)) {
		return true;
	}
	for (string root : roots) {
		if (path_exists(root, r1) && path_exists(root, r2)) {
			return true;
		}
	}
	return false;
}

/**
 * Given two reading IDs, checks if they agree after trivial edges are collapsed.
 * Most obviously, if the reading IDs are equal, then they agree.
 * But they also count as agreeing if there is a path of weight 0 from one to the other
 * or if they have a common ancestor with a path of weight 0 to each of them.
*/
bool local_stemma::readings_agree(const string & r1, const string & r2) const {
	if (r1 == r2) {
		return true;
	}
	if ((path_exists(r1, r2) && get_path(r1, r2).weight == 0) || (path_exists(r2, r1) && get_path(r2, r1).weight == 0)) {
		return true;
	}
	for (local_stemma_vertex vertex : vertices) {
		if ((path_exists(vertex.id, r1) && get_path(vertex.id, r1).weight == 0) && (path_exists(vertex.id, r2) && get_path(vertex.id, r2).weight == 0)) {
			return true;
		}
	}
	return false;
}

/**
 * Given an output stream, writes the local stemma graph to output in .dot format.
 * An optional flag indicating whether to print edge weights can also be specified.
 */
void local_stemma::to_dot(ostream & out, bool print_weights) {
	//Add the graph first:
	out << "digraph local_stemma {\n";
	//Add a subgraph for the legend:
	out << "\tsubgraph cluster_legend {\n";
	//Add a box node indicating the label of this graph:
	out << "\t\tlabel [shape=plaintext, label=\"" << label << "\"];\n";
	out << "\t}\n";
	//Add a subgraph for the plot:
	out << "\tsubgraph cluster_plot {\n";
	//Make its border invisible:
	out << "\t\tstyle=invis;\n";
	//Add a line indicating that nodes do not have any shape:
	out << "\t\tnode [shape=plaintext];\n";
	//Add all of its nodes, assigning them numerical indices:
	map<string, int> id_to_index = map<string, int>();
	for (local_stemma_vertex v : vertices) {
		//Add the vertex's ID to the map:
		unsigned int i = (unsigned int) id_to_index.size();
		id_to_index[v.id] = i;
		out << "\t\t";
		out << id_to_index.at(v.id);
		//Use the ID as the node label:
		out << " [label=\"" << v.id << "\"];";
		out << "\n";
	}
	//Add all of its edges:
	for (local_stemma_edge e : edges) {
		string prior = e.prior;
		string posterior = e.posterior;
		float weight = e.weight;
		//Format the line style based on the edge weight:
		string edge_style = "";
		if (weight > 0) {
			edge_style = "style=solid";
		}
		else {
			edge_style = "style=dashed";
		}
		out << "\t\t";
		if (print_weights) {
			out << id_to_index.at(prior) << " -> " << id_to_index.at(posterior) << "[" << edge_style << ", label=\"" << fixed << std::setprecision(3) << weight << "\", fontsize=10];";
		}
		else {
			out << id_to_index.at(prior) << " -> " << id_to_index.at(posterior) << "[" << edge_style << "];";
		}
		out << "\n";
	}
	out << "\t}\n";
	out << "}" << endl;
	return;
}

/**
 * Given an output stream, writes the local stemma graph to output in JavaScript Object Notation (JSON) format.
 */
void local_stemma::to_json(ostream & out) {
    //Open the root object:
    out << "{";
    //Add the metadata fields:
	out << "\"label\":" << "\"" << label << "\"" << ",";
    //Open the vertices array:
    out << "\"vertices\":" << "[";
    //Print each vertex as an object:
	unsigned int vertex_num = 0;
	for (local_stemma_vertex v : vertices) {
        //Open the vertex object:
        out << "{";
        //Add its key-value pairs:
        out << "\"id\":" << "\"" << v.id << "\"";
        //Close the vertex object:
        out << "}";
        //Add a comma if this is not the last vertex:
        if (vertex_num != vertices.size() - 1) {
            out << ",";
        }
		vertex_num++;
	}
	//Close the vertices array:
    out << "]" << ",";
	//Open the edges array:
    out << "\"edges\":" << "[";
    //Print each edge as an object:
	unsigned int edge_num = 0;
	for (local_stemma_edge e : edges) {
        //Open the edge object:
        out << "{";
        //Add its key-value pairs:
        out << "\"prior\":" << "\"" << e.prior << "\"" << ",";
        out << "\"posterior\":" << "\"" << e.posterior << "\"" << ",";
        out << "\"weight\":" << e.weight;
        //Close the edge object:
        out << "}";
        //Add a comma if this is not the last edge:
        if (edge_num != edges.size() - 1) {
            out << ",";
        }
		edge_num++;
	}
	//Close the edges array:
    out << "]";
    //Close the root object:
    out << "}";
	return;
}
