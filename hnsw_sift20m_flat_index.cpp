#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>

template<typename T>
void readBinaryPOD(std::istream &in,T &podRef) {
	in.read((char *) &podRef, sizeof(T));
}

int main() {
	char *path_index = "sift1b_20m_ef_40_M_16.bin";
	char *path_data = "hnsw_data.dat";
	char *path_edges = "hnsw_edge.dat";
	char *path_nodes = "hnsw_node.dat";
  	char *path_stats = "hnsw_stat.dat";
	
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
	int dimmension = 128;
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
	char* element_data;
	for(int i=0; i < cur_element_count; i++) {
		element_data = (elements_data + i * size_data_per_element_);
		
		{	
			size_t label; 
			memcpy(&label,(element_data + label_offset_), sizeof(size_t));
			unsigned short int link_list_size = *(element_data);
			unsigned int* link_list_data = (unsigned int*) element_data;
		
			// std::cout << ">>> label: " << label << "\n" ;
			// std::cout << "    links-lv0: (" << link_list_size << ") ";
			for(int d=1; d <= link_list_size; d++) {
				unsigned int temp = *(link_list_data + d);
				// std::cout << temp << " ";
				output_edges << "0." << i << ", 0." << temp << "\n";
			}
			// std::cout << "\n    data: ";
			output_nodes << "0, 0." << i << ", ";
			for(int j=0; j<dimmension; j++) {
				float temp = *(element_data + offsetData_ + j);
				// std::cout << temp << " ";
				output_nodes << temp << " ";
			}
			output_nodes << "\n";
			// std::cout << "\n";
		}
		
	}
	num_node_level[0] = cur_element_count;
	std::cout << "finish reading the main data...\n";

	// reading connection in upper levels
	size_t size_links_per_element_ = maxM_ * sizeof(unsigned int) + sizeof(unsigned int);
	std::cout << "reading connection data (" << size_links_per_element_ << ")\n";
	for(int i=0; i < cur_element_count; i++) {
		unsigned int linkListSize;
		readBinaryPOD(input, linkListSize);
		unsigned int level = linkListSize / size_links_per_element_;
		
		// print out nodes and count nodes in each level
		for(int l=1; l<=level; l++) { 
			num_node_level[l]++; 

			output_nodes << l << ", " << l << "." << i << ", ";
			unsigned int* raw_element_data = (unsigned int*) elements_data + i * size_data_per_element_ + offsetData_;
			for(int j=0; j<dimmension; j++) {
				float temp = *(raw_element_data + j);
				output_nodes << temp << " ";
			}
			output_nodes << "\n";
		}

		// print out interlevel and intralevel edges
		if (linkListSize>0) {
			char connection_data[linkListSize];
			input.read(connection_data, linkListSize);

			// std::cout << ">>> id: " << i << ", linkListSize: " << linkListSize << ", level: " << level << "\n";
			for(unsigned int j=0; j<level; j++) {
				unsigned int *connection_data_level_i = (unsigned int*) (connection_data + j * size_links_per_element_);
				unsigned short int size_conn_level_i = *(connection_data_level_i);

				// edge for the same node in different layer, connect with below layer
				output_edges << level << "." << i << ", " << level+1 << "." << i << "\n";
				
				// std::cout << "   " << j+1 << ": (" << size_conn_level_i << ") ";
				for(int k=1; k<=size_conn_level_i; k++) {
					unsigned int temp = *(connection_data_level_i + k);
					// std::cout << temp << " ";
					output_edges << level+1 << "." << i << ", " << level+1 << "." << temp << "\n";
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
  output_stats << "enterpoint_node_: " << enterpoint_node_ << "\n";
  output_stats << "maxM_: " << maxM_ << "\n";
  output_stats << "maxM0_: " << maxM0_ << "\n";
  output_stats << "M_: " << M_ << "\n";
  output_stats << "mult_: " << mult_ << "\n";
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
