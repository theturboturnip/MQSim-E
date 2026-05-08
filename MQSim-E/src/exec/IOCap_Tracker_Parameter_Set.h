#ifndef IOCAP_TRACKER_PARAMETER_SET_H
#define IOCAP_TRACKER_PARAMETER_SET_H

#include <string>
#include "Parameter_Set_Base.h"

class IOCap_Tracker_Parameter_Set : public Parameter_Set_Base
{
public:
    // int n_ops_per_lease;
	void XML_serialize(Utils::XmlWriter& xmlwrite);
	void XML_deserialize(rapidxml::xml_node<> *node);
private:
};

#endif // !IOCAP_TRACKER_PARAMETER_SET_H
