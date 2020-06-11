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

#include "pugixml.h"
#include "local_stemma.h"

using namespace std;

/**
 * Populates a given shortest paths map
 * by applying Dijkstra's algorithm to the given graph represented by an adjacency map.
 */
void populate_shortest_paths(const map<string, list<local_stemma_edge>> & adjacency_map, map<pair<string, string>, float> & shortest_paths) {
	//Proceed for each vertex:
	for (pair<string, list<local_stemma_edge>> kv : adjacency_map) {
		string s = kv.first;
		//Initialize a min priority queue of paths proceeding from this source (which we will represent using local_stemma_edge data structures):
		auto compare = [](local_stemma_edge e1, local_stemma_edge e2) {
			return e1.weight > e2.weight;
		};
		priority_queue<local_stemma_edge, vector<local_stemma_edge>, decltype(compare)> queue = priority_queue<local_stemma_edge, vector<local_stemma_edge>, decltype(compare)>(compare);
		local_stemma_edge p0;
		p0.prior = s;
		p0.posterior = s;
		p0.weight = 0;
		queue.push(p0);
		//Add a shortest path of length 0 from the current source to itself:
		shortest_paths[pair<string, string>(s, s)] = 0;
		//Then proceed by best-first search:
		while (!queue.empty()) {
			//Pop the minimum-length path from the queue:
			local_stemma_edge p = queue.top();
			queue.pop();
			//Get the destination vertex of this edge and the length of the path to it:
			string u = p.posterior;
			float p_length = p.weight;
			//Then proceed for each edge proceeding from the destination vertex:
			for (local_stemma_edge e : adjacency_map.at(u)) {
				local_stemma_edge q;
				q.prior = s;
				q.posterior = e.posterior;
				q.weight = p_length + e.weight;
				//Check if the path from the source to the endpoint of this edge has already been processed:
				pair<string, string> endpoints = pair<string, string>(q.prior, q.posterior);
				if (shortest_paths.find(endpoints) == shortest_paths.end()) {
					//If it hasn't, then add its length to the shortest paths map:
					shortest_paths[endpoints] = q.weight;
					//Then add the path itself to the queue:
					queue.push(q);
				}
				else {
					//If it has, then update its length in the shortest paths map if necessary:
					shortest_paths[endpoints] = min(shortest_paths.at(endpoints), q.weight);
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
local_stemma::local_stemma(const pugi::xml_node & xml, const string & vu_id, const string & vu_label, const set<pair<string, string>> & split_pairs, const set<string> & trivial_readings, const set<string> & dropped_readings) {
	graph.vertices = list<local_stemma_vertex>();
	graph.edges = list<local_stemma_edge>();
	//Set the ID:
	id = vu_id;
	//Set the label:
	label = vu_label;
	//Add a vertex for each <node/> element:
	for (pugi::xml_node node : xml.children("node")) {
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
		graph.vertices.push_back(v);
	}
	//Add an edge for each <arc/> element:
	for (pugi::xml_node arc : xml.children("arc")) {
		local_stemma_edge e;
		e.prior = arc.attribute("from").value();
		e.posterior = arc.attribute("to").value();
		//Don't add any self-loops (these will be handled automatically):
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
		graph.edges.push_back(e);
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
		graph.edges.push_back(e1);
		graph.edges.push_back(e2);
	}
	//Construct an adjacency map from the graph:
	map<string, list<local_stemma_edge>> adjacency_map = map<string, list<local_stemma_edge>>();
	for (local_stemma_vertex v: graph.vertices) {
		adjacency_map[v.id] = list<local_stemma_edge>();
	}
	for (local_stemma_edge e : graph.edges) {
		adjacency_map[e.prior].push_back(e);
	}
	//Now use Dijkstra's algorithm to populate the map of shortest paths:
	shortest_paths = map<pair<string, string>, float>();
	populate_shortest_paths(adjacency_map, shortest_paths);
}

/**
 * Constructs a local stemma from a variation unit ID, label, and graph data structure populated using the genealogical cache.
 */
local_stemma::local_stemma(const string & _id, const string & _label, const local_stemma_graph & _graph) {
	id = _id;
	label = _label;
	graph = _graph;
	//Construct an adjacency map from the graph:
	map<string, list<local_stemma_edge>> adjacency_map = map<string, list<local_stemma_edge>>();
	for (local_stemma_vertex v: graph.vertices) {
		adjacency_map[v.id] = list<local_stemma_edge>();
	}
	for (local_stemma_edge e : graph.edges) {
		adjacency_map[e.prior].push_back(e);
	}
	//Now use Dijkstra's algorithm to populate the map of shortest paths:
	shortest_paths = map<pair<string, string>, float>();
	populate_shortest_paths(adjacency_map, shortest_paths);
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
 * Returns the graph structure for this local_stemma.
 */
local_stemma_graph local_stemma::get_graph() const {
	return graph;
}

/**
 * Return the map of shortest paths for this local_stemma's graph.
 */
map<pair<string, string>, float> local_stemma::get_shortest_paths() const {
	return shortest_paths;
}

/**
 * Given two reading IDs, checks if a path exists between them in the local stemma.
 */
bool local_stemma::path_exists(const string & r1, const string & r2) const {
	return shortest_paths.find(pair<string, string>(r1, r2)) != shortest_paths.end();
}

/**
 * Given two reading IDs, returns the length of the shortest path between them in the local stemma.
 * It is assumed that a path exists between the two readings.
 */
float local_stemma::get_shortest_path_length(const string & r1, const string & r2) const {
	return shortest_paths.at(pair<string, string>(r1, r2));
}

/**
 * Given an output stream, writes the local stemma graph to output in .dot format.
 * An optional flag indicating whether to print edge weights can also be specified.
 */
void local_stemma::to_dot(ostream & out, bool print_weights=false) {
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
	for (local_stemma_vertex v : graph.vertices) {
		//Add the vertex's ID to the map:
		unsigned int i = id_to_index.size();
		id_to_index[v.id] = i;
		out << "\t\t";
		out << id_to_index.at(v.id);
		//Use the ID as the node label:
		out << " [label=\"" << v.id << "\"];";
		out << "\n";
	}
	//Add all of its edges:
	for (local_stemma_edge e : graph.edges) {
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
