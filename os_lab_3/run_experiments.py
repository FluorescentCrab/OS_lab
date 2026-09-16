import os
import re
import subprocess
import csv
from pathlib import Path

# --- Configuration ---
WORKLOADS = [f"testcases/process{i}.txt" for i in range(1, 6)]
CPUS = [1, 2]
RR_QUANTA = [2, 5, 10, 20, 50]

# MLFQ parameters
MLFQ_TQ = 2
MLFQ_QUEUES = 3
MLFQ_QTIME = 10
MLFQ_RESET_NO_RESET = 1000000  # Large value to simulate "no reset"
MLFQ_RESET_WITH_RESET = 20

# Output parsing patterns based on your provided format
METRIC_PATTERNS = {
    'run_time': r'RUN TIME\s*:\s*(\d+)',
    'avg_turn_around_time': r'AVERAGE TURN AROUND TIME\s*:\s*(\d+)',
    'max_turn_around_time': r'MAX TURN AROUND TIME\s*:\s*(\d+)',
    # Added waiting time patterns just in case they appear in other outputs
    'avg_waiting_time': r'AVERAGE WAITING TIME\s*:\s*(\d+)',
    'max_waiting_time': r'MAX WAITING TIME\s*:\s*(\d+)'
}

RESULTS_DIR = Path("results")
RAW_DIR = RESULTS_DIR / "raw"
CSV_FILE = RESULTS_DIR / "report.csv"
SUMMARY_FILE = RESULTS_DIR / "SUMMARY.txt"

def parse_output(output_text):
    metrics = {}
    for key, pattern in METRIC_PATTERNS.items():
        match = re.search(pattern, output_text)
        if match:
            metrics[key] = int(match.group(1))
        else:
            metrics[key] = None
    return metrics

def run_command(cmd, label):
    try:
        # Run the command and capture output
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30  # 30 seconds timeout per run to prevent infinite loops
        )
        output = result.stdout + "\n" + result.stderr
        
        # Save raw output to file
        with open(RAW_DIR / f"{label}.txt", "w") as f:
            f.write(output)
            
        metrics = parse_output(output)
        status = "SUCCESS" if result.returncode == 0 else f"FAILED (code {result.returncode})"
        
        return status, metrics
    except subprocess.TimeoutExpired:
        with open(RAW_DIR / f"{label}.txt", "w") as f:
            f.write("TIMEOUT EXPIRED")
        return "TIMEOUT", {k: None for k in METRIC_PATTERNS}
    except Exception as e:
        with open(RAW_DIR / f"{label}.txt", "w") as f:
            f.write(f"EXCEPTION: {str(e)}")
        return "ERROR", {k: None for k in METRIC_PATTERNS}

def main():
    # Setup directories
    RESULTS_DIR.mkdir(exist_ok=True)
    RAW_DIR.mkdir(exist_ok=True)
    
    experiments = []
    
    # 1. FIFO (5 workloads * 2 CPU counts = 10 runs)
    for wl in WORKLOADS:
        for cpu in CPUS:
            cmd = ['./fifo_mp.out', wl, str(cpu)]
            label = f"fifo_wl{Path(wl).stem}_cpu{cpu}"
            experiments.append(('fifo', wl, cpu, None, None, None, None, cmd, label))
            
    # 2. Round Robin (5 workloads * 5 quanta * 2 CPU counts = 50 runs)
    for wl in WORKLOADS:
        for q in RR_QUANTA:
            for cpu in CPUS:
                cmd = ['./round_robin_mp.out', wl, str(q), str(cpu)]
                label = f"rr_wl{Path(wl).stem}_q{q}_cpu{cpu}"
                experiments.append(('round_robin', wl, cpu, q, None, None, None, cmd, label))
                
    # 3. MLFQ no reset (5 workloads * 2 CPU counts = 10 runs)
    for wl in WORKLOADS:
        for cpu in CPUS:
            cmd = ['./mlfq_mp.out', wl, str(MLFQ_TQ), str(MLFQ_QUEUES), str(MLFQ_RESET_NO_RESET), str(MLFQ_QTIME), str(cpu)]
            label = f"mlfq_no_reset_wl{Path(wl).stem}_cpu{cpu}"
            experiments.append(('mlfq_no_reset', wl, cpu, MLFQ_TQ, MLFQ_QUEUES, MLFQ_RESET_NO_RESET, MLFQ_QTIME, cmd, label))
            
    # 4. MLFQ reset=20 (5 workloads * 2 CPU counts = 10 runs)
    for wl in WORKLOADS:
        for cpu in CPUS:
            cmd = ['./mlfq_mp.out', wl, str(MLFQ_TQ), str(MLFQ_QUEUES), str(MLFQ_RESET_WITH_RESET), str(MLFQ_QTIME), str(cpu)]
            label = f"mlfq_reset20_wl{Path(wl).stem}_cpu{cpu}"
            experiments.append(('mlfq_reset20', wl, cpu, MLFQ_TQ, MLFQ_QUEUES, MLFQ_RESET_WITH_RESET, MLFQ_QTIME, cmd, label))

    # Run experiments and collect results
    results = []
    success_count = 0
    fail_count = 0
    
    print(f"Running {len(experiments)} experiments...")
    
    for exp in experiments:
        scheduler, wl, cpu, q, queues, reset, qtime, cmd, label = exp
        print(f"  [{label}] {' '.join(cmd)}")
        status, metrics = run_command(cmd, label)
        
        if status == "SUCCESS":
            success_count += 1
        else:
            fail_count += 1
            
        row = {
            'scheduler': scheduler,
            'workload': wl,
            'cpus': cpu,
            'quantum': q if q is not None else '',
            'queues': queues if queues is not None else '',
            'reset_time': reset if reset is not None else '',
            'queue_time': qtime if qtime is not None else '',
            'status': status
        }
        row.update(metrics)
        results.append(row)
        
    # Write CSV Report
    fieldnames = ['scheduler', 'workload', 'cpus', 'quantum', 'queues', 'reset_time', 'queue_time', 'status'] + list(METRIC_PATTERNS.keys())
    with open(CSV_FILE, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)
        
    # Write Summary
    with open(SUMMARY_FILE, 'w') as f:
        f.write(f"Total experiments: {len(experiments)}\n")
        f.write(f"Successful: {success_count}\n")
        f.write(f"Failed/Timeout: {fail_count}\n")
        
    print(f"\nDone! Results saved to {RESULTS_DIR}")
    print(f"  - Raw outputs: {RAW_DIR}")
    print(f"  - CSV Report: {CSV_FILE}")
    print(f"  - Summary: {SUMMARY_FILE}")

if __name__ == "__main__":
    main()
