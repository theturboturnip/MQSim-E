#ifndef IOCAP_TRACKER_H
#define IOCAP_TRACKER_H

#include <string>
#include <iostream>
#include <unordered_map>
#include <set>
#include <list>
#include <vector>
#include "../sim/Sim_Defs.h"
#include "../sim/Sim_Object.h"
#include "../sim/Sim_Reporter.h"
#include "../ssd/SSD_Defs.h"
#include "../ssd/Host_Interface_Defs.h"
#include "Host_IO_Request.h"
#include "../utils/Workload_Statistics.h"

namespace Host_Components
{
    using FlowID = uint16_t;
    using CommandID = uint16_t;
    using FlowCommandID = uint32_t;
    using LeaseID = size_t;

    struct IOCapLeaseStatus {
        LeaseID id;
        int n_outstanding_commands; // no need to be unsigned, detect if something has gone wrong with <0
        std::vector<sim_time_type> all_command_submit_times;
        std::vector<sim_time_type> all_command_close_times;

        IOCapLeaseStatus(LeaseID id);

        inline bool is_open() const {
            return !all_command_submit_times.empty();
        }
    };

	class SingleIOCapTrackerContext : public MQSimEngine::Sim_Object, public MQSimEngine::Sim_Reporter
	{
	public:
	    SingleIOCapTrackerContext(const sim_object_id_type& name, int n_ops_per_lease);

	    void submit_command(FlowID flow_id, CommandID command_id);
		void complete_command(FlowID flow_id, CommandID command_id);

		virtual void Start_simulation();
		virtual void Validate_simulation_config();
		virtual void Execute_simulator_event(MQSimEngine::Sim_Event* event);
		virtual void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);
	protected:
	    unsigned int n_ops_per_lease;
	    std::vector<IOCapLeaseStatus> lease_statuses;
		IOCapLeaseStatus* next_lease;
		std::unordered_map<FlowCommandID, LeaseID> outstanding_commands;

		sim_time_type STAT_sum_time_exposed;
		sim_time_type STAT_max_time_exposed;
		sim_time_type STAT_sum_extra_time_exposed;
		sim_time_type STAT_max_extra_time_exposed;
		LeaseID STAT_total_lease_ids_required_for_total_availability;
		LeaseID STAT_total_lease_openings;
		LeaseID STAT_sum_lease_occupancy_at_close;
	};

	class IOCapTrackerContext : public MQSimEngine::Sim_Object, public MQSimEngine::Sim_Reporter
	{
	public:
	    IOCapTrackerContext(const sim_object_id_type& name);

	    void submit_command(FlowID flow_id, CommandID command_id);
		void complete_command(FlowID flow_id, CommandID command_id);

		virtual void Start_simulation();
		virtual void Validate_simulation_config();
		virtual void Execute_simulator_event(MQSimEngine::Sim_Event* event);
		virtual void Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter);
	protected:
        std::vector<SingleIOCapTrackerContext> contexts;
	};
}

#endif // !IOCAP_TRACKER_H
