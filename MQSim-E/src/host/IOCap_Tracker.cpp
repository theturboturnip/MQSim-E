#include "IOCap_Tracker.h"
#include "../sim/Engine.h"

namespace Host_Components
{
    IOCapLeaseStatus::IOCapLeaseStatus(LeaseID id) : id(id), n_outstanding_commands(0), all_command_submit_times() {}

    static inline sim_time_type max(sim_time_type x, sim_time_type y) {
        return (y > x) ? y : x;
    }

    static inline FlowCommandID join_ids(FlowID flow_id, CommandID command_id) {
        return (((uint32_t)flow_id) << 16) | ((uint32_t)command_id);
    }

    IOCapTracker::IOCapTracker(const sim_object_id_type& name, int n_ops_per_lease) : MQSimEngine::Sim_Object(name), n_ops_per_lease(n_ops_per_lease), lease_statuses(), outstanding_commands(), STAT_sum_time_exposed(0), STAT_max_time_exposed(0), STAT_sum_extra_time_exposed(0), STAT_max_extra_time_exposed(0),  STAT_total_lease_ids_required_for_total_availability(0), next_lease(nullptr), STAT_total_lease_openings(0), STAT_sum_lease_occupancy_at_close(0) {
    }

    void IOCapTracker::submit_command(FlowID flow_id, CommandID command_id) {
        auto time = Simulator->Time();
        auto combined_id = join_ids(flow_id, command_id);
        if (outstanding_commands.find(combined_id) != outstanding_commands.end()) {
            PRINT_ERROR("reused flow_id/command_id " << flow_id << " " << command_id);
        } else {
            // Search for a lease to use.
            // If we find an open one
            auto lease_to_use = next_lease;
            if (lease_to_use == nullptr) {
                // Search for an open usable lease
                for (auto& lease : lease_statuses) {
                    if (lease.all_command_submit_times.size() < n_ops_per_lease) {
                        lease_to_use = &lease;
                        break;
                    }
                }
                if (lease_to_use == nullptr) {
                    // OK, we have to make a new one
                    lease_statuses.emplace_back(lease_statuses.size());
                    lease_to_use = &lease_statuses.back();
                    STAT_total_lease_ids_required_for_total_availability = lease_statuses.size();
                }
            }
            // lease_to_use is non-NULL
            if (!lease_to_use->is_open()) {
                STAT_total_lease_openings++;
            }
            lease_to_use->n_outstanding_commands++;
            lease_to_use->all_command_submit_times.push_back(time);
            if (lease_to_use->all_command_submit_times.size() < n_ops_per_lease) {
                // I worry that this strategy of sticking to a lease until it has been filled,
                // and indeed keeping this pointer alive even if the lease it points to is closed,
                // could end up lowering average occupancy-on-close?
                next_lease = lease_to_use;
            }
            outstanding_commands[combined_id] = lease_to_use->id;
        }
    }
	void IOCapTracker::complete_command(FlowID flow_id, CommandID command_id) {
        auto time = Simulator->Time();
        auto combined_id = join_ids(flow_id, command_id);

        auto outstanding_command = outstanding_commands.find(command_id);
        if (outstanding_command == outstanding_commands.end()) {
            PRINT_ERROR("completed unsubmitted flow_id/command_id " << flow_id << " " << command_id);
        }
        auto lease_id = outstanding_commands[combined_id];
        auto& lease = lease_statuses[lease_id];
        outstanding_commands.erase(outstanding_command);

        lease.n_outstanding_commands--;
        if (lease.n_outstanding_commands < 0) {
            PRINT_ERROR("completed flow/command " << flow_id << " " << command_id << ", mapped to lease " << lease_id << " but now has " << lease.n_outstanding_commands << " outstanding...");
        }

        lease.all_command_close_times.push_back(time);

        if (lease.n_outstanding_commands == 0) {
            // closing the lease!

            for (auto submit_time : lease.all_command_submit_times) {
                auto time_exposed = time - submit_time;
                STAT_sum_time_exposed += time_exposed;
                STAT_max_time_exposed = max(STAT_max_time_exposed, time_exposed);
            }

            for (auto close_time : lease.all_command_close_times) {
                auto extra_time_exposed = time - close_time;
                STAT_sum_extra_time_exposed += extra_time_exposed;
                STAT_max_extra_time_exposed = max(STAT_max_extra_time_exposed, extra_time_exposed);
            }

            STAT_sum_lease_occupancy_at_close += lease.all_command_submit_times.size();

            lease.all_command_submit_times.clear();
            lease.all_command_close_times.clear();
        } else {
            // still have outstanding commands? doesn't matter, keep going.
        }
	}

	void IOCapTracker::Report_results_in_XML(std::string name_prefix, Utils::XmlWriter& xmlwriter) {
	    if (!outstanding_commands.empty()) {
			PRINT_ERROR("Reporting results while commands outstanding");
		}
		std::string tmp = name_prefix + ".IOCapTracker";
		xmlwriter.Write_open_tag(tmp);

		std::string attr = "Name";
		std::string val = ID();
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Sum_Time_Exposed";
		val = std::to_string(STAT_sum_time_exposed);
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Max_Time_Exposed";
		val = std::to_string(STAT_max_time_exposed);
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Sum_Extra_Time_Exposed";
		val = std::to_string(STAT_sum_extra_time_exposed);
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Max_Extra_Time_Exposed";
		val = std::to_string(STAT_max_extra_time_exposed);
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Total_Lease_Ids_Required_For_Total_Availability";
		val = std::to_string(STAT_total_lease_ids_required_for_total_availability);
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Total_Lease_Openings";
		val = std::to_string(STAT_total_lease_openings);
		xmlwriter.Write_attribute_string(attr, val);

		attr = "Sum_Lease_Occupancy_At_Close";
		val = std::to_string(STAT_sum_lease_occupancy_at_close);
		xmlwriter.Write_attribute_string(attr, val);

		xmlwriter.Write_close_tag();
	}

	void IOCapTracker::Start_simulation()
    {
    }

    void IOCapTracker::Validate_simulation_config()
    {
        if (n_ops_per_lease <= 0)
        {
            PRINT_ERROR(ID() << " given n_ops_per_lease=" << n_ops_per_lease);
        }
    }

    void IOCapTracker::Execute_simulator_event(MQSimEngine::Sim_Event* event)
    {
    }
}
