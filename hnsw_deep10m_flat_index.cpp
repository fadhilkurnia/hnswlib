#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <map>


// hnsw index file contents:
// - hnsw parametres
// - data: label, neighbors for level=0 (first layer), embeddings
// - neighbors for upper layers

template<typename T>
void readBinaryPOD(std::istream &in,T &podRef) {
	in.read((char *) &podRef, sizeof(T));
}

int main() {
	char *path_index = "deep_10m_ef_40_M_16.bin";
	char *path_data = "hnsw_data_deep10m.dat";
	char *path_edges = "hnsw_edge_deep10m.dat";
	char *path_nodes = "hnsw_node_deep10m.dat";
  	char *path_stats = "hnsw_stat_deep10m.dat";
	
	// open index file
	std::ifstream input(path_index, std::ios::binary);
	if (!input.is_open())
		throw std::runtime_error("Cannot open file");

	// prepare output files
	std::ofstream output_nodes;
	std::ofstream output_edges;
  	std::ofstream output_stats;
	output_nodes.open(path_nodes);
	output_edges.open(path_edges);
  	output_stats.open(path_stats);
	
	// read all hnsw parameters
	size_t offsetLevel0_, max_elements_, cur_element_count, size_data_per_element_;
	size_t label_offset_, offsetData_, maxM_, maxM0_, M_, ef_construction_;
	int maxlevel_; unsigned int enterpoint_node_, size_links_level0_; double mult_;
	int dimmension = 96;
	readBinaryPOD(input, offsetLevel0_);
	readBinaryPOD(input, max_elements_);
	readBinaryPOD(input, cur_element_count);
	readBinaryPOD(input, size_data_per_element_);
	readBinaryPOD(input, label_offset_);
	readBinaryPOD(input, offsetData_);
	readBinaryPOD(input, maxlevel_);
	readBinaryPOD(input, enterpoint_node_);
	readBinaryPOD(input, maxM_);
	readBinaryPOD(input, maxM0_);
	readBinaryPOD(input, M_);
	readBinaryPOD(input, mult_);
	readBinaryPOD(input, ef_construction_);

	std::cout << "offsetLevel0_: " << offsetLevel0_ << "\n";
	std::cout << "max_elements_: " << max_elements_ << "\n";
	std::cout << "cur_element_count: " << cur_element_count << "\n";
	std::cout << "size_data_per_element_: " << size_data_per_element_ << "\n";
	std::cout << "label_offset_: " << label_offset_ << "\n";
	std::cout << "offsetData_: " << offsetData_ << "\n";
	std::cout << "maxlevel_: " << maxlevel_ << "\n";
	std::cout << "enterpoint_node_: " << enterpoint_node_ << "\n";
	std::cout << "maxM_: " << maxM_ << "\n";
	std::cout << "maxM0_: " << maxM0_ << "\n";
	std::cout << "M_: " << M_ << "\n";
	std::cout << "mult_: " << mult_ << "\n";
	std::cout << "ef_construction_: " << ef_construction_ << "\n";
	
	std::vector<int> num_node_level(maxlevel_+1);
	char* elements_data = (char *) malloc(cur_element_count * size_data_per_element_);
	if (elements_data == NULL) {
		std:: cout << "failed to allocate memory for data\n";
		return 1;
	}
	input.read(elements_data, cur_element_count * size_data_per_element_);

	// read the data
	// format: links_len .. links_lv0[] ... data[] ... label
	unsigned int global_id = 0;

	// get global_id by pair of (level_id and internal_id)
	std::map<std::pair<int, unsigned int>, unsigned int> global_id_map;

#pragma omp parallel for
	for(unsigned int i=0; i < cur_element_count; i++) {
		char* element_data = (elements_data + i * size_data_per_element_);

#pragma omp critical		
		{
			size_t label; 
			memcpy(&label,(element_data + label_offset_), sizeof(size_t));
			unsigned short int link_list_size = *(element_data);
			unsigned int* link_list_data = (unsigned int*) element_data;

			// for node in level 0, global_id == i
			global_id_map.insert({std::make_pair(0, i), global_id});
		
			// std::cout << ">>> label: " << label << "\n" ;
			// std::cout << "    links-lv0: (" << link_list_size << ") ";
			for(int d=1; d <= link_list_size; d++) {
				// edge format: global_id, global_id
				unsigned int temp = *(link_list_data + d);
				// std::cout << temp << " ";
				output_edges << global_id << ", " << temp << "\n";
			}

			// node format: layer_id, global_id, embeddings...
			// std::cout << "\n    data: ";
			output_nodes << "0, " << global_id << ", ";
			for(int j=0; j<dimmension; j++) {
				float temp;
				memcpy(&temp, (element_data + offsetData_ + j * sizeof(temp)), sizeof(temp));
				// std::cout << temp << " ";
				output_nodes << temp << " ";
			}
			output_nodes << "\n";
			// std::cout << "\n";

			global_id++;
		}
		
	}
	num_node_level[0] = global_id;
	std::cout << "finish reading the main data...\n";

	// reading connection in upper levels
	size_t size_links_per_element_ = maxM_ * sizeof(unsigned int) + sizeof(unsigned int);
	std::cout << "reading connection data (" << size_links_per_element_ << ")\n";
	for(unsigned int i=0; i < cur_element_count; i++) {
		unsigned int linkListSize;
		readBinaryPOD(input, linkListSize);

		// if max_level = 0 because linkListSize = 0 then this node
		// only exit in the first level (level=0).
		int max_level = linkListSize / size_links_per_element_;
		
		// print out nodes and count nodes in each level
		for(int l=1; l<=max_level; l++) { 
			
			// count number of nodes in each level
			num_node_level[l]++; 

			// store mapping from unique global_id to internal_id i
			// if not exist
			unsigned int crt_global_node_id;
			auto search = global_id_map.find(std::make_pair(l, i));
			if (search != global_id_map.end()) {
				crt_global_node_id = search->second;
			} else {
				crt_global_node_id = global_id;
				global_id_map.insert({std::make_pair(l, i), global_id});
				global_id++;
			}

			// node format: layer_id, global_id, embeddings...
			output_nodes << l << ", " << crt_global_node_id << ", ";
			char* raw_element_data = (char*) elements_data + i * size_data_per_element_ + offsetData_;
			for(int j=0; j<dimmension; j++) {
				float temp;
				memcpy(&temp, (raw_element_data + j * sizeof(temp)), sizeof(temp));
				output_nodes << temp << " ";
			}
			output_nodes << "\n";
		}

		// print out interlevel and intralevel edges
		// if this node exist in multiple layers
		if (linkListSize>0) {
			char connection_data[linkListSize];
			input.read(connection_data, linkListSize);
			unsigned int cur_internal_node_id = i;

			// std::cout << ">>> id: " << i << ", linkListSize: " << linkListSize << ", level: " << level << "\n";
			for(int l=1; l<=max_level; l++) {
				unsigned int *connection_data_level_i = (unsigned int*) (connection_data + (l-1) * size_links_per_element_);
				unsigned short int size_conn_level_i = *(connection_data_level_i);

				// edge for the same "node" in different layer, connect with lower layer
				// edge format: global_id, global_id
				unsigned int lower_layer_node_id;
				unsigned int crt_global_node_id;		
				{
					auto search = global_id_map.find(std::make_pair(l-1, cur_internal_node_id));
					if (search != global_id_map.end()) {
						lower_layer_node_id = search->second;
					} else {
						std::cout << "(1) could not find global_id for node_id=" << cur_internal_node_id << " level=" << l-1 << "\n";
						exit(1);
					}
					search = global_id_map.find(std::make_pair(l, cur_internal_node_id));
					if (search != global_id_map.end()) {
						crt_global_node_id = search->second;
					} else {
						std::cout << "(2) could not find global_id for node_id=" << cur_internal_node_id << " level=" << l << "\n";
						exit(1);
					}
				}
				output_edges << lower_layer_node_id << ", " << crt_global_node_id << "\n";
				
				// std::cout << "   " << j+1 << ": (" << size_conn_level_i << ") ";
				for(int k=1; k<=size_conn_level_i; k++) {
					// edge format: global_id, global_id
					unsigned int temp = *(connection_data_level_i + k);
					unsigned int neighbor_global_node_id;
					{
						auto search = global_id_map.find(std::make_pair(l, temp));
						if (search != global_id_map.end()) {
							neighbor_global_node_id = search->second;
						} else {
							// if not found, assign global_id for this neighbor node
							global_id_map.insert({std::make_pair(l, temp), global_id});
							neighbor_global_node_id = global_id;
							global_id++;
						}
					}
					// std::cout << temp << " ";
					output_edges << crt_global_node_id << ", " << neighbor_global_node_id << "\n";
				}
				// std::cout << "\n";
			}
		}
	}
	std::cout << "finish reading the connection data\n";

	// print out stat data
	output_stats << "offsetLevel0_: " << offsetLevel0_ << "\n";
	output_stats << "max_elements_: " << max_elements_ << "\n";
	output_stats << "cur_element_count: " << cur_element_count << "\n";
	output_stats << "size_data_per_element_: " << size_data_per_element_ << "\n";
	output_stats << "label_offset_: " << label_offset_ << "\n";
	output_stats << "offsetData_: " << offsetData_ << "\n";
	output_stats << "maxlevel_: " << maxlevel_ << "\n";
	output_stats << "enterpoint_node_: " << enterpoint_node_ << " (global_id=" << global_id_map[std::make_pair(maxlevel_, enterpoint_node_)] << ")\n";
	output_stats << "maxM_: " << maxM_ << "\n";
	output_stats << "maxM0_: " << maxM0_ << "\n";
	output_stats << "M_: " << M_ << "\n";
	output_stats << "mult_: " << mult_ << "\n";
	output_stats << "num nodes: " << global_id << "\n";
	output_stats << "ef_construction_: " << ef_construction_ << "\n";
  	for(int l=0; l <= maxlevel_; l++) {
		std::cout << "   level-" << l << " " << num_node_level[l] << " nodes\n";
    	output_stats << "   level-" << l << " " << num_node_level[l] << " nodes\n";
	}

  	// closing files
  	output_edges.close();
	output_nodes.close();
  	output_stats.close();

	return 0;
}
