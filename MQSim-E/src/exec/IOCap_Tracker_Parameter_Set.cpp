#include "IOCap_Tracker_Parameter_Set.h"
#include <string>
#include <set>
#include <cstring>
#include <algorithm>
#include "../sim/Sim_Defs.h"

//All serialization and deserialization functions should be replaced by a C++ reflection implementation
void IOCap_Tracker_Parameter_Set::XML_serialize(Utils::XmlWriter& xmlwriter)
{
	std::string attr = "N_Ops_Per_Lease";
	std::string val = std::to_string(n_ops_per_lease);
	xmlwriter.Write_attribute_string(attr, val);
}

void IOCap_Tracker_Parameter_Set::XML_deserialize(rapidxml::xml_node<> *node)
{
	try {
		for (auto param = node->first_node(); param; param = param->next_sibling()) {
			if (strcmp(param->name(), "N_Ops_Per_Lease") == 0) {
				std::string val = param->value();
				n_ops_per_lease = std::stoul(val);
			} else {
                PRINT_ERROR("N_Ops_Per_Lease not configured!");
			}
		}
	} catch (...) {
		PRINT_ERROR("Error in IOCap_Tracker_Parameter_Set!")
	}
}
