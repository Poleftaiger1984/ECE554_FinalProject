import serial
import time
import psutil
import wmi
import requests
import json

print("Opening virtual USB COM port to QEMU...")

# On real physical Pi change this string to COM3
PORT_NAME = "socket://127.0.0.1:4321"

# change to any API key
API_KEY = "KEY GOES HERE"
AI_COOLDOWN_SECONDS = 60
last_ai_trigger_time = 0

def get_ai_report(log_history):
    #this is an interchangeable ai function currently set up to work with gemini
    if API_KEY == "KEY GOES HERE" or API_KEY == "":
        return "API KEY MISSING. Add key to Python script."

    print(">> Generating AI Root Cause Report...")

    prompt = (
        "You are a PC hardware diagnostics expert. Analyze this system log "
        "and identify the most likely root cause of any anomalies. Be concise "
        "— max 15 words. Log data:\n" + json.dumps(log_history)
    )

    # change to any LLM service
    url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key={API_KEY}"

    payload = {
        "contents": [{"parts": [{"text": prompt}]}]
    }

    try:
        response = requests.post(url, headers = {'Content-Type': 'application/json'}, json=payload)
        response.raise_for_status()
        data = response.json()

        # extract data from gemini json
        report_text = data['candidates'][0]['content']['parts'][0]['text'].strip()

        report_text = report_text.replace('\n', ' ')
        return report_text

    except Exception as e:
        print(f"AI API ERROR: {e}")
        return "AI DIAGNOSTIC FAILED TO GENERATE"

def get_top_tasks():
    task_dict = {}
    core_count = psutil.cpu_count(logical=True)

    for proc in psutil.process_iter(['name', 'cpu_percent', 'memory_info']):
        try:
            info = proc.info
            name = info['name']

            # skip windows idle tasks and VM
            if name in ['System Idle Process', 'System', 'Registry', 'VmmemWSL']:
                continue

            clean_name = name.replace('.exe', '').replace(',', '').upper()

            cpu = info['cpu_percent'] or 0.0

            ram_mb = (info['memory_info'].rss) / (1024 * 1024) if info['memory_info'] else 0

            if clean_name not in task_dict:
                task_dict[clean_name] = {'cpu': cpu, 'ram': ram_mb}
            else:
                task_dict[clean_name]['cpu'] += cpu
                task_dict[clean_name]['ram'] += ram_mb

        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            pass

    # Sortby CPU usage (highest first)
    sorted_tasks = sorted(task_dict.items(), key=lambda item: item[1]['cpu'], reverse=True)
    
    top_3 = []

    for i in range(3):
        if i < len(sorted_tasks):
            t_name = sorted_tasks[i][0][:8]
            t_cpu = int(sorted_tasks[i][1]['cpu'] / core_count)
            t_ram = int(sorted_tasks[i][1]['ram'])
            top_3.append((t_name, t_cpu, t_ram))
        else:
            top_3.append(("NONE", 0, 0))

    return top_3

try:
    w = wmi.WMI(namespace="root\\wmi")
except Exception as e:
    print(f"Warning: WMI initialization failed. {e}")
    w = None

try:
    with serial.serial_for_url(PORT_NAME, baudrate=115200, timeout=1) as ser:
        print("Plugged in")
        
        packet_count = 0

        # list to hold the last 60 seconds of data
        rolling_log = []

        prev_disk_io = psutil.disk_io_counters()

        while True:
            packet_count += 1
            # --- STAT GATHERING ---
            cpu_u = int(psutil.cpu_percent(interval=None)) 
            
            cpu_t = 500

            if w is not None:
                try:
                    thermal_zones = w.MSAcpi_ThermalZoneTemperature()
                    if len(thermal_zones) > 0:
                        # WMI returns Kelvin
                        kelvin_tenths = thermal_zones[0].CurrentTemperature
                        # Convert to tenths of degrees Celsius
                        cpu_t = int(kelvin_tenths - 2732)
                except Exception:
                    pass 

            vm = psutil.virtual_memory()
            ram_t = int(vm.total / (1024 * 1024 * 1024))
            ram_m = int(vm.used / (1024 * 1024 * 1024))
            ram_u = int(vm.percent)

            curr_disk_io = psutil.disk_io_counters()
            read_mb  = (curr_disk_io.read_bytes  - prev_disk_io.read_bytes)  / (1024 * 1024)
            write_mb = (curr_disk_io.write_bytes - prev_disk_io.write_bytes) / (1024 * 1024)
            disk_io_mbps = int(read_mb + write_mb)
            prev_disk_io = curr_disk_io

            # --- TASK GATHERING ---
            top_tasks = get_top_tasks()

            # --- UPDATE LOG ---
            current_snapshot = {
                "CPU%": cpu_u, "TempC": cpu_t/10, "RAM%": ram_u,
                "Top_Task": top_tasks[0][0], "Task_CPU": top_tasks[0][1]
            }
            rolling_log.append(current_snapshot)
            if len(rolling_log) > 60:
                rolling_log.pop(0) # remove oldest entry after 60

            # --- FORMAT REGULAR STRING ---
            task_str = (
                f"AN{top_tasks[0][0]},AC{top_tasks[0][1]},AR{top_tasks[0][2]},"
                f"BN{top_tasks[1][0]},BC{top_tasks[1][1]},BR{top_tasks[1][2]},"
                f"CN{top_tasks[2][0]},CC{top_tasks[2][1]},CR{top_tasks[2][2]}"
            )

            
            message = f"$CU{cpu_u},CT{cpu_t},RT{ram_t},RU{ram_u},RM{ram_m},DI{disk_io_mbps},{task_str}\n"

            ser.write(message.encode('utf-8'))
            ser.flush()
            print(f"[{packet_count}] Sent over virtual USB: {message.strip()}")
            
            current_time = time.time()
            is_anomaly = (cpu_u > 95) or (cpu_t > 900) or (ram_u > 95)

            if is_anomaly and (current_time - last_ai_trigger_time > AI_COOLDOWN_SECONDS):
                ai_text = get_ai_report(rolling_log)
                ai_packet = f"@{ai_text}\n"

                ser.write(ai_packet.encode('utf-8'))
                ser.flush()
                print(f"    [!] AI Report Sent: {ai_packet.strip()}")
                
                last_ai_trigger_time = current_time
            
            time.sleep(1) 
            
except serial.SerialException as e:
    print(f"\nFailed to connect.\nError: {e}")
except KeyboardInterrupt:
    print("\nStopping script...")