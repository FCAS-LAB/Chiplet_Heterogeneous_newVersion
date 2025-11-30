import re
from collections import defaultdict
import argparse

def analyze_chiplet_communication_computation(log_file_path, cpu_node=35):
    raw_records = []
    pattern = r'start time: (\d+) source address: (\d+) destination address: (\d+) delay: (\d+)'
    
    try:
        with open(log_file_path, 'r') as file:
            for line in file:
                match = re.search(pattern, line)
                if match:
                    start_time, source, destination, delay = match.groups()
                    start_time = int(start_time)
                    source = int(source)
                    destination = int(destination)
                    delay = int(delay)
                    
                    raw_records.append({
                        'start_time': start_time,
                        'source': source,
                        'destination': destination,
                        'delay': delay
                    })
    except Exception as e:
        print(f"Error reading file: {e}")
        return
    
    max_delay_records = {}
    for record in raw_records:
        start_time = record['start_time']
        delay = record['delay']
        
        if start_time not in max_delay_records or delay > max_delay_records[start_time]['delay']:
            max_delay_records[start_time] = record
    
    communication_records = list(max_delay_records.values())
    communication_records.sort(key=lambda x: x['start_time'])
    
    print(f"Original records: {len(raw_records)}")
    print(f"Deduplicated records: {len(communication_records)}")
    
    print("\nAll deduplicated communication records:")
    for i, record in enumerate(communication_records):
        print(f"{i+1}. Time: {record['start_time']}, Source: {record['source']}, Destination: {record['destination']}, Delay: {record['delay']}")
    
    node_timeline = defaultdict(list)
    
    for record in communication_records:
        start_time = record['start_time']
        source = record['source']
        destination = record['destination']
        
        node_timeline[source].append(('send', start_time, record))
        node_timeline[destination].append(('receive', start_time, record))
    
    computation_time = defaultdict(list)
    computation_details = defaultdict(list)
    
    for node, events in node_timeline.items():
        events.sort(key=lambda x: x[1])
        
        for i in range(len(events) - 1):
            current_event = events[i]
            next_event = events[i + 1]
            
            if current_event[0] == 'receive' and next_event[0] == 'send':
                recv_time = current_event[1]
                send_time = next_event[1]
                
                comp_time = send_time - recv_time
                
                if comp_time > 0:
                    computation_time[node].append(comp_time)
                    
                    computation_details[node].append({
                        'receive_time': recv_time,
                        'receive_record': current_event[2],
                        'send_time': send_time,
                        'send_record': next_event[2],
                        'computation_time': comp_time
                    })
    
    communication_time = defaultdict(int)
    for record in communication_records:
        communication_time[record['destination']] += record['delay']
    
    tau_values = {}
    for node in set(communication_time.keys()):
        total_comp_time = sum(computation_time[node])
        total_comm_time = communication_time[node]
        
        if total_comm_time > 0:
            tau = total_comp_time / total_comm_time
            tau_values[node] = tau
        else:
            tau_values[node] = "N/A"
    
    print("\nNode Communication Statistics:")
    for node in sorted(set([rec['source'] for rec in communication_records] + [rec['destination'] for rec in communication_records])):
        as_source = sum(1 for rec in communication_records if rec['source'] == node)
        as_dest = sum(1 for rec in communication_records if rec['destination'] == node)
        print(f"Node {node}: As source {as_source} times, As destination {as_dest} times")
    
    print("\nNode Computation and Communication Analysis:")
    print("Node\tTotal Comp Time\tTotal Comm Time\tτ Value\tBottleneck Type")
    print("-" * 70)
    
    total_comm_time_all = 0
    total_comp_time_all = 0
    for node in sorted(set(communication_time.keys())):
        total_comp_time = sum(computation_time[node])
        total_comm_time = communication_time[node]
        
        tau = tau_values[node]
        if tau != "N/A":
            bottleneck = "Computation Bound" if tau > 1 else "Communication Bound"
        else:
            bottleneck = "Unknown"
        
        if(node != cpu_node): 
            print(f"{node}\t{total_comp_time}\t{total_comm_time}\t{tau}\t{bottleneck}")
            total_comm_time_all += total_comm_time
            total_comp_time_all += total_comp_time
    
    print(f"Total computation time: {total_comp_time_all}")
    print(f"Total communication time: {total_comm_time_all}")
    avg_tau = total_comp_time_all / total_comm_time_all
    print(f"Average τ value: {avg_tau}")
    
    print("\nComputation Time Details for Each Node:")
    for node in sorted(computation_time.keys()):
        times = computation_time[node]
        if times:
            print(f"Node {node} computation times: {times}")
            print(f"Node {node} computation count: {len(times)}")
            print(f"Node {node} average computation time: {sum(times)/len(times)}")
            
            print(f"Detailed computation process for Node {node}:")
            for i, detail in enumerate(computation_details[node]):
                recv = detail['receive_record']
                send = detail['send_record']
                print(f"  Computation {i+1}:")
                print(f"    Receive: Time {detail['receive_time']}, Source {recv['source']}, Destination {recv['destination']}, Delay {recv['delay']}")
                print(f"    Send: Time {detail['send_time']}, Source {send['source']}, Destination {send['destination']}, Delay {send['delay']}")
                print(f"    Computation Time: {detail['computation_time']}")
            
            print("-" * 30)
    
    return {
        'communication_records': communication_records,
        'computation_time': dict(computation_time),
        'communication_time': dict(communication_time),
        'tau_values': tau_values
    }

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="communication and computation proportion analyse")
    parser.add_argument("--popnet_log", default="./proc_r1_p2_t0/popnet_0.log", help="Popnet log file path")
    parser.add_argument("--cpu_node", type=int, default=35, help="CPU node ID for analysis")
    args = parser.parse_args()
    log_file_path = args.popnet_log
    cpu_node = args.cpu_node
    analyze_chiplet_communication_computation(log_file_path, cpu_node)
